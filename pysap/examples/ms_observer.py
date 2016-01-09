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
from socket import error as SocketError
from optparse import OptionParser, OptionGroup
# External imports
from scapy.config import conf
# Custom imports
import pysap
from pysap.SAPMS import SAPMS
from pysap.SAPRouter import SAPRoutedStreamSocket


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = \
    """This example script connects with the Message Server service of a SAP
    Netweaver Application Server and monitors the clients to identify new
    application servers. Similar to SAP's msprot tool.

    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host", help="Remote host")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int", help="Remote port [%default]", default=3900)
    target.add_option("--route-string", dest="route_string", help="Route string for connecting through a SAP Router")
    parser.add_option_group(target)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-c", "--client", dest="client", default="pysap's-observer", help="Client name [%default]")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Verbose output [%default]")
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

    # Initiate the connection
    conn = SAPRoutedStreamSocket.get_nisocket(options.remote_host,
                                              options.remote_port,
                                              options.route_string,
                                              base_cls=SAPMS)
    print("[*] Connected to the message server %s:%d" % (options.remote_host, options.remote_port))

    # Generate a random client string to differentiate our connection
    client_string = options.client

    # Send MS_LOGIN_2 packet
    print("[*] Sending login packet")
    p = SAPMS(flag=0x00, iflag=0x08, toname=client_string, fromname=client_string)
    response = conn.sr(p)[SAPMS]

    print("[*] Login performed, server string: %s" % response.fromname)
    server_string = response.fromname

    # Send MS_SERVER_CHG packet
    print("[*] Sending server change packet")
    p = SAPMS(flag=0x02, iflag=0x01, toname=server_string, fromname=client_string, opcode=0x01, opcode_version=4)
    response = conn.sr(p)[SAPMS]

    # Send MS_SERVER_LONG_LIST packet
    print("[*] Sending server long list packet")
    p = SAPMS(flag=0x01, iflag=0x01, toname=server_string, fromname=client_string, opcode=0x40, opcode_charset=0x00)
    conn.send(p)

    clients = []

    def print_client(msg, client):
        if options.verbose:
            print("[*] %s %s (host=%s, service=%s, port=%d)" % (msg,
                                                                client.client.strip(),
                                                                client.host.strip(),
                                                                client.service.strip(),
                                                                client.servno))

    # Send MS_SERVER_LST packet
    print("[*] Retrieving list of current clients")
    p = SAPMS(flag=0x02, iflag=0x01, toname=server_string, fromname=client_string, opcode=0x05, opcode_version=0x68)
    response = conn.sr(p)[SAPMS]
    for client in response.clients:
        if client.client != client_string:
            clients.append(("LIST", client))
            print_client("Client", client)

    try:
        while (True):
            response = conn.recv()[SAPMS]

            response.show()
            if response.opcode == 0x02:  # Added client
                client = response.clients[0]
                clients.append(("ADD", client))
                print_client("Added client", client)
            elif response.opcode == 0x03:  # Deleted client
                client = response.clients[0]
                clients.append(("DEL", client))
                print_client("Deleted client", client)
            elif response.opcode == 0x04:  # Modified client
                client = response.clients[0]
                clients.append(("MOD", client))
                print_client("Modified client", client)

    except SocketError:
        print("[*] Connection error")
    except KeyboardInterrupt:
        print("[*] Cancelled by the user")

    finally:
        print("[*] Observed clients:")
        for action, client in clients:
            print("\t%s\tclient %s (host=%s, service=%s, port=%d)" % (action,
                                                                      client.client.strip(),
                                                                      client.host.strip(),
                                                                      client.service.strip(),
                                                                      client.servno))

if __name__ == "__main__":
    main()
