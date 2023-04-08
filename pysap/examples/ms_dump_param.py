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
#   Example script by Yvan Genuer (@iggy38).
#   Martin Gallo (@martingalloar)
#   Code contributed by SecureAuth to the OWASP CBAS project
#

# Standard imports
import re
import logging
from argparse import ArgumentParser
# External imports
from scapy.config import conf
# Custom imports
import pysap
from pysap.SAPRouter import SAPRoutedStreamSocket
from pysap.SAPMS import SAPMS, SAPMSAdmRecord

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script is a subset of ms_dump_info.py." \
                  " For each parameter provide in file, it retrieve value and check the result." \
                  " Access of the internal Message Server port is required." \
                  " Due to the wide of parameters type in SAP System, it's not perfect, and false positive could exist."

    usage = "%(prog)s [options] -d <remote host> -f <parameters_file>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3900,
                        help="Remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")

    param = parser.add_argument_group("Parameter")
    param.add_argument("-f", "--parameter-file", dest="file_param", metavar="FILE",
                       help="parameters file (<SAP parameter>:<Check type>:<expected_value>)")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")
    misc.add_argument("-c", "--client", dest="client", default="pysap's-getparam",
                      help="Client name [%(default)s]")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")
    if not options.file_param:
        parser.error("Parameters file is required")

    return options


# Main
# -----
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # initiate the connection :
    print("[*] Initiate connection to message server %s:%d" % (options.remote_host, options.remote_port))
    try:
        conn = SAPRoutedStreamSocket.get_nisocket(options.remote_host,
                                                  options.remote_port,
                                                  options.route_string,
                                                  base_cls=SAPMS)
    except Exception as e:
        print(e)
        print ("Error during MS connection. Is internal ms port %d reachable ?" % options.remote_port)
    else:
        print ("[*] Connected. I check parameters...")
        client_string = options.client
        # Send MS_LOGIN_2 packet
        p = SAPMS(flag=0x00, iflag=0x08, toname=client_string, fromname=client_string)
        print("[*] Sending login packet:")
        response = conn.sr(p)[SAPMS]
        print("[*] Login OK, Server string: %s\n" % response.fromname)
        server_string = response.fromname

        try:
            with open(options.file_param) as list_param:
                for line in list_param.readlines():
                    line = line.strip()

                    # Check for comments or empty lines
                    if len(line) == 0 or line.startswith("#"):
                        continue

                    # Get parameters, check type and expected value
                    # param2c = the SAP parameter to check
                    # check_type = EQUAL, SUP, INF, REGEX, <none>
                    # value2c = the expect value for 'ok' status
                    (param2c, check_type, value2c) = line.split(':')
                    status = '[!]'

                    # create request
                    adm = SAPMSAdmRecord(opcode=0x1, parameter=param2c)
                    p = SAPMS(toname=server_string, fromname=client_string, version=4, flag=0x04, iflag=0x05,
                              adm_records=[adm])

                    # send request
                    respond = conn.sr(p)[SAPMS]
                    value = respond.adm_records[0].parameter.replace(respond.adm_records[0].parameter.split('=')[0] +
                                                                     '=', '')

                    status = '[ ]'
                    # Verify if value match with expected value
                    if value == '':
                        value = 'NOT_EXIST'
                    elif check_type == 'EQUAL':
                        if value.upper() == str(value2c).upper():
                            status = '[+]'
                    elif check_type == 'NOTEQUAL':
                        if value.upper() != str(value2c).upper():
                            status = '[+]'
                    elif check_type == 'REGEX':
                        if re.match(value2c.upper(), value.upper()) and value2c != 'NOT_EXIST':
                            status = '[+]'
                    elif check_type == 'SUP':
                        if float(value) >= float(value2c):
                            status = '[+]'
                    elif check_type == 'INF':
                        if float(value) <= float(value2c):
                            status = '[+]'

                    # display result
                    print ("%s %s = %s" % (status, param2c, value))

        except IOError:
            print("Error reading parameters file !")
            exit(0)
        except ValueError:
            print("Invalid parameters file format or access denied!")
            exit(0)


if __name__ == '__main__':
    main()
