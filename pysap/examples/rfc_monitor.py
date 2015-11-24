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
from pysap.utils import BaseConsole
from pysap.SAPRFC import SAPRFC
from pysap.SAPRouter import SAPRoutedStreamSocket


# Set the verbosity to 0
conf.verb = 0


class SAPRFCMonitorConsole(BaseConsole):

    intro = "SAP Gateway/RFC Monitor Console"
    connection = None
    connected = False
    clients = []

    def __init__(self, options):
        super(SAPRFCMonitorConsole, self).__init__(options)
        self.runtimeoptions["client_string"] = self.options.client

    # Initialization
    def preloop(self):
        super(SAPRFCMonitorConsole, self).preloop()
        self.do_connect(None)
        self.do_client_list(None)

    # SAP Gateway/RFC Monitor commands

    def do_connect(self, args):
        """ Initiate the connection to the Gateway service. The connection is
        registered using the client_string runtime option. """

        # Create the socket connection
        try:
            self.connection = SAPRoutedStreamSocket.get_nisocket(self.options.remote_host,
                                                                 self.options.remote_port,
                                                                 self.options.route_string,
                                                                 base_cls=SAPRFC)
        except SocketError as e:
            self._error("Error connecting with the Gateway service")
            self._error(str(e))
            return

        self._print("Attached to %s / %d" % (self.options.remote_host, self.options.remote_port))

    def do_disconnect(self, args):
        """ Disconnects from the Gateway service. """

        if not self.connected:
            self._error("You need to connect to the server first !")
            return

        self.connection.close()
        self._print("Dettached from %s / %d ..." % (self.options.remote_host, self.options.remote_port))
        self.connected = False

    def do_exit(self, args):
        if self.connected:
            self.do_disconnect(None)
        return super(SAPRFCMonitorConsole, self).do_exit(args)

    def do_client_list(self, args):
        """ Retrieve the list of clients connected to the Gateway service.
        Use the client # value when required to provide a client IDs as
        parameter. """

        if not self.connected:
            self._error("You need to connect to the server first !")
            return


# Command line options parser
def parse_options():

    description = \
    """This script is an example implementation of SAP's Gateway Monitor
    program (gwmon). It allows the monitoring of a Gateway service and allows
    sending different commands and opcodes.

    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host", help="Remote host")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int", help="Remote port [%default]", default=3300)
    target.add_option("--route-string", dest="route_string", help="Route string for connecting through a SAP Router")
    parser.add_option_group(target)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-c", "--client", dest="client", default="pysap's-monitor", help="Client name [%default]")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Verbose output [%default]")
    misc.add_option("--log-file", dest="logfile", help="Log file")
    misc.add_option("--console-log", dest="consolelog", help="Console log file")
    misc.add_option("--script", dest="script", help="Script file to run")
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

    rfc_console = SAPRFCMonitorConsole(options)

    try:
        if options.script:
            rfc_console.do_script(options.script)
        else:
            rfc_console.cmdloop()
    except KeyboardInterrupt:
        print("Cancelled by the user !")
        rfc_console.do_exit(None)


if __name__ == "__main__":
    main()
