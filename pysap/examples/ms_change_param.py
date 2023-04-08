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
# Custom imports
import pysap
from pysap.SAPRouter import SAPRoutedStreamSocket
from pysap.SAPMS import SAPMS, SAPMSAdmRecord, ms_domain_values_inv


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script changes a parameter using Message Server Administration requests. In order to " \
                  "be able to change a parameter the Message Server should be configured in monitoring mode " \
                  "(ms/monitor=1)[1] and the internal port should be reachable. Keep in mind that some of the " \
                  "parameters are not dynamic and can't be changed using this method. If the parameter value is not " \
                  "specified, the script retrieve the current value. " \
                  "[1] https://help.sap.com/saphelp_nw70/helpdata/en/4e/cffdb69d10424e97eb1d993b1e2cfd/content.htm"

    usage = "%(prog)s [options] -d <remote host> -n <parameter name> [-l <parameter value>]"

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

    param = parser.add_argument_group("Parameter")
    param.add_argument("-n", "--parameter-name", dest="param_name",
                       help="Parameter name")
    param.add_argument("-l", "--parameter-value", dest="param_value",
                       help="Parameter value")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")
    misc.add_argument("-c", "--client", dest="client", default="pysap's-paramchanger",
                      help="Client name [%(default)s]")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")
    if not options.param_name:
        parser.error("Parameter name is required")
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

    # Build MS_LOGIN_2 packet
    p = SAPMS(flag=0x00, iflag=0x08, domain=domain, toname=client_string, fromname=client_string)

    # Send MS_LOGIN_2 packet
    print("[*] Sending login packet")
    response = conn.sr(p)[SAPMS]

    print("[*] Login performed, server string: %s" % response.fromname)
    server_string = response.fromname

    print("[*] Retrieving current value of parameter: %s" % options.param_name)

    # Send ADM AD_PROFILE request
    adm = SAPMSAdmRecord(opcode=0x1, parameter=options.param_name)
    p = SAPMS(toname=server_string, fromname=client_string, version=4,
              flag=0x04, iflag=0x05, domain=domain, adm_records=[adm])

    print("[*] Sending packet")
    response = conn.sr(p)[SAPMS]

    if options.verbose:
        print("[*] Response:")
        response.show()

    param_old_value = response.adm_records[0].parameter
    print("[*] Parameter %s" % param_old_value)

    # If a parameter change was requested, send an ADM AD_SHARED_PARAMETER request
    if options.param_value:
        print("[*] Changing parameter value from: %s to: %s" % (param_old_value,
                                                                options.param_value))

        # Build the packet
        adm = SAPMSAdmRecord(opcode=0x2e,
                             parameter="%s=%s" % (options.param_name,
                                                  options.param_value))
        p = SAPMS(toname=server_string, fromname=client_string, version=4,
                  iflag=5, flag=4, domain=domain, adm_records=[adm])

        # Send the packet
        print("[*] Sending packet")
        response = conn.sr(p)[SAPMS]

        if options.verbose:
            print("[*] Response:")
            response.show()

        if response.adm_records[0].errorno != 0:
            print("[*] Error requesting parameter change (error number %d)" % response.adm_records[0].errorno)
        else:
            print("[*] Parameter changed for the current session !")


if __name__ == "__main__":
    main()
