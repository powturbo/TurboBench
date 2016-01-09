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
from optparse import OptionParser, OptionGroup
# External imports
from scapy.config import conf
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI, SAPNIProxyHandler, SAPNIProxy
from pysap.SAPDiag import SAPDiag, SAPDiagDP
from pysap.SAPDiagItems import *


# Bind the SAPDiag layer
bind_layers(SAPNI, SAPDiag,)
bind_layers(SAPNI, SAPDiagDP,)
bind_layers(SAPDiagDP, SAPDiag,)
bind_layers(SAPDiag, SAPDiagItem,)
bind_layers(SAPDiagItem, SAPDiagItem,)


# Set the verbosity to 0
conf.verb = 0


def filter_client(packet):
    atoms = []
    # Grab all the Atom items in the packet
    if SAPDiag in packet:
        atoms = packet[SAPDiag].get_item(["APPL", "APPL4"], "DYNT", "DYNT_ATOM")

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

    # Return the original packet
    return packet


def filter_server(packet):
    # Just return the original packet, server's data is not relevant
    # for this example
    return packet


class SAPDiagProxyHandler(SAPNIProxyHandler):
    """
    SAP Diag Proxy Handler

    Handles Diag packets and pass the data to filter functions
    """

    def process_client(self, packet):
        # Reprocess the messages using filter_client function
        packet = filter_client(packet)
        # Return the message
        return packet

    def process_server(self, packet):
        # Reprocess the packet using filter_server function
        packet = filter_server(packet)
        # Return the message
        return packet


# Command line options parser
def parse_options():

    description = \
    """This example script can be used to establish a proxy between a SAP GUI
    client and a SAP Netweaver Application Server and inspect the traffic via
    the filter_client and filter_server functions.

    The given example grabs input fields sent by the client.
    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host",
                      help="Remote host")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int",
                      help="Remote port [%default]", default=3200)
    parser.add_option_group(target)

    local = OptionGroup(parser, "Local")
    local.add_option("-b", "--local-host", dest="local_host",
                     help="Local address [%default]", default="127.0.0.1")
    local.add_option("-l", "--local-port", dest="local_port", type="int",
                     help="Local port [%default]", default=3200)
    parser.add_option_group(local)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true",
                    default=False, help="Verbose output [%default]")
    parser.add_option_group(misc)

    (options, _) = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    print("[*] Establishing a Diag proxy between %s:%d and remote %s:%d" % (options.local_host,
                                                                            options.local_port,
                                                                            options.remote_host,
                                                                            options.remote_port))
    proxy = SAPNIProxy(options.local_host, options.local_port,
                       options.remote_host, options.remote_port,
                       SAPDiagProxyHandler)
    while(True):
        proxy.handle_connection()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("[*] Canceled by the user ...")
        exit(0)
