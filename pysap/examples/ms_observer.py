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
from socket import error as SocketError
# External imports
from scapy.config import conf
# Custom imports
import pysap
from pysap.SAPRouter import SAPRoutedStreamSocket
from pysap.SAPMS import SAPMS, ms_domain_values_inv


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script connects with the Message Server service of a SAP Netweaver Application Server "\
                  "and monitors the clients to identify new application servers. Similar to SAP's msprot tool."

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3900,
                        help="Remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")
    target.add_argument("--domain", dest="domain", default="ABAP",
                        help="Domain to connect to (ABAP, J2EE or JSTARTUP) [%(default)s]")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")
    misc.add_argument("-c", "--client", dest="client", default="pysap's-observer",
                      help="Client name [%(default)s]")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")
    if options.domain not in ms_domain_values_inv.keys():
        parser.error("Invalid domain specified")

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    domain = ms_domain_values_inv[options.domain]

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
    p = SAPMS(flag=0x00, iflag=0x08, domain=domain, toname=client_string, fromname=client_string)
    response = conn.sr(p)[SAPMS]

    print("[*] Login performed, server string: %s" % response.fromname)
    server_string = response.fromname

    # Send MS_SERVER_CHG packet
    print("[*] Sending server change packet")
    p = SAPMS(flag=0x02, iflag=0x01, domain=domain, toname=server_string, fromname=client_string, opcode=0x01,
              opcode_version=4)
    response = conn.sr(p)[SAPMS]

    # Send MS_SERVER_LONG_LIST packet
    print("[*] Sending server long list packet")
    p = SAPMS(flag=0x01, iflag=0x01, domain=domain, toname=server_string, fromname=client_string, opcode=0x40,
              opcode_charset=0x00)
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
    p = SAPMS(flag=0x02, iflag=0x01, domain=domain, toname=server_string, fromname=client_string, opcode=0x05,
              opcode_version=0x68)
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
