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
from scapy.packet import Raw
# Custom imports
import pysap
from pysap.SAPRouter import SAPRoutedStreamSocket
from pysap.SAPMS import SAPMS, ms_domain_values_inv


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script connects with the Message Server service and sends a message to another client."

    usage = "%(prog)s [options] -d <remote host> -t <target server> -m <message>"

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
    misc.add_argument("-c", "--client", dest="client", default="pysap's-messager",
                      help="Client name [%(default)s]")
    misc.add_argument("-m", "--message", dest="message", default="Message",
                      help="Message to send to the target client [%(default)s]")
    misc.add_argument("-t", "--target", dest="target", default="pysap's-listener",
                      help="Target client name to send the message [%(default)s]")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")
    if not options.message or not options.target:
        parser.error("Target server and message are required !")
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

    client_string = options.client

    # Send MS_LOGIN_2 packet
    p = SAPMS(flag=0x00, iflag=0x08, domain=domain, toname=client_string, fromname=client_string)

    print("[*] Sending login packet")
    response = conn.sr(p)[SAPMS]

    print("[*] Login performed, server string: %s" % response.fromname)

    # Sends a message to another client
    p = SAPMS(flag=0x02, iflag=0x01, domain=domain, toname=options.target, fromname=client_string, opcode=1)
    p /= Raw(options.message)

    print("[*] Sending packet to: %s" % options.target)
    conn.send(p)


if __name__ == "__main__":
    main()
