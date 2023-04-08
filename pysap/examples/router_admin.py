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
from socket import error
from argparse import ArgumentParser
# External imports
from scapy.config import conf
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI, SAPNIStreamSocket
from pysap.utils.fields import saptimestamp_to_datetime
from pysap.SAPRouter import (SAPRouter, router_is_error, get_router_version,
                             SAPRouterInfoClients, SAPRouterInfoServer)
# Optional imports
try:
    from tabulate import tabulate
except ImportError:
    tabulate = None


# Bind the SAPRouter layer
bind_layers(SAPNI, SAPRouter, )

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script connects with a SAP Router server and allows perform administrative tasks. " \
                  "The options are the ones found on the regular SAP Router tool with the addition of some " \
                  "undocumented operation codes."

    usage = "%(prog)s [options] [command] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host", default="127.0.0.1",
                        help="Remote host [%(default)s]")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3299,
                        help="Remote port [%(default)s]")
    target.add_argument("--router-version", dest="router_version", type=int,
                        help="SAP Router version to use [retrieve from the remote SAP Router]")

    command = parser.add_mutually_exclusive_group(required=True)
    command.add_argument("-s", "--stop-router", dest="stop", action="store_true",
                         help="Stop router request")
    command.add_argument("-o", "--soft-shutdown", dest="soft", action="store_true",
                         help="Soft shutdown request")
    command.add_argument("-l", "--router-info", dest="info", action="store_true",
                         help="Router info request")
    command.add_argument("-P", "--info-pass", dest="info_password", action="store",
                         help="Password for info request")
    command.add_argument("-n", "--new-route", dest="new_route", action="store_true",
                         help="New route table request")
    command.add_argument("-t", "--toggle-trace", dest="trace", action="store_true",
                         help="Toggle trace request")
    command.add_argument("-c", "--cancel-route", dest="cancel", action="store",
                         help="Cancel route request")
    command.add_argument("-b", "--dump-buffer", dest="dump", action="store_true",
                         help="Dump buffers request")
    command.add_argument("-f", "--flush-buffer", dest="flush", action="store_true",
                         help="Flush buffers request")
    command.add_argument("-z", "--hide-errors", dest="hide", action="store_true",
                         help="Hide errors info")
    command.add_argument("--set-peer-trace", dest="set_peer", action="store",
                         help="Set peer trace (undocumented command)")
    command.add_argument("--clear-peer-trace", dest="clear_peer", action="store",
                         help="Clear peer trace (undocumented command)")
    command.add_argument("--trace-connection", dest="trace_conn", action="store",
                         help="Trace connection (undocumented command)")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")

    return options


def print_table(clients):
    """Prints the client table"""
    if tabulate:
        logging.info(tabulate(clients))
    else:
        logging.info("\n".join("\t| ".join([str(col).strip() for col in line]).expandtabs(20) for line in clients))


