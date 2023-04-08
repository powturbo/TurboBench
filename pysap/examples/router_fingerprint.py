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
import json
import logging
from argparse import ArgumentParser
# External imports
from scapy.config import conf
from scapy.packet import bind_layers, Raw
# Custom imports
import pysap
from pysap.SAPNI import SAPNIStreamSocket, SAPNI
from pysap.SAPRouter import SAPRouter, SAPRouterRouteHop


# Bind the SAPRouter layer
bind_layers(SAPNI, SAPRouter, )

# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script connects with a SAP Router service and tries to determine its version. " \
                  "Finger printing is performed by triggering different errors and looking at the lines where the " \
                  "error is produced."

    usage = "%(prog)s -d <remote host> [options]"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host", default="127.0.0.1",
                        help="Remote host [%(default)s]")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3299,
                        help="Remote port [%(default)d]")

    database = parser.add_argument_group("Database options")
    database.add_argument("-f", "--fingerprints-file", dest="fingerprints", metavar="FILE",
                          default="router_fingerprints.json", help="Fingerprints file to use [%(default)s]")
    database.add_argument("-a", "--add-fingerprint", dest="add_fingerprint", action="store_true",
                          help="New fingerprint to add to the database in json format")
    database.add_argument("-i", "--version-information", dest="version_info",
                          help="Version information to use when adding new entries in json format")
    database.add_argument("-n", "--new-entries", dest="new_entries", action="store_true",
                          help="Generate new database entries even when the fingerprints matched")
    database.add_argument("--new-fingerprints-file", dest="new_fingerprint_file", metavar="FILE",
                          default="saprouter_new_fingerprints.json", help="File to write or load from new fingerprints")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")

    return options


# This is the list of fields we want to track when checking for fingerprints
fingerprint_fields = ["error", "return_code", "component", "release", "version", "module", "line", "errorno",
                      "errorno_text", "system_call"]


# The fields used to identify a version being fingerprinted
version_info_fields = ["version", "release", "patch_number", "source_id", "update_level", "file_version", "platform",
                       "submitted_by", "comment"]


# This is the list of target packets that we use during the fingerprinting. Basically it consist of a dict with the
# key being the name of the target packet and the value the construction of it.
fingerprint_targets = {
    # Connect to the SAP Route but not send any packet to trigger a timeout
    "Timeout": None,
    # Send a large packet
    "Network packet too big": Raw("X" * 10025),
    # Use an invalid opcode
    "Invalid control opcode": SAPRouter(type=SAPRouter.SAPROUTER_CONTROL, version=38, opcode=3),
    # Do not send a route
    "No route": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE),
    # Set one entry but do no provide a route
    "No route one entry": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                    route_entries=1),
    # Set one entry with an invalid length but do no provide it
    "No route invalid length": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                         route_entries=2,
                                         route_rest_nodes=1,
                                         route_length=1,
                                         route_offset=3),
    # Set one entry with an invalid route
    "Empty route invalid length": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                            route_entries=2,
                                            route_rest_nodes=1,
                                            route_length=1,
                                            route_offset=0,
                                            route_string=[SAPRouterRouteHop()]),
    # Set an empty route with valid length
    "Empty route valid length": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                          route_entries=2,
                                          route_rest_nodes=1,
                                          route_length=6,
                                          route_offset=3,
                                          route_string=[SAPRouterRouteHop(), SAPRouterRouteHop()]),
    # Set an empty route with a null offset
    "Empty route null offset": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE, route_entries=2,
                                         route_rest_nodes=1,
                                         route_length=6,
                                         route_offset=0,
                                         route_string=[SAPRouterRouteHop(), SAPRouterRouteHop()]),
    # Set an empty route with an invalid offset
    "Empty route invalid offset": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                            route_entries=2,
                                            route_rest_nodes=1,
                                            route_length=6,
                                            route_offset=6,
                                            route_string=[SAPRouterRouteHop(), SAPRouterRouteHop()]),
    # Set a route for a non existent domain
    "Non existent domain": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                     route_entries=2,
                                     route_rest_nodes=1,
                                     route_length=38,
                                     route_offset=19,
                                     route_string=[SAPRouterRouteHop(hostname="non.existent.dom"),
                                                   SAPRouterRouteHop(hostname="non.existent.dom")]),
    # Valid route for an non existent domain with an old version
    "Non existent domain old version": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                                 route_ni_version=0,
                                                 route_entries=2,
                                                 route_rest_nodes=1,
                                                 route_length=38,
                                                 route_offset=19,
                                                 route_string=[SAPRouterRouteHop(hostname="non.existent.dom"),
                                                               SAPRouterRouteHop(hostname="non.existent.dom")]),
    # Valid route for a valid domain but an invalid service name
    "Valid domain invalid service": SAPRouter(type=SAPRouter.SAPROUTER_ROUTE,
                                              route_entries=2,
                                              route_rest_nodes=1,
                                              route_length=56,
                                              route_offset=28,
                                              route_string=[SAPRouterRouteHop(hostname="www.coresecurity.com",
                                                                              port="someservice"),
                                                            SAPRouterRouteHop(hostname="www.coresecurity.com",
                                                                              port="someservice")]),
}


