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
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI
from pysap.SAPRouter import (SAPRouter, SAPRouteException,
                             SAPRouterNativeProxy,
                             SAPRouterNativeRouterHandler)


# Bind the SAPRouter layer
bind_layers(SAPNI, SAPRouter, )

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = \
    """This example script routes a connection through a SAP Router service.

    Similar to Bizploit's 'saprouterNative', for more information check:
    * http://blog.onapsis.com/assessing-a-saprouters-security-with-onapsis-bizploit-part-i/
    * http://blog.onapsis.com/assessing-a-saprouters-security-with-onapsis-bizploit-part-ii/
    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host", help="Remote host [%default]", default="127.0.0.1")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int", help="Remote port [%default]", default=3299)
    target.add_option("-t", "--target-host", dest="target_host", help="Target host to connect")
    target.add_option("-r", "--target-port", dest="target_port", type="int", help="Target port to connect")
    target.add_option("-P", "--target-pass", dest="target_pass", help="Target password")
    target.add_option("-a", "--local-host", dest="local_host", help="Local host to listen [%default]", default="127.0.0.1")
    target.add_option("-l", "--local-port", dest="local_port", type="int", help="Local port to listen [target-port]")
    target.add_option("--talk-mode", dest="talk_mode", help="Talk mode to use when requesting the route (raw or ni) [%default]", default="raw")
    parser.add_option_group(target)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Verbose output [%default]")
    parser.add_option_group(misc)

    (options, _) = parser.parse_args()

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

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    print("[*] Setting a proxy between %s:%d and remote SAP Router %s:%d (talk mode %s)" % (options.local_host,
                                                                                            options.local_port,
                                                                                            options.remote_host,
                                                                                            options.remote_port,
                                                                                            options.talk_mode))

    options.talk_mode = {"raw": 1, "ni": 0}[options.talk_mode]

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
                print("[*] Socket Error %s" % e)

    except KeyboardInterrupt:
        print("[*] Cancelled by the user !")
    except SAPRouteException as e:
        print("[*] Closing routing do to error %s" % e)


if __name__ == "__main__":
    main()
