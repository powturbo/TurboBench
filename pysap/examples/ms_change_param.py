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
from optparse import OptionParser, OptionGroup
# External imports
from scapy.config import conf
# Custom imports
import pysap
from pysap.SAPMS import SAPMS, SAPMSAdmRecord
from pysap.SAPRouter import SAPRoutedStreamSocket


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = \
    """This example script changes a parameter using Message Server
    Administration requests. In order to be able to change a parameter the
    Message Server should be configured in monitoring mode (ms/monitor=1)[1]
    and the internal port should be reachable. Keep in mind that some of the
    parameters are not dynamic and can't be changed using this method.

    If the parameter value is not specified, the script retrieve the current
    value.

    [1] http://help.sap.com/saphelp_nw70/helpdata/en/4e/cffdb69d10424e97eb1d993b1e2cfd/content.htm
    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host> -n <parameter name> [-l <parameter value>]"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host", help="Remote host")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int", help="Remote port [%default]", default=3900)
    target.add_option("--route-string", dest="route_string", help="Route string for connecting through a SAP Router")
    parser.add_option_group(target)

    param = OptionGroup(parser, "Parameter")
    param.add_option("-n", "--parameter-name", dest="param_name", help="Parameter name")
    param.add_option("-l", "--parameter-value", dest="param_value", help="Parameter value")
    parser.add_option_group(param)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-c", "--client", dest="client", default="pysap's-paramchanger", help="Client name [%default]")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Verbose output [%default]")
    parser.add_option_group(misc)

    (options, _) = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")
    if not options.param_name:
        parser.error("Parameter name is required")

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

    client_string = options.client

    # Build MS_LOGIN_2 packet
    p = SAPMS(flag=0x00, iflag=0x08, toname=client_string, fromname=client_string)

    # Send MS_LOGIN_2 packet
    print("[*] Sending login packet")
    response = conn.sr(p)[SAPMS]

    print("[*] Login performed, server string: %s" % response.fromname)
    server_string = response.fromname

    print("[*] Retrieving current value of parameter: %s" % options.param_name)

    # Send ADM AD_PROFILE request
    adm = SAPMSAdmRecord(opcode=0x1, parameter=options.param_name)
    p = SAPMS(toname=server_string, fromname=client_string, version=4,
              flag=0x04, iflag=0x05, adm_records=[adm])

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
                  iflag=5, flag=4, adm_records=[adm])

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
