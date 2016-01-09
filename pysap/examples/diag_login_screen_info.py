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
from re import escape
from optparse import OptionParser, OptionGroup
# External imports
from scapy.config import conf
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI
from pysap.SAPDiagItems import *
from pysap.SAPDiagClient import SAPDiagConnection
from pysap.SAPDiag import (SAPDiag, SAPDiagDP, diag_appl_ids, diag_appl_sids,
                           diag_item_types)


# Bind the SAPDiag layer
bind_layers(SAPNI, SAPDiag,)
bind_layers(SAPNI, SAPDiagDP,)
bind_layers(SAPDiagDP, SAPDiag,)
bind_layers(SAPDiag, SAPDiagItem,)
bind_layers(SAPDiagItem, SAPDiagItem,)


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = \
    """This example script gather information provided by a SAP Netweaver
    Application Server during the login process. This information includes
    generally hostname, instance, database name, etc.
    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host", help="Remote host")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int", help="Remote port [%default]", default=3200)
    target.add_option("--route-string", dest="route_string", help="Route string for connecting through a SAP Router")
    parser.add_option_group(target)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Verbose output [%default]")
    misc.add_option("--terminal", dest="terminal", default=None, help="Terminal name")
    parser.add_option_group(misc)

    (options, _) = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")

    return options


def show(item):
    """
    Print the information about each item: type, ID, SID and value.

    """
    print("\tType = %s\tId = %s\tSID = %s\tValue = %s" % (diag_item_types[item.item_type],
                                                          diag_appl_ids[item.item_id],
                                                          diag_appl_sids[item.item_id][item.item_sid],
                                                          escape(str(item.item_value))))


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Create the connection to the SAP Netweaver server
    print("[*] Connecting to %s:%d" % (options.remote_host, options.remote_port))
    connection = SAPDiagConnection(options.remote_host, options.remote_port,
                                   init=False, terminal=options.terminal,
                                   route=options.route_string)

    # Send the initialization packet and store the response (login screen)
    login_screen = connection.init()
    # Filter the response and show the interesting info
    print("[*] Login Screen information:")
    for item in login_screen[SAPDiag].get_item(["APPL", "APPL4"],
                                               ["ST_R3INFO", "ST_USER", "VARINFO"]):
        show(item)

    # Close the connection
    connection.close()


if __name__ == "__main__":
    main()
