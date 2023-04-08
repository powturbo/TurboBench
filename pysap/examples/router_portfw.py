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
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI
from pysap.SAPRouter import (SAPRouter, SAPRouteException,
                             SAPRouterNativeProxy,
                             SAPRouterNativeRouterHandler,
                             SAPRouterRouteHop,
                             ROUTER_TALK_MODE_NI_RAW_IO, ROUTER_TALK_MODE_NI_MSG_IO)


# Bind the SAPRouter layer
bind_layers(SAPNI, SAPRouter, )

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script routes a connection through a SAP Router service. Similar to Bizploit's " \
                  "'saprouterNative', for more information check: " \
                  "https://blog.onapsis.com/blog/assessing-a-saprouters-security-with-onapsis-bizploit-part-i/ and "\
                  "https://blog.onapsis.com/blog/assessing-a-saprouters-security-with-onapsis-bizploit-part-ii/"

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host", default="127.0.0.1",
                        help="Remote host [%(default)s]")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3299,
                        help="Remote port [%(default)d]")
    target.add_argument("-t", "--target-host", dest="target_host",
                        help="Target host to connect")
    target.add_argument("-r", "--target-port", dest="target_port", type=int,
                        help="Target port to connect")
    target.add_argument("-P", "--target-pass", dest="target_pass",
                        help="Target password")
    target.add_argument("-a", "--local-host", dest="local_host", default="127.0.0.1",
                        help="Local host to listen [%(default)s]")
    target.add_argument("-l", "--local-port", dest="local_port", type=int,
                        help="Local port to listen [target-port]")
    target.add_argument("--talk-mode", dest="talk_mode", default="raw",
                        help="Talk mode to use when requesting the route (raw or ni) [%(default)s]")

    target.add_argument("--route-string", dest="target_route_string",
                        help="Route String for connecting through a SAP Router")
    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

    if options.target_route_string and ( options.remote_host or options.target_host):
        print("[!] Route String specified, Remote and Target host ignored")
        route = SAPRouterRouteHop.from_string(options.target_route_string)
        options.remote_host = route[0].hostname
        options.remote_port = 3299
        if route[0].port and route[0].port.isdigit(): options.remote_port = int(route[0].port)
        options.target_host = route[-1].hostname
        options.target_port = 3299
        if route[-1].port and route[-1].port.isdigit(): options.target_port = int(route[-1].port)
        del route
    if not options.remote_host:
        parser.error("Remote host is required")
    if not options.target_host:
        parser.error("Target host to connect to is required")
    if not options.target_port:
        parser.error("Target port to connect to is required")
    options.talk_mode = options.talk_mode.lower()
    if options.talk_mode not in ["raw", "ni"]:
        parser.error("Invalid talk mode")

    if not options.local_port:
        print("[*] No local port specified, using target port %d" % options.target_port)
        options.local_port = options.target_port

    return options


# Main function
def main():
    options = parse_options()

    level = logging.INFO
    if options.verbose:
        level = logging.DEBUG
    logging.basicConfig(level=level, format='%(message)s')

    logging.info("[*] Setting a proxy between %s:%d and remote SAP Router %s:%d (talk mode %s)" % (options.local_host,
                                                                                                   options.local_port,
                                                                                                   options.remote_host,
                                                                                                   options.remote_port,
                                                                                                   options.talk_mode))

    if options.target_route_string:
    	logging.info("[*] using Route String %s" % (options.target_route_string))
    options.talk_mode = {"raw": ROUTER_TALK_MODE_NI_RAW_IO, "ni": ROUTER_TALK_MODE_NI_MSG_IO}[options.talk_mode]

    proxy = SAPRouterNativeProxy(options.local_host, options.local_port,
                                 options.remote_host, options.remote_port,
                                 SAPRouterNativeRouterHandler,
                                 target_address=options.target_host,
                                 target_port=options.target_port,
                                 target_pass=options.target_pass,
                                 talk_mode=options.talk_mode,
                                 keep_alive=False,
                                 options=options)

    try:
        while True:
            try:
                proxy.handle_connection()
            except SocketError as e:
                logging.error("[*] Socket Error %s" % e)

    except KeyboardInterrupt:
        logging.error("[*] Cancelled by the user !")
    except SAPRouteException as e:
        logging.error("[*] Closing routing do to error %s" % e)


if __name__ == "__main__":
    main()
