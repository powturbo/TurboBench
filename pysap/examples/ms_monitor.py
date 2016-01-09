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
from pysap.SAPMS import (SAPMS, ms_client_status_values, ms_opcode_error_values,
                         ms_dump_command_values, SAPMSCounter, ms_opcode_values,
                         ms_errorno_values, SAPMSProperty, ms_property_id_values,
                         SAPMSAdmRecord)
from pysap.SAPRouter import SAPRoutedStreamSocket


# Set the verbosity to 0
conf.verb = 0


class SAPMSMonitorConsole(BaseConsole):

    intro = "SAP MS Monitor Console"
    connection = None
    connected = False
    clients = []

    def __init__(self, options):
        super(SAPMSMonitorConsole, self).__init__(options)
        self.runtimeoptions["client_string"] = self.options.client

    # Initialization
    def preloop(self):
        super(SAPMSMonitorConsole, self).preloop()
        self.do_connect(None)
        self.do_client_list(None)

    # Helper for crafting packets
    def _build(self, flag, iflag, **args):
        return SAPMS(flag=flag, iflag=iflag,
                     toname=self.runtimeoptions["server_string"],
                     fromname=self.runtimeoptions["client_string"],
                     **args)

    # Helper for sending simple commands and opcodes
    def _send_simple(self, flag, iflag, **args):
        if not self.connected:
            self._error("You need to connect to the server first !")
            return

        if "opcode" in args:
            opcode_name = ms_opcode_values[args["opcode"]] + " "
        else:
            opcode_name = ""

        p = self._build(flag, iflag, **args)

        self._debug("Sending %spacket" % opcode_name)
        response = self.connection.sr(p)[SAPMS]

        if response.opcode_error != 0:
            self._print("Error: %s" % ms_opcode_error_values[response.opcode_error])
            return None
        else:
            return response

    # SAP MS Monitor commands

    def do_connect(self, args):
        """ Initiate the connection to the Message Server service. The
        connection is registered using the client_string runtime option. """

        # Create the socket connection
        try:
            self.connection = SAPRoutedStreamSocket.get_nisocket(self.options.remote_host,
                                                                 self.options.remote_port,
                                                                 self.options.route_string,
                                                                 base_cls=SAPMS)
        except SocketError as e:
            self._error("Error connecting with the Message Server")
            self._error(str(e))
            return

        self._print("Attached to %s / %d" % (self.options.remote_host, self.options.remote_port))

        # Send MS_LOGIN_2 packet
        p = SAPMS(flag=0x00, iflag=0x08, toname=self.runtimeoptions["client_string"], fromname=self.runtimeoptions["client_string"])

        self._debug("Sending login packet")
        response = self.connection.sr(p)[SAPMS]

        if response.errorno == 0:
            self.runtimeoptions["server_string"] = response.fromname
            self._debug("Login performed, server string: %s" % response.fromname)
            self._print("pysap's Message Server monitor, connected to %s / %d" % (self.options.remote_host, self.options.remote_port))
            self.connected = True
        else:
            if response.errorno in ms_errorno_values:
                self._error("Error performing login: %s" % ms_errorno_values[response.errorno])
            else:
                self._error("Unknown error performing login: %d" % response.errorno)

    def do_disconnect(self, args):
        """ Disconnects from the Message Server service. """

        if not self.connected:
            self._error("You need to connect to the server first !")
            return

        # Send MS_LOGOUT packet
        p = self._build(0x00, 0x04)
        self._debug("Sending logout packet")
        self.connection.send(p)

        self.connection.close()
        self._print("Dettached from %s / %d ..." % (self.options.remote_host, self.options.remote_port))

        self.connected = False

    def do_exit(self, args):
        if self.connected:
            self.do_disconnect(None)
        return super(SAPMSMonitorConsole, self).do_exit(args)

    def do_client_list(self, args):
        """ Retrieve the list of clients connected to the Message Server
        service. Use the client # value when required to provide a client
        IDs as parameter. """

        if not self.connected:
            self._error("You need to connect to the server first !")
            return

        # Send MS_SERVER_LONG_LIST packet
        p = self._build(0x01, 0x01, opcode=0x40, opcode_charset=0x00)
        self._debug("Sending server long list packet")
        self.connection.send(p)

        # Send MS_SERVER_LST packet
        response = self._send_simple(0x02, 0x01, opcode=0x05, opcode_version=0x68)

        # Print clients table
        table = [["#", "Client Name", "Host", "Service", "IPv4", "IPv6", "ServNo", "State", "Services"]]
        instance = self.runtimeoptions["server_string"]
        i = 0
        for client in response.clients:
            if client.status == 1:
                instance = client.client
            table.append([str(i),
                          client.client,
                          client.host,
                          client.service,
                          client.hostaddrv4,
                          client.hostaddrv6 if "hostaddrv6" in client.fields else None,
                          str(client.servno),
                          ms_client_status_values[client.status]])
            i += 1
        self._tabulate(table)

        # Store clients for further use
        self.clients = response.clients

        self._debug("Server instance: %s" % instance)
        self.runtimeoptions["instance"] = instance

    def do_hardware_id(self, args):
        """ Retrieve the installation's hardware ID. """
        response = self._send_simple(0x02, 0x01, opcode=0x0a)
        if response:
            self._print("Hardware ID: %s" % response.hwid)

    def do_get_security_by_name(self, args):
        """ Get Security Key by name. """
        response = self._send_simple(0x02, 0x01, opcode=0x08, security_name=args)
        if response:
            self._print("Security Name: %s" % response.security_name)
            self._print("Security Key: %s" % response.security_key)

    def do_get_security_by_ip(self, args):
        """ Get Security Key by ip/port. Options <IPv4 address> <port> """

        try:
            ip, port = args.split(" ")
            port = int(port)
        except ValueError:
            self._error("Wrong parameters !")
            return

        # Send MS_GET_SECURITY
        response = self._send_simple(0x02, 0x01, opcode=0x09, security2_addressv4=ip, security2_port=port)
        if response:
            self._print("Security IPv4 Address: %s" % response.security2_addressv4)
            self._print("Security Port: %s" % response.security2_port)
            self._print("Security Key: %s" % response.security2_key)
            self._print("Security IPv6 Address: %s" % response.security2_addressv6)

    def do_dump(self, args):
        """ Dump information. Options [<dump command> | all] """

        if args == "all":
            for key in ms_dump_command_values:
                self.do_dump(key)
            return

        # Split list of commands
        try:
            args = args.split(" ")
        except AttributeError:
            args = [args]

        try:
            command = int(args[0])
        except ValueError:
            self._error("Wrong dump command ! Valid values:")
            for key in ms_dump_command_values:
                self._error("%d: %s" % (key, ms_dump_command_values[key]))
            self._error("all: dumps all the available information")
            return

        if command == 1:  # MS_DUMP_MSADM
            try:
                client_id = int(args[1])
                client = self.clients[client_id]
            except (ValueError, KeyError, IndexError):
                self._error("Wrong parameters ! Specify client ID")
                return
            response = self._send_simple(0x02, 0x01, opcode=0x1e,
                                         dump_dest=0x02, dump_command=command,
                                         dump_name=client.client)
        elif command == 12:  # MS_DUMP_COUNTER
            try:
                counter = args[1]
            except IndexError:
                self._error("Wrong parameters ! Specify counter number")
                return
            response = self._send_simple(0x02, 0x01, opcode=0x1e,
                                         dump_dest=0x02, dump_command=command,
                                         dump_name=counter)
        else:
            # Send MS_DUMP_INFO
            response = self._send_simple(0x02, 0x01, opcode=0x1e, dump_dest=0x02, dump_command=command)

        if response:
            self._print("Dump information:\n%s" % response.opcode_value)

    def do_server_parameters(self, args):
        """ Dump server parameters. """
        self.do_dump("3")

    def do_server_release(self, args):
        """ Dump release information. """
        self.do_dump("8")

    def do_trace_increase(self, args):
        """ Increase server's trace level. """

        # Send MS_INCRE_TRACE
        response = self._send_simple(0x02, 0x01, opcode=0x0b)
        if response:
            self._print("Trace increased")

    def do_trace_decrease(self, args):
        """ Decrease server's trace level. """

        # Send MS_DECRE_TRACE
        response = self._send_simple(0x02, 0x01, opcode=0x0c)
        if response:
            self._print("Trace decreased")

    def do_trace_reset(self, args):
        """ Reset server's trace level. """

        # Send MS_RESET_TRACE
        response = self._send_simple(0x02, 0x01, opcode=0x0d)
        if response:
            self._print("Trace reset")

    def do_statistics_activate(self, args):
        """ Activates server's statistics. """

        # Send MS_ACT_STATISTIC
        response = self._send_simple(0x02, 0x01, opcode=0x0e)
        if response:
            self._print("Statistics activated")

    def do_statistics_deactivate(self, args):
        """ Deactivates server's statistics. """

        # Send MS_DEACT_STATISTIC
        response = self._send_simple(0x02, 0x01, opcode=0x0f)
        if response:
            self._print("Statistics deactivated")

    def do_statistics_reset(self, args):
        """ Reset server's statistics. """

        # Send MS_RESET_STATISTIC
        response = self._send_simple(0x02, 0x01, opcode=0x10)
        if response:
            self._print("Statistics reset")

    def do_statistics_get(self, args):
        """ Get server's statistics. """

        # Send MS_GET_STATISTIC
        response = self._send_simple(0x02, 0x01, opcode=0x11)
        # TODO: Statistics fields are not correctly defined, just showing the
        # complete packet now
        if response:
            response.show()

    def do_network_buffer_dump(self, args):
        """ Dump network buffer. """

        # Send MS_DUMP_NIBUFFER
        response = self._send_simple(0x02, 0x01, opcode=0x12)
        if response:
            self._print("Network buffer dumped")

    def do_network_buffer_reset(self, args):
        """ Reset network buffer. """

        # Send MS_RESET_NIBUFFER
        response = self._send_simple(0x02, 0x01, opcode=0x13)
        if response:
            self._print("Network buffer reset")

    def do_get_codepage(self, args):
        """ Get code page. """

        # Send MS_GET_CODEPAGE
        response = self._send_simple(0x02, 0x01, opcode=0x1c)
        if response:
            self._print("Codepage: %s" % response.codepage)

    def do_counter_list(self, args):
        """ List Counters. """

        # Send MS_COUNTER_LST
        response = self._send_simple(0x02, 0x01, opcode=0x2a)
        if response:
            self._print("Counters:\n%s" % response.counters)

    def _counter_opcodes(self, counter, opcode, count=0, number=0):
        """ Helper for counter commands. """

        c = SAPMSCounter(uuid=counter, count=count, no=number)
        response = self._send_simple(0x02, 0x01, opcode=opcode, counter=c)
        if response and response.counter:
            self._print("Counter UUID: %s Count: %d No: %d" % (response.counter.uuid,
                                                               response.counter.count,
                                                               response.counter.no))

    def do_counter_get(self, args):
        """ Get Counter. Options: <counter> """
        self._counter_opcodes(args, 0x29)

    def do_counter_create(self, args):
        """ Create Counter. Options: <counter> """
        self._counter_opcodes(args, 0x24)

    def do_counter_delete(self, args):
        """ Delete Counter. Options: <counter> """
        self._counter_opcodes(args, 0x25)

    def do_counter_increment(self, args):
        """ Increment Counter. Options: <counter> <value> """
        try:
            counter, count = args.split(" ")
            count = int(count)
        except ValueError:
            self._error("Invalid parameters !")
            return

        self._counter_opcodes(counter, 0x26, count=count)

    def do_counter_decrement(self, args):
        """ Decrement Counter. Options: <counter> <value> """
        try:
            counter, count = args.split(" ")
            count = int(count)
        except ValueError:
            self._error("Invalid parameters !")
            return

        self._counter_opcodes(counter, 0x27, count=count)

    def do_counter_register(self, args):
        """ Register Counter. Options: <counter> """
        self._counter_opcodes(args, 0x28)

    def do_server_disconnect(self, args):
        """ Server disconnect. Options: <client id> <reason> """

        try:
            client_id, reason = args.split(" ", 2)
            client_id = int(client_id)
            client = self.clients[client_id]
        except (ValueError, KeyError):
            self._error("Invalid parameters !")
            self.do_client_list(None)
            return

        # Send MS_SERVER_DISC packet
        response = self._send_simple(0x02, 0x01, opcode=0x2e,
                                     shutdown_client=client,
                                     shutdown_reason=reason)
        if response:
            self._print("Disconnected from server")

    def do_server_shutdown(self, args):
        """ Server shutdown. Options: <client id> <reason> """

        try:
            client_id, reason = args.split(" ", 2)
            client_id = int(client_id)
            client = self.clients[client_id]
        except (ValueError, KeyError):
            self._error("Invalid parameters !")
            self.do_client_list(None)
            return

        # Send MS_SERVER_SHUTDOWN packet
        response = self._send_simple(0x02, 0x01, opcode=0x2f,
                                     shutdown_client=client,
                                     shutdown_reason=reason)
        if response:
            self._print("Server shutdown")

    def do_server_soft_shutdown(self, args):
        """ Server soft shutdown. Options: <client id> <reason> """

        try:
            client_id, reason = args.split(" ", 2)
            client_id = int(client_id)
            client = self.clients[client_id]
        except (ValueError, KeyError):
            self._error("Invalid parameters !")
            self.do_client_list(None)
            return

        # Send MS_SERVER_SOFT_SHUTDOWN packet
        response = self._send_simple(0x02, 0x01, opcode=0x30,
                                     shutdown_client=client,
                                     shutdown_reason=reason)
        if response:
            self._print("Server soft shutdown")

    def do_property_get(self, args):
        """ Get property. Options: <client id> <prop id> """

        try:
            prop_client, prop_id = args.split(" ")
            prop_client = self.clients[int(prop_client)]
            prop_id = int(prop_id)
        except (ValueError, KeyError):
            self._error("Invalid parameters !")
            return

        # Send MS_GET_PROPERTY packet
        prop = SAPMSProperty(client=prop_client.client,
                             id=prop_id)
        response = self._send_simple(0x02, 0x01, opcode=0x44,
                                     property=prop)
        if response:
            self._print("Property %s for client %s:" % (ms_property_id_values[prop_id],
                                                        prop_client.client.strip()))
            response.property.show()

    def do_parameter_get(self, args):
        """ Get parameter value. Options: <parameter name> """

        parameter_name = args

        # Send ADM AD_PROFILE request
        adm = SAPMSAdmRecord(opcode=0x1, parameter=parameter_name)
        p = self._build(0x04, 0x05, adm_records=[adm])

        response = self.connection.sr(p)[SAPMS]

        if response:
            self._print("Parameter value: %s" % (response.adm_records[0].parameter))

    def do_parameter_set(self, args):
        """ Set parameter value (requires monitor mode enabled).
            Options: <parameter name> <parameter value> """

        try:
            parameter_name, parameter_value = args.split(" ", 2)
        except ValueError:
            self._error("Invalid parameters !")
            return

        # Send ADM AD_SHARED_PARAMETER request
        adm = SAPMSAdmRecord(opcode=0x2e,
                             parameter="%s=%s" % (parameter_name,
                                                  parameter_value))
        p = self._build(0x04, 0x05, adm_records=[adm])

        response = self.connection.sr(p)[SAPMS]

        if response:
            if response.adm_records[0].errorno != 0:
                self._error("Error changing the parameter !")
            else:
                self._print("Parameter %s set to %s !" % (parameter_name,
                                                          parameter_value))


# Command line options parser
def parse_options():

    description = \
    """This script is an example implementation of SAP's Message Server Monitor
    program (msmon). It allows the monitoring of a Message Server service and
    allows sending different commands and opcodes. Includes some commands not
    available on the msmon program. Some commands requires the server running
    in monitor mode, the most requires access to the Message Server internal
    port.

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

    ms_console = SAPMSMonitorConsole(options)

    try:
        if options.script:
            ms_console.do_script(options.script)
        else:
            ms_console.cmdloop()
    except KeyboardInterrupt:
        print("Cancelled by the user !")
        ms_console.do_exit(None)


if __name__ == "__main__":
    main()
