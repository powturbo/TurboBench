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
from pysap.SAPNI import SAPNIStreamSocket, SAPNI
from pysap.SAPRouter import (SAPRouterRouteHop, get_router_version,
                             SAPRoutedStreamSocket, SAPRouteException,
                             SAPRouter,
                             ROUTER_TALK_MODE_NI_MSG_IO, ROUTER_TALK_MODE_NI_RAW_IO)
# Optional imports
try:
    import netaddr
except ImportError:
    print("[-] netaddr library not found, running without network range parsing support")
    netaddr = None


# Bind the SAPRouter layer
bind_layers(SAPNI, SAPRouter, )

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script performs a port scanning through a SAP Router service. Similar to Bizploit's " \
                  "'saprouterSpy', for more information check: " \
                  "https://blog.onapsis.com/blog/assessing-a-saprouters-security-with-onapsis-bizploit-part-i/ and " \
                  "https://blog.onapsis.com/blog/assessing-a-saprouters-security-with-onapsis-bizploit-part-ii/"

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host", default="127.0.0.1",
                        help="Remote host [%(default)s]")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3299,
                        help="Remote port [%(default)d]")
    target.add_argument("-t", "--target-hosts", dest="target_hosts",
                        help="Target hosts to scan (comma separated or CIDR if netaddr is installed)")
    target.add_argument("-r", "--target-ports", dest="target_ports",
                        help="Target ports to scan (comma separated or range)")
    target.add_argument("--router-version", dest="router_version", type=int,
                        help="SAP Router version to use [retrieve from the remote SAP Router]")
    target.add_argument("--talk-mode", dest="talk_mode", default="raw",
                        help="Talk mode to use when requesting the route (raw or ni) [%(default)s]")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")
    if not options.target_hosts:
        parser.error("Target hosts to scan are required")
    if not options.target_ports:
        parser.error("Target ports to scan are required")
    options.talk_mode = options.talk_mode.lower()
    if options.talk_mode not in ["raw", "ni"]:
        parser.error("Invalid talk mode")

    return options


def parse_target_ports(target_ports):
    ranges = (x.split("-") for x in target_ports.split(","))
    return [i for r in ranges for i in range(int(r[0]), int(r[-1]) + 1)]


def parse_target_hosts(target_hosts, target_ports):
    for port in parse_target_ports(target_ports):
        for host in target_hosts.split(','):
            if netaddr:
                if netaddr.valid_nmap_range(host):
                    for ip in netaddr.iter_nmap_range(host):
                        yield (ip, port)
                else:
                    for ip in netaddr.iter_unique_ips(host):
                        yield (ip, port)
            else:
                yield (host, port)


def route_test(rhost, rport, thost, tport, talk_mode, router_version):

    logging.info("[*] Routing connections to %s:%s" % (thost, tport))

    # Build the route to the target host passing through the SAP Router
    route = [SAPRouterRouteHop(hostname=rhost,
                               port=rport),
             SAPRouterRouteHop(hostname=thost,
                               port=tport)]

    # Try to connect to the target host using the routed stream socket
    try:
        conn = SAPRoutedStreamSocket.get_nisocket(route=route,
                                                  talk_mode=talk_mode,
                                                  router_version=router_version)
        conn.close()
        status = 'open'

    # If an SAPRouteException is raised, the route was denied or an error
    # occurred with the SAP router
    except SAPRouteException:
        status = 'denied'

    # Another error occurred on the server (e.g. timeout), mark the target as error
    except Exception:
        status = 'error'

    return status


# Main function
def main():
    options = parse_options()

    level = logging.INFO
    if options.verbose:
        level = logging.DEBUG
    logging.basicConfig(level=level, format='%(message)s')

    logging.info("[*] Connecting to SAP Router %s:%d (talk mode %s)" % (options.remote_host,
                                                                        options.remote_port,
                                                                        options.talk_mode))

    # Retrieve the router version used by the server if not specified
    if options.router_version is None:
        conn = SAPNIStreamSocket.get_nisocket(options.remote_host,
                                              options.remote_port,
                                              keep_alive=False)
        options.router_version = get_router_version(conn)
        conn.close()
    logging.info("[*] Using SAP Router version %d" % options.router_version)

    options.talk_mode = {"raw": ROUTER_TALK_MODE_NI_RAW_IO,
                         "ni": ROUTER_TALK_MODE_NI_MSG_IO}[options.talk_mode]

    results = []
    for (host, port) in parse_target_hosts(options.target_hosts, options.target_ports):
        status = route_test(options.remote_host, options.remote_port, host, port,
                            options.talk_mode, options.router_version)
        if options.verbose:
            logging.info("[*] Status of %s:%s: %s" % (host, port, status))
        if status == "open":
            results.append((host, port))

    logging.info("[*] Host/Ports found open:")
    for (host, port) in results:
        logging.info("\tHost: %s\tPort:%s" % (host, port))


if __name__ == "__main__":
    main()
