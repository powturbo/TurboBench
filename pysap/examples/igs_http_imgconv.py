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

    description = "This example script convert provided jpg file to png 100x100 file " \
                  "using IGS interpreter IMGCONV through HTTP listener."

    usage = "%(prog)s [options] -d <remote host> -i <input image>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=40080,
                        help="Remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")
    misc.add_argument("-i", "--image", dest="input_image", metavar="FILE", default="poc.jpg",
                      help="Image to convert [%(default)s]")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Open image to convert
    try:
        with open(options.input_image, "rb") as f:
            image = f.read()
    except IOError:
        print("Error reading image file !")
        exit(0)

    print("[*] Testing IGS IMGCONV on http://%s:%d" % (options.remote_host,
                                                       options.remote_port))

    # Initiate the connection
    conn = SAPRoutedStreamSocket.get_nisocket(options.remote_host,
                                              options.remote_port,
                                              options.route_string,
                                              talk_mode=ROUTER_TALK_MODE_NI_RAW_IO)

    # XML file request
    # JPEG to PNG size 100x100
    xml = '''<?xml version="1.0" encoding="UTF-8"?>
    <IMAGE>
      <WIDTH>100</WIDTH>
      <HEIGTH>100</HEIGTH>
      <INPUT>image/jpeg</INPUT>
      <OUTPUT>image/png</OUTPUT>
    </IMAGE>
    '''

    # build http packet
    files = {"xml": ("xml", xml), "img": ("img", image)}
    p = SAPIGS.http(options.remote_host, options.remote_port, 'IMGCONV', files)

    # Send request
    print("[*] Send packet to IGS...")
    conn.send(p)
    print("[*] Response :")
    response = conn.recv()
    response.show()

    # Extract picture url from response
    print("[*] Generated file(s) :")
    for url in str(response).split('href='):
        if "output" in url:
            print("http://%s:%d%s" % (options.remote_host,
                                      options.remote_port,
                                      url.split('"')[1]))


if __name__ == "__main__":
    main()
