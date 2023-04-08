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
from re import escape
from argparse import ArgumentParser
# External imports
from scapy.config import conf
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI
from pysap.SAPDiagItems import *
from pysap.SAPDiagClient import SAPDiagConnection
from pysap.SAPDiag import (SAPDiag, SAPDiagDP, diag_appl_ids, diag_appl_sids,
                           diag_item_types)
# adding for text info rendering # gelim
from collections import OrderedDict


# Bind the SAPDiag layer
bind_layers(SAPNI, SAPDiag,)
bind_layers(SAPNI, SAPDiagDP,)
bind_layers(SAPDiagDP, SAPDiag,)
bind_layers(SAPDiag, SAPDiagItem,)
bind_layers(SAPDiagItem, SAPDiagItem,)

gui_lang = {
    "0": "Serbian",
    "1": "Chinese",
    "2": "Thai",
    "3": "Korean",
    "4": "Romanian",
    "5": "Slovenian",
    "6": "Croatian",
    "7": "Malaysian",
    "8": "Ukrainian",
    "9": "Estonian",
    "A": "Arabic",
    "B": "Hebrew",
    "C": "Czech",
    "D": "German",
    "E": "English",
    "F": "French",
    "G": "Greek",
    "H": "Hungarian",
    "I": "Italian",
    "J": "Japanese",
    "K": "Danish",
    "L": "Polish",
    "M": "trad.",
    "N": "Dutch",
    "O": "Norwegian",
    "P": "Portuguese",
    "Q": "Slovakian",
    "R": "Russian",
    "S": "Spanish",
    "T": "Turkish",
    "U": "Finnish",
    "V": "Swedish",
    "W": "Bulgarian",
    "X": "Lithuanian",
    "Y": "Latvian",
    "Z": "reserve",
    "a": "Afrikaans",
    "b": "Icelandic",
    "c": "Catalan",
    "d": "(Latin)",
    "i": "Indonesian",
}

serv_info = {
    'DBNAME': lambda s: s,
    'CPUNAME': lambda s: s,
    'CLIENT': lambda s: s,
    'LANGUAGE': lambda s: gui_lang.get(s, 'Language unknown (%s)' % s),
    'SESSION_ICON': lambda s: s,
    'SESSION_TITLE': lambda s: s,
    'KERNEL_VERSION': lambda s: '.'.join(s[:-1].split('\x00')),
}

key_len = 20
val_len = 60


def show_all(item):
    """
    Print the information about each item: type, ID, SID and value.

    """
    print("\tType = %s\tId = %s\tSID = %s\tValue = %s" % (diag_item_types[item.item_type],
                                                          diag_appl_ids[item.item_id],
                                                          diag_appl_sids[item.item_id][item.item_sid],
                                                          escape(str(item.item_value))))


def show_serv_info(item):
    """
    Print server information displayed in login screen

    """
    isid = diag_appl_sids[item.item_id][item.item_sid]
    if isid in serv_info.keys():
        print(("%s" % isid).ljust(key_len) + "\t" + ("%s" % serv_info[isid](item.item_value)).ljust(val_len))


def show_text_info(item):
    """
    Print (only) text information rendered in login screen

    """
    isid = diag_appl_sids[item.item_id][item.item_sid]
    iid = diag_appl_ids[item.item_id]

    if iid == 'DYNT' and isid == 'DYNT_ATOM':
        dico = OrderedDict()
        items = item.item_value.items

        for it in items:
            var = it.getfieldval('name_text')
            value = it.getfieldval("field1_text")
            if value is None:
                value = it.getfieldval("field2_text")
            key = '%s_%s' % (it.row, it.col)
            if key not in dico.keys():
                dico[key] = {'var': key, 'value': value}
            if value:
                dico[key]['value'] = value.strip()
            if var:
                dico[key]['var'] = var

        # second pass to bind left text to right value (in screen)
        dico_final = OrderedDict()
        for pos in dico.keys():
            var = dico[pos]['var']
            value = dico[pos]['value']
            dico_final[var] = value

        # final rendering
        for k in dico_final.keys():
            if dico_final[k]:
                print(("%s" % k).ljust(key_len) + "\t" + ("%s" % dico_final[k]).ljust(val_len))


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script gather information provided by a SAP Netweaver Application Server during the " \
                  "login process. This information includes generally hostname, instance, database name, etc."

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host", help="SAP remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3200,
                        help="SAP remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")
    misc.add_argument("--terminal", dest="terminal", default=None,
                      help="Terminal name")

    options = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")

    return options


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Create the connection to the SAP Netweaver server
    print("[*] Connecting to %s:%d" % (options.remote_host, options.remote_port))
    connection = SAPDiagConnection(options.remote_host, options.remote_port,
                                   init=False, terminal=options.terminal,
                                   route=options.route_string)

    # Send the initialization packet and store the response (login screen)
    login_screen = connection.init()

    print("[+] Dumping technical information")
    for item in login_screen[SAPDiag].get_item(["APPL"],
                                               ["ST_R3INFO", "ST_USER", "VARINFO"]):
        show_serv_info(item)
    print("\n[+] Login Screen text")
    for item in login_screen[SAPDiag].get_item(["APPL", "APPL4"],
                                               ["DYNT"]):
        show_text_info(item)
    print("-" * key_len + "-" * val_len)

    connection.close()


if __name__ == "__main__":
    main()