class FingerprintDB(object):

    def __init__(self, fingerprints_file):
        self.fingerprints_db = {}
        self.fingerprints_file = fingerprints_file
        self.load_fingerprints(fingerprints_file)

    def load_fingerprints(self, fingerprints_file):
        with open(fingerprints_file, 'r') as f:
            fingerprints = json.load(f)

        for target in fingerprint_targets:
            if target in fingerprints:
                self.fingerprints_db[target] = fingerprints[target]

    def add_fingerprint(self, new_fingerprint, version_info):
        with open(new_fingerprint, 'r') as f:
            new_fingerprint = json.load(f)
        if version_info:
            version_info = json.loads(version_info)
        else:
            version_info = {}

        for target in fingerprint_targets:
            if target in new_fingerprint:
                if target not in self.fingerprints_db:
                    self.fingerprints_db[target] = []
                for entry in new_fingerprint[target]:
                    entry.update(version_info)
                    if entry not in self.fingerprints_db[target]:
                        logging.info("[*]\tAdded a new entry for the target %s" % target)
                        self.fingerprints_db[target].append(entry)

        with open(self.fingerprints_file, 'w') as f:
            json.dump(self.fingerprints_db, f, sort_keys=True, indent=4, separators=(',', ': '))

    def match_fingerprint(self, target, error_text):
        matches = []
        if target in self.fingerprints_db:
            for finger in self.fingerprints_db[target]:
                match = True
                for key, value in list(finger.items()):
                    if key in fingerprint_fields and hasattr(error_text, key) and getattr(error_text, key) != value:
                        match = False
                        logging.debug("[ ]\tUnmatched field: \"%s\" Value: \"%s\" vs \"%s\"" % (key, value,
                                                                                                getattr(error_text,
                                                                                                        key)))
                if match:
                    matches.append(finger)
        return matches


# Main function
def main():
    options = parse_options()

    level = logging.INFO
    if options.verbose:
        level = logging.DEBUG
    logging.basicConfig(level=level, format='%(message)s')

    logging.info("[*] Loading fingerprint database")
    fingerprint_db = FingerprintDB(options.fingerprints)

    # Check if we were asked to add a new fingerprint
    if options.add_fingerprint:
        if not options.version_info:
            logging.info("[-] You must provide version info to add new entries to the fingerprint database !")
            return
        logging.info("[*] Adding a new entry to the fingerprint database")
        fingerprint_db.add_fingerprint(options.new_fingerprint_file, options.version_info)
        return

    misses = []
    matches = []
    logging.info("[*] Trying to fingerprint version using %d packets" % (len(fingerprint_targets)))

    # Trigger some errors and check with fingerprint db
    l = len(fingerprint_targets)
    i = 1
    for (target, packet) in list(fingerprint_targets.items()):

        logging.info("[*] (%d/%d) Fingerprint for packet '%s'" % (i, l, target))

        # Initiate the connection and send the packet
        conn = SAPNIStreamSocket.get_nisocket(options.remote_host,
                                              options.remote_port,
                                              keep_alive=False)
        if packet is None:  # Timeout error
            error_text = conn.recv().err_text_value
        else:
            error_text = conn.sr(packet).err_text_value

        matched = fingerprint_db.match_fingerprint(target, error_text)

        if matched:
            logging.info("[*] (%d/%d) Fingerprint for packet '%s' matched !" % (i, l, target))
            matches.append((target, matched))
        else:
            logging.info("[*] (%d/%d) Fingerprint for packet '%s' not matched" % (i, l, target))
            misses.append((target, error_text))

        i += 1

    if matches:
        versions = []
        counts = {}
        logging.info("\n[*] Matched fingerprints (%d/%d):" % (len(matches), l))
        for (target, fingerprints) in matches:
            logging.info("[+] Request: %s" % target)
            for fingerprint in fingerprints:
                match = {}
                for field in version_info_fields:
                    if field in fingerprint:
                        match[field] = fingerprint[field]
                if match not in versions:
                    counts[str(match)] = 0
                    versions.append(match)
                counts[str(match)] += 1

        logging.info("\n[*] Probable versions (%d):" % len(versions))
        for version in versions:
            msg = " ".join(["%s: \"%s\"" % (field, version[field]) for field in version_info_fields
                            if version[field] != ""])
            logging.info("[*]\tHits: %d Version: %s" % (counts[str(version)], msg))

    if misses:
        logging.info("\n[*] Non matched fingerprints (%d/%d):" % (len(misses), l))
        for (target, _) in misses:
            logging.info("[-] Request: %s" % target)

        logging.info("\n[-] Some error values where not found in the fingerprint database. "
                     "If you want to contribute submit a issue to https://github.com/OWASP/pysap/ "
                     "or write an email to mgallo@secureauth.com with the following information along "
                     "with the SAP Router file information and how it was configured.\n")
        options.new_entries = True

    # Build new entries for the fingerprint database
    if options.new_entries:
        new_fingerprint = {}
        for (target, error_text) in misses:
            new_fingerprint[target] = [{}]
            for field in fingerprint_fields:
                new_fingerprint[target][0][field] = getattr(error_text, field)
        # Expand with matched targets also
        for (target, fingerprint) in matches:
            new_fingerprint[target] = fingerprint

        logging.info("\nNew fingerprint saved to: %s" % options.new_fingerprint_file)
        with open(options.new_fingerprint_file, 'w') as f:
            json.dump(new_fingerprint, f)

        version_info = {"patch_number": "",
                        "source_id": "",
                        "update_level": "",
                        "file_version": "",
                        "platform": "",
                        "submitted_by": "",
                        "comment": "",
                        }
        logging.info("\n\nVersion information to complete and submit:")
        logging.info("%s" % json.dumps(version_info, indent=4))


if __name__ == "__main__":
    main()
