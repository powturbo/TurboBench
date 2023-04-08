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

# External imports
from scapy.layers.inet import TCP
from scapy.packet import Packet, bind_layers
from scapy.fields import ByteField, StrFixedLenField
# Custom imports
from pysap.SAPNI import SAPNI
from pysap.utils.fields import StrFixedLenPaddedField
# Optional imports
try:
    from requests import Request
except ImportError:
    Request = None


# IGS Request Interpreter
# Not used yet, but it list interpreters
igs_req_interpreter = {
    1: "ZIPPER",           # ZIP provide file(s)
    2: "IMGCONV",          # Image converter
    3: "RSPOCONNECTOR",    # Remote Spool Connector
    4: "XMLCHART",         # Chart generator through XML input
    5: "CHART",            # Chart generator through ABAP Table input
    6: "BWGIS",            # BW Geographic Information System
    7: "SAPGISXML",        # old SAP GIS through XML input
}
""" IGS Request Interpreter """


# IGS Request Administrator function
# Not used yet, but it list admin functions
igs_req_adm = {
    1: "ADM:REGPW",              # Register a PortWatcher
    2: "ADM:UNREGPW",            # Unregister a PortWatcher
    3: "ADM:REGIP",              # Register an Interpreter
    4: "ADM:UNREGIP",            # Unregister an Interpreter
    5: "ADM:FREEIP",             # Inform than Interpreter is free
    6: "ADM:ILLBEBACK",          # Call back function
    7: "ADM:ABORT",              # Abort Interpreter work
    8: "ADM:PING",               # Ping receive
    9: "ADM:PONG",               # Ping send
    10: "ADM:SHUTDOWNIGS",       # Shutdown IGS
    11: "ADM:SHUTDOWNPW",        # Shutdown PortWatcher
    12: "ADM:CHECKCONSUMER",     # Check Portwatcher status
    13: "ADM:FREECONSUMER",      # Inform than Portwatcher is free
    14: "ADM:GETLOGFILE",        # Display log file
    15: "ADM:GETCONFIGFILE",     # Display configfile
    16: "ADM:GETDUMP",           # Display dump file
    17: "ADM:DELETEDUMP",        # Delete dump file
    18: "ADM:INSTALL",           # Upload shape files for GIS
    19: "ADM:SWITCH",            # Switch trace log level
    20: "ADM:GETVERSION",        # Get IGS Version
    21: "ADM:STATUS",            # Display IGS Status
    22: "ADM:STATISTIC",         # old Display IGS Statistic
    23: "ADM:STATISTICNEW",      # Display IGS Statistic
    24: "ADM:GETSTATCHART",      # Get IGS Statistic chart
    25: "ADM:SIM",               # Simulation function
}
"""IGS Request Administrator function"""


class SAPIGSTable(Packet):
    """SAP IGS table description

    Mandatory for RFC call type.
    This describes table name/width/length where content will be stored.
    """
    name = "SAP IGS Table description"
    fields_desc = [
        StrFixedLenPaddedField("version_label", "VERS", length=4),
        StrFixedLenPaddedField("version", "    1.0", length=44),
        StrFixedLenPaddedField("table_name_label", "TBNM", length=4),
        StrFixedLenPaddedField("table_name", "", length=44),
        StrFixedLenPaddedField("table_line_label", "TBLN", length=4),
        StrFixedLenPaddedField("table_line", "", length=44),
        StrFixedLenPaddedField("table_width_label", "TBWD", length=4),
        StrFixedLenPaddedField("table_width", "", length=44),
        StrFixedLenPaddedField("table_column_label", "TBCL", length=4),
        StrFixedLenPaddedField("table_column", "", length=44),
        StrFixedLenPaddedField("column_name_label", "CLNM", length=4),
        StrFixedLenPaddedField("column_name", "", length=44),
        StrFixedLenPaddedField("column_width_label", "CLWD", length=4),
        StrFixedLenPaddedField("column_width", "", length=44),
    ]

    @staticmethod
    def add_entry(t_name, t_line, t_width, t_column, c_name, c_width, **kwargs):
        """Add an entry into Table description

        This where custom content is stored
        Entry must be write from the end like : '              abcd'

        :param t_name: name of table
        :type t_name: ``string``

        :param t_line: number of line
        :type t_line: ``int``

        :param t_width: width of table
        :type t_width: ``int``

        :param t_column: number of column
        :type t_column: ``int``

        :param c_name: name of column
        :type c_name: ``string``

        :param c_width: width of column
        :type c_width: ``int``

        :return: SAP IGS Table
        :rtype: :class:`SAPIGSTable`
        """
        NewEntry = SAPIGSTable(table_name=format(t_name, ">43"),
                               table_line=format(t_line, ">43"),
                               table_width=format(t_width, ">43"),
                               table_column=format(t_column, ">43"),
                               column_name=format(c_name, ">43"),
                               column_width=format(c_width, ">43")
                               )
        return NewEntry


