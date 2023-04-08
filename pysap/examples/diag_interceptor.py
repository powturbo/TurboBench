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
from argparse import ArgumentParser
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
    if atoms:
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

    description = "This example script can be used to establish a proxy between a SAP GUI client and a SAP Netweaver " \
                  "Application Server and inspect the traffic via the filter_client and filter_server functions.\n" \
                  "The given example grabs input fields sent by the client."

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3200,
                        help="Remote port [%(default)d]")

    local = parser.add_argument_group("Local")
    local.add_argument("-b", "--local-host", dest="local_host", default="127.0.0.1",
                       help="Local address [%(default)s]")
    local.add_argument("-l", "--local-port", dest="local_port", type=int, default=3200,
                       help="Local port [%(default)d]")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

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
    while True:
        proxy.handle_connection()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("[*] Canceled by the user ...")
        exit(0)
