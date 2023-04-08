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
import logging
from argparse import ArgumentParser
# External imports
from scapy.config import conf
# Custom imports
import pysap
from pysap.SAPIGS import SAPIGS
from pysap.SAPRouter import SAPRoutedStreamSocket, ROUTER_TALK_MODE_NI_RAW_IO

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script send provided file to IGS ZIPPER interpreter " \
                  "using HTTP Listener"

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=40080,
                        help="Remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")

    param = parser.add_argument_group("Parameters")
    param.add_argument("-i", dest="file_input", default='poc.txt', metavar="FILE",
                       help="File to zip [%(default)s]")
    param.add_argument("-a", dest="file_path", default='',
                       help="Path in zip file [%(default)s]")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    print("[*] Testing IGS ZIPPER interpreter on %s:%d" % (options.remote_host,
                                                           options.remote_port))
    # open input file
    try:
        with open(options.file_input, 'rb') as f:
            file_input_content = f.read()
    except IOError:
        print("[!] Error reading %s file." % options.file_input)
        exit(2)

    # Initiate the connection
    conn = SAPRoutedStreamSocket.get_nisocket(options.remote_host,
                                              options.remote_port,
                                              options.route_string,
                                              talk_mode=ROUTER_TALK_MODE_NI_RAW_IO)

    # the xml request for zipper interpreter
    xml = '<?xml version="1.0"?><REQUEST><COMPRESS type="zip"><FILES>'
    xml += '<FILE name="%s" ' % (options.file_input)
    xml += 'path="%s" ' % (options.file_path)
    xml += 'size="%s"/>' % (len(file_input_content))
    xml += '</FILES></COMPRESS></REQEST>'

    # http request type multipart/form-data
    files = {"xml": ("xml", xml), "zipme": ("zipme", file_input_content)}
    p = SAPIGS.http(options.remote_host, options.remote_port, 'ZIPPER', files)

    # Send/Receive request
    print("[*] Send %s to ZIPPER interpreter..." % options.file_input)
    conn.send(p)
    print("[*] Response :")
    response = conn.recv(1024)
    response.show()

    # Extract zip from response
    print("[*] Generated file(s) :")
    for url in str(response).split('href='):
        if "output" in url:
            print("http://%s:%d%s" % (options.remote_host,
                                      options.remote_port,
                                      url.split('"')[1]))


if __name__ == "__main__":
    main()
