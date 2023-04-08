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
import datetime
from argparse import ArgumentParser
# External imports
from scapy.config import conf
# Custom imports
import pysap
from pysap.SAPHDB import (SAPHDBConnection, SAPHDBTLSConnection, SAPHDBConnectionError,
                          SAPHDBAuthenticationError, saphdb_auth_methods)
# Optional imports
try:
    import jwt as py_jwt
except ImportError:
    py_jwt = None


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script is an experimental implementation of the HANA's hdbsql tool. It" \
                  "focuses on the authentication and connection to the HANA server, and it't not meant" \
                  "to implement the full capabilities offered by hdbsql or any other HDB client interface."

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=39015,
                        help="Remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")

    tls = parser.add_argument_group("TLS")
    tls.add_argument("--tls", dest="tls", action="store_true",
                     help="Use TLS")
    tls.add_argument("--tls-no-trust-cert", dest="tls_cert_trust", action="store_false",
                     help="Do not trust the TLS certificate and validate it")
    tls.add_argument("--tls-cert-file", dest="tls_cert_file",
                     help="Path to the certificate file to use when validating server's TLS certificate.")
    tls.add_argument("--tls-check-hostname", dest="tls_check_hostname", action="store_true",
                     help="Validate the hostname provided in the TLS certificate")

    auth = parser.add_argument_group("Authentication")
    auth.add_argument("-m", "--method", dest="method", default="SCRAMSHA256",
                      help="Authentication method. Supported methods: {} [%(default)s]".format(",".join(saphdb_auth_methods.keys())))
    auth.add_argument("--username", dest="username", help="User name")
    auth.add_argument("--password", dest="password", help="Password")
    auth.add_argument("--jwt-file", dest="jwt_file", metavar="FILE",
                      help="File to read a signed JWT from")
    auth.add_argument("--jwt-key", dest="jwt_key", metavar="FILE",
                      help="File to read the private key to sign the JWT")
    auth.add_argument("--jwt-issuer", dest="jwt_issuer", help="JWT signature issuer")
    auth.add_argument("--jwt-claim", dest="jwt_claim", default="user_name",
                      help="Name of the JWT claim to map username [%(default)s]")
    auth.add_argument("--saml-assertion", dest="saml_assertion", metavar="FILE",
                      help="File to read a signed SAML 2.0 bearer assertion from")
    auth.add_argument("--session-cookie", dest="session_cookie", help="Session Cookie")
    auth.add_argument("--pid", dest="pid", default="0", help="Process ID [%(default)s]")
    auth.add_argument("--hostname", dest="hostname", help="Hostname")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")

    options = parser.parse_args()

    if not options.remote_host:
        parser.error("Remote host is required")

    if options.method not in saphdb_auth_methods:
        parser.error("Invalid authentication method")
    if not options.username and options.method not in ["SAML"]:
        parser.error("Username needs to be provided")

    if options.method == "JWT":
        if not (options.jwt_file or (options.jwt_key and options.jwt_issuer)):
            parser.error("JWT file or a signing private key and issuer need to be provided for JWT authentication")
        if options.jwt_key and not py_jwt:
            parser.error("JWT crafting requires the PyJWT library installed")

    if options.method == "SAML" and not options.saml_assertion:
        parser.error("SAML bearer assertion file need to be provided for SAML authentication")

    if options.method in ["SCRAMSHA256", "SCRAMPBKDF2SHA256"] and not options.password:
        parser.error("Password need to be provided for SCRAM-based authentication")

    if options.method == "SessionCookie" and not options.session_cookie:
        parser.error("Session cookie need to be provided for SessionCookie authentication")

    return options


def craft_auth_method(options):
    """Obtain the corresponding Authentication method according to the command line options provided.
    """

    logging.debug("[*] Using authentication method %s" % options.method)
    auth_method_cls = saphdb_auth_methods[options.method]
    auth_method = None

    if options.method == "SAML":
        # If SAML was specified, read the SAML signed assertion from a file and pass it to the auth method
        # Note that the username is not specified and instead read by the server from the SAML assertion
        with open(options.saml_assertion, 'r') as saml_assertion_fd:
            auth_method = auth_method_cls("", saml_assertion_fd.read())

    elif options.method == "JWT":
        # If JWT from file was specified, read the signed JWT from a file and pass it to the auth method
        if options.jwt_file:
            with open(options.jwt_file, 'r') as jwt_fd:
                auth_method = auth_method_cls(options.username, jwt_fd.read())
        # Otherwise if a JWT certificate was specified, we'll try to create and sign a new JWT and pass it
        # Note that this requires the PyJWT library
        elif options.jwt_key:
            with open(options.jwt_key, 'r') as jwt_key_fd:
                jwt_raw = {options.jwt_claim: options.username,
                           "iss": options.jwt_issuer,
                           "nbf": datetime.datetime.utcnow() - datetime.timedelta(seconds=30),
                           "exp": datetime.datetime.utcnow() + datetime.timedelta(seconds=30),
                           }
                jwt_signed = py_jwt.encode(jwt_raw, jwt_key_fd.read(), algorithm="RS256")
                auth_method = auth_method_cls(options.username, jwt_signed)

    elif options.method in ["SCRAMSHA256", "SCRAMPBKDF2SHA256"]:
        auth_method = auth_method_cls(options.username, options.password)

    elif options.method == "SessionCookie":
        auth_method = auth_method_cls(options.username, options.session_cookie)

    return auth_method


# Main function
def main():
    options = parse_options()

    level = logging.INFO
    if options.verbose:
        level = logging.DEBUG
    logging.basicConfig(level=level, format='%(message)s')

    # Select the desired authentication method
    auth_method = craft_auth_method(options)
    if auth_method is None:
        logging.error("[-] Unsupported authentication method")
        return

    # Create the connection
    connection_class = SAPHDBConnection
    kwargs = {"auth_method": auth_method,
              "route": options.route_string,
              "pid": options.pid,
              "hostname": options.hostname}
    if options.tls:
        connection_class = SAPHDBTLSConnection
        kwargs.update({"tls_cert_trust": options.tls_cert_trust,
                       "tls_cert_file": options.tls_cert_file,
                       "tls_check_hostname": options.tls_check_hostname})

    hdb = connection_class(options.remote_host,
                           options.remote_port,
                           **kwargs)

    try:
        hdb.connect()
        logging.debug("[*] Connected to HANA database %s:%d" % (options.remote_host, options.remote_port))
        hdb.initialize()
        logging.debug("[*] HANA database version %d/protocol version %d" % (hdb.product_version,
                                                                            hdb.protocol_version))
        hdb.authenticate()
        logging.info("[*] Successfully authenticated against HANA database server")

        if hdb.auth_method.session_cookie is not None:
            logging.info("[*] Session cookie assigned to this session: %s" % hdb.auth_method.session_cookie)

        hdb.close()
        logging.debug("[*] Connection with HANA database server closed")

    except SAPHDBAuthenticationError as e:
        logging.error("[-] Authentication error: %s" % e.message)
    except SAPHDBConnectionError as e:
        logging.error("[-] Connection error: %s" % e.message)
    except KeyboardInterrupt:
        logging.info("[-] Connection canceled")


if __name__ == "__main__":
    main()