class SAPIGS(Packet):
    """SAP Internet Graphic Server call packet

    This packet is used for the Remote IGS Function Call protocol.
    """
    name = "SAP IGS Call"
    fields_desc = [
        StrFixedLenPaddedField("function", "", padd="\x00", length=32),
        StrFixedLenPaddedField("listener", "ListenerX", padd="\x00", length=32),
        StrFixedLenPaddedField("hostname", "pysap", padd="\x00", length=81),
        StrFixedLenPaddedField("id", "1234", padd="\x00", length=4),
        StrFixedLenPaddedField("padd1", "\x00" * 15, length=15),
        ByteField("todo1", 0x01),  # TODO: need to work on it
        StrFixedLenPaddedField("padd2", "\x00" * 20, length=20),
        ByteField("todo2", 0x08),  # TODO: need to work on it
        StrFixedLenPaddedField("padd3", "\x00" * 6, length=6),
        StrFixedLenField("eye_catch", "TransMagic", length=10),
        StrFixedLenPaddedField("padd4", "\x00" * 2, length=2),
        StrFixedLenField("codepage", "4103", length=4),
        StrFixedLenPaddedField("offset_content", "", padd="\x00", length=16),
        StrFixedLenPaddedField("packet_size", "", padd="\x00", length=16),
    ]

    @staticmethod
    def http(host, port, interpreter, files=None, tls=False, method='POST'):
        """HTTP request for IGS

        This method build a http(s) request for IGS HTTP Multiplexer service
        instead of Remote IGS Function Call

        :param host: remote host to connect to
        :type host: ``string``

        :param port: remote port to connect to
        :type port: ``int``

        :param interpreter: interpreter of function to call
        :type interpreter: ``string``

        :param files: list of files to send in case of multipart/form-data
        :type files: ``dict``

        :param tls: using https or not
        :type tls: ``bool``

        :param method: HTTP request method to use
        :type method: ``string``

        :return: the HTTP request to send as raw
        :rtype: ``string``

        :raise ImportError: if the requests library can't be imported
        """
        if Request is None:
            raise ImportError("requests library not available")

        protocol = 'https' if tls else 'http'
        url = "%s://%s:%d/%s" % (protocol, host, port, interpreter)  # forge url
        # using PreparedRequest to retrieve the raw http request
        req = Request(method, url, files=files).prepare()
        # update User-Agent header
        req.headers['User-Agent'] = 'pysap'
        # format the request than could be send with SAP NI
        req_format = ('{}\r\n{}\r\n\r\n{}'.format(
                    req.method + ' ' + req.url + ' HTTP/1.1',
                    '\r\n'.join('{}: {}'.format(k, v) for k, v in req.headers.items()),
                    req.body)
                 )
        return req_format


# Bind SAP NI with the IGS ports
bind_layers(TCP, SAPNI, dport=40000)
bind_layers(TCP, SAPNI, dport=40080)