# Main function
def main():
    options = parse_options()

    level = logging.INFO
    if options.verbose:
        level = logging.DEBUG
    logging.basicConfig(level=level, format='%(message)s')

    response = False

    p = SAPRouter(type=SAPRouter.SAPROUTER_ADMIN)

    if options.stop:
        p.adm_command = 5
        logging.info("[*] Requesting stop of the remote SAP Router")

    elif options.soft:
        p.adm_command = 9
        logging.info("[*] Requesting a soft shutdown of the remote SAP Router")
        response = True

    elif options.info:
        p.adm_command = 2
        if options.info_password:
            if len(options.info_password) > 19:
                logging.info("[*] Password too long, truncated at 19 characters")
            p.adm_password = options.info_password
            logging.info("[*] Requesting info using password %s" % p.adm_password)
        else:
            logging.info("[*] Requesting info")
        response = True

    elif options.new_route:
        p.adm_command = 3
        logging.info("[*] Requesting a refresh of the router table")

    elif options.trace:
        p.adm_command = 4
        logging.info("[*] Requesting a toggle on the trace settings")

    elif options.cancel:
        p.adm_command = 6
        p.adm_client_ids = list(map(int, options.cancel.split(",")))
        logging.info("[*] Requesting a cancel of the route(s) with client id(s) %s" % p.adm_client_ids)
        response = True

    elif options.dump:
        p.adm_command = 7
        logging.info("[*] Requesting a dump of the buffers")

    elif options.flush:
        p.adm_command = 8
        logging.info("[*] Requesting a flush of the buffers")

    elif options.hide:
        p.adm_command = 14
        logging.info("[*] Requesting a hide on the errors to clients")
        response = True

    elif options.set_peer:
        p.adm_command = 10
        p.adm_address_mask = options.set_peer
        logging.info("[*] Request a set peer trace for the address mask %s" % p.adm_address_mask)
        response = True

    elif options.clear_peer:
        p.adm_command = 11
        p.adm_address_mask = options.clear_peer
        logging.info("[*] Request a clear peer trace for the address mask %s" % p.adm_address_mask)
        response = True

    elif options.trace_conn:
        p.adm_command = 12
        p.adm_client_ids = list(map(int, options.trace_conn.split(",")))
        logging.info("[*] Requesting a connection trace with client id(s) %s" % p.adm_client_ids)
        response = True

    else:
        logging.error("[*] No command specified !")
        return

    # Initiate the connection
    conn = SAPNIStreamSocket.get_nisocket(options.remote_host, options.remote_port)
    logging.info("[*] Connected to the SAP Router %s:%d" % (options.remote_host, options.remote_port))

    # Retrieve the router version used by the server if not specified
    if options.router_version:
        p.version = options.router_version
    else:
        p.version = get_router_version(conn) or p.version
    logging.info("[*] Using SAP Router version %d" % p.version)

    # Send the router admin request
    logging.info("[*] Sending Router Admin packet")
    if options.verbose:
        p.show2()
    conn.send(p)

    # Grab the response if required
    if response:

        # Some responses has no SAPRouter's packet format and are raw strings,
        # we need to get the SAP NI layer first and then check if we could go
        # down to the SAPRouter layer.
        raw_response = conn.recv()[SAPNI]
        if SAPRouter in raw_response:
            router_response = raw_response[SAPRouter]

        # If the response was null, just return
        elif raw_response.length == 0:
            return

        # If the response is an error, print and exit
        if router_is_error(router_response):
            logging.info("[*] Error requesting info:")
            if options.verbose:
                router_response.show2()
            else:
                logging.error(router_response.err_text_value.error)

        # Otherwise, print the packets sent by the SAP Router
        else:
            logging.info("[*] Response:\n")

            if options.info:
                # Decode the first packet as a list of info client
                raw_response.decode_payload_as(SAPRouterInfoClients)

                clients = [["ID", "Client", "Partner", "Service", "Connected on"]]
                for client in raw_response.clients:

                    # If the trace flag is set, add a mark
                    flag = "(*)" if client.flag_traced else "(+)" if client.flag_routed else ""

                    fields = [str(client.id),
                              client.address,
                              "%s%s" % (flag, client.partner) if client.flag_routed else "(no partner)",
                              client.service if client.flag_routed else "",
                              saptimestamp_to_datetime(client.connected_on).ctime()]
                    clients.append(fields)

                # Decode the second packet as server info
                raw_response = conn.recv()
                raw_response.decode_payload_as(SAPRouterInfoServer)

                logging.info("SAP Network Interface Router running on port %d (PID = %d)\n"
                             "Started on: %s\n"
                             "Parent process: PID = %d, port = %d\n" % (raw_response.port, raw_response.pid,
                                                                        saptimestamp_to_datetime(raw_response.started_on).ctime(),
                                                                        raw_response.ppid, raw_response.pport))
                print_table(clients)
                logging.info("(*) Connections being traced")

            # Show the plain packets returned
            try:
                raw_response = conn.recv()
                while raw_response:
                    logging.info(raw_response.payload)
                    raw_response = conn.recv()
            except error:
                pass


if __name__ == "__main__":
    main()
