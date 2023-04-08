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
import ssl
import socket
import struct
# External imports
from cryptography.hazmat.backends import default_backend
from scapy.layers.inet import TCP
from scapy.packet import Packet, bind_layers, Raw
from scapy.supersocket import SSLStreamSocket
from scapy.fields import (ByteField, ConditionalField, EnumField, FieldLenField, YesNoByteField,
                          IntField, PacketListField, LongField, PadField, LEIntField, LESignedIntField,
                          StrFixedLenField, ShortField, MultipleTypeField, StrField, MultiEnumField, Field)
# Custom imports
import pysap
from pysap.SAPRouter import SAPRoutedStreamSocket
from pysap.utils.crypto import SCRAM_SHA256, SCRAM_PBKDF2SHA256
from pysap.utils.fields import (PacketNoPadded, AdjustableFieldLenField, LESignedByteField,
                                LESignedShortField, LESignedLongField)


hdb_packetoptions_values = {
    0: "Uncompressed",
    2: "Compressed",
}
"""SAP HDB Packet Options Values"""


hdb_segmentkind_values = {
    0: "Invalid",
    1: "Request",
    2: "Reply",
    5: "Error",
}
"""SAP HDB Segment Kind Values"""


hdb_message_type_values = {
    0: "NIL",
    2: "EXECUTEDIRECT",
    3: "PREPARE",
    4: "ABAPSTREAM",
    5: "XA_START",
    6: "XA_JOIN",
    7: "XA_COMMIT",
    13: "EXECUTE",
    16: "READLOB",
    17: "WRITELOB",
    18: "FINDLOB",
    25: "PING",
    65: "AUTHENTICATE",
    66: "CONNECT",
    67: "COMMIT",
    68: "ROLLBACK",
    69: "CLOSERESULTSET",
    70: "DROPSTATEMENTID",
    71: "FETCHNEXT",
    72: "FETCHABSOLUTE",
    73: "FETCHRELATIVE",
    74: "FETCHFIRST",
    75: "FETCHLAST",
    77: "DISCONNECT",
    78: "EXECUTEITAB",
    79: "FETCHNEXTITAB",
    80: "INSERTNEXTITAB",
    81: "BATCHPREPARE",
    82: "DBCONNECTINFO",
    83: "XOPEN_XASTART",
    84: "XOPEN_XAEND",
    85: "XOPEN_XAPREPARE",
    86: "XOPEN_XACOMMIT",
    87: "XOPEN_XAROLLBACK",
    88: "XOPEN_XARECOVER",
    89: "XOPEN_XAFORGET",
}
"""SAP HDB Segment Message Type Values"""


hdb_function_code_values = {
    0: "NIL",
    1: "DDL",
    2: "INSERT",
    3: "UPDATE",
    4: "DELETE",
    5: "SELECT",
    6: "SELECTFORUPDATE",
    7: "EXPLAIN",
    8: "DBPROCEDURECALL",
    9: "DBPROCEDURECALLWITHRESULT",
    10: "FETCH",
    11: "COMMIT",
    12: "ROLLBACK",
    13: "SAVEPOINT",
    14: "CONNECT",
    15: "WRITELOB",
    16: "READLOB",
    17: "PING",
    18: "DISCONNECT",
    19: "CLOSECURSOR",
    20: "FINDLOB",
    21: "ABAPSTREAM",
    22: "XASTART",
    23: "XAJOIN",
    24: "ITABWRITE",
    25: "XOPEN_XACONTROL",
    26: "XOPEN_XAPREPARE",
    27: "XOPEN_XARECOVER",
}
"""SAP HDB Segment Function Code Values"""


hdb_partkind_values = {
    0: "NIL",
    3: "COMMAND",
    5: "RESULTSET",
    6: "ERROR",
    10: "STATEMENTID",
    11: "TRANSACTIONID",
    12: "ROWSAFFECTED",
    13: "RESULTSETID",
    15: "TOPOLOGYINFORMATION",
    16: "TABLELOCATION",
    17: "READLOBREQUEST",
    18: "READLOBREPLY",
    25: "ABAPISTREAM",
    26: "ABAPOSTREAM",
    27: "COMMANDINFO",
    28: "WRITELOBREQUEST",
    29: "CLIENTCONTEXT",
    30: "WRITELOBREPLY",
    32: "PARAMETERS",
    33: "AUTHENTICATION",
    34: "SESSIONCONTEXT",
    35: "CLIENTID",
    38: "PROFILE",
    39: "STATEMENTCONTEXT",
    40: "PARTITIONINFORMATION",
    41: "OUTPUTPARAMETERS",
    42: "CONNECTOPTIONS",
    43: "COMMITOPTIONS",
    44: "FETCHOPTIONS",
    45: "FETCHSIZE",
    47: "PARAMETERMETADATA",
    48: "RESULTSETMETADATA",
    49: "FINDLOBREQUEST",
    50: "FINDLOBREPLY",
    51: "ITABSHM",
    53: "ITABCHUNKMETADATA",
    55: "ITABMETADATA",
    56: "ITABRESULTCHUNK",
    57: "CLIENTINFO",
    58: "STREAMDATA",
    59: "OSTREAMRESULT",
    60: "FDAREQUESTMETADATA",
    61: "FDAREPLYMETADATA",
    62: "BATCHPREPARE",
    63: "BATCHEXECUTE",
    64: "TRANSACTIONFLAGS",
    65: "ROWSLOTIMAGEPARAMMETADATA",
    66: "ROWSLOTIMAGERESULTSET",
    67: "DBCONNECTINFO",
    68: "LOBFLAGS",
    69: "RESULTSETOPTIONS",
    70: "XATRANSACTIONINFO",
    71: "SESSIONVARIABLE",
    72: "WORKLOADREPLAYCONTEXT",
    73: "SQLREPLYOTIONS",
}
"""SAP HDB Part Kind Values"""


hdb_data_type_vals = {
    0: "NULL",
    1: "TINYINT",
    2: "SMALLINT",
    3: "INT",
    4: "BIGINT",
    5: "DECIMAL",
    6: "REAL",
    7: "DOUBLE",
    8: "CHAR",
    9: "VARCHAR1",
    10: "NCHAR",
    11: "NVARCHAR",
    12: "BINARY",
    13: "VARBINARY",
    14: "DATE",
    15: "TIME",
    16: "TIMESTAMP",
    17: "TIME_TZ",
    18: "TIME_LTZ",
    19: "TIMESTAMP_TZ",
    20: "TIMESTAMP_LTZ",
    21: "INTERVAL_YM",
    22: "INTERVAL_DS",
    23: "ROWID",
    24: "UROWID",
    25: "CLOB",
    26: "NCLOB",
    27: "BLOB",
    28: "BOOLEAN",
    29: "STRING",
    30: "NSTRING",
    31: "LOCATOR",
    32: "NLOCATOR",
    33: "BSTRING",
    34: "DECIMAL_DIGIT_ARRAY",
    35: "VARCHAR2",
    36: "VARCHAR3",
    37: "NVARCHAR3",
    38: "VARBINARY3",
    39: "VARGROUP",
    40: "TINYINT_NOTNULL",
    41: "SMALLINT_NOTNULL",
    42: "INT_NOTNULL",
    43: "BIGINT_NOTNULL",
    44: "ARGUMENT",
    45: "TABLE",
    46: "CURSOR",
    47: "SMALLDECIMAL",
    48: "ABAPSTREAM",
    49: "ABAPSTRUCT",
    50: "ARRAY",
    51: "TEXT",
    52: "SHORTTEXT",
    53: "FIXEDSTRING",
    54: "FIXEDPOINTDECIMAL",
    55: "ALPHANUM",
    56: "TLOCATOR",
    61: "LONGDATE",
    62: "SECONDDATE",
    63: "DAYDATE",
    64: "SECONDTIME",
    65: "CSDATE",
    66: "CSTIME",
    71: "BLOB_DISK",
    72: "CLOB_DISK",
    73: "NCLOB_DISK",
    74: "GEOMETRY",
    75: "POINT",
    76: "FIXED16",
    77: "BLOB_HYBRID",
    78: "CLOB_HYBRID",
    79: "NCLOB_HYBRID",
    80: "POINTZ",
}
"""SAP HDB Data Type Values"""


def hdb_segment_is_request(segment):
    """Returns if the segment is a request

    :param segment: segment to look at
    :type segment: :class:`SAPHDBSegment`

    :return: if the segment is a request
    :rtype: ``bool``
    """
    return segment.segmentkind == 1


def hdb_segment_is_reply(segment):
    """Returns if the segment is a reply

    :param segment: segment to look at
    :type segment: :class:`SAPHDBSegment`

    :return: if the segment is a reply
    :rtype: ``bool``
    """
    return segment.segmentkind == 2


# Registry of Part classes and Option Part key values
hdb_part_kind_classes = {}
hdb_option_part_key_vals = {}
hdb_multi_option_part_key_vals = {}


def hdb_register_part_kind(cls):
    """Helper function to use as a decorator when defining Part classes. It will register
    the class with the proper Part Kind so dissection can find it. It will also register
    the keys for the options defined in each Part Kind.

    :param cls: Part class to register
    :type cls: type

    :return: Registered Part class
    :rtype: type
    """
    if hasattr(cls, "part_kind") and cls.part_kind:
        hdb_part_kind_classes[cls.part_kind] = cls
        if hasattr(cls, "option_keys") and hasattr(cls, "option_part") and cls.option_part:
            hdb_option_part_key_vals[cls.part_kind] = cls.option_keys
        elif hasattr(cls, "option_keys") and hasattr(cls, "multi_option_part") and cls.multi_option_part:
            hdb_multi_option_part_key_vals[cls.part_kind] = cls.option_keys
    return cls


def hdb_get_part_kind_option(part, key):
    """Helper function to obtain the value of an Option Part kind

    :param part: part kind object
    :type part: :class:`SAPHDBOptionPartRow`

    :param key: key to look at
    :type key: int

    :return: value from the Option Part kind
    """
    for option in part.buffer:
        if hasattr(option, "key") and option.key == key:
            return option.value
    return None


class SAPHDBOptionPartRow(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Option Part Row

    This packet represents a row in an Option Part.

    Each row is comprised of a key, type and value.
    """
    part_kind = None
    option_keys = None
    name = "SAP HANA SQL Command Network Protocol Option Part Row"
    fields_desc = [
        MultiEnumField("key", 0, hdb_option_part_key_vals, depends_on=lambda x: x.part_kind, fmt="<b"),
        EnumField("type", 0, hdb_data_type_vals, fmt="<b"),
        ConditionalField(FieldLenField("length", None, length_of="value", fmt="<h"), lambda x: x.type in [29, 30, 33]),
        MultipleTypeField(
            [
                (LESignedByteField("value", 0), lambda x: x.type == 1),
                (LESignedShortField("value", 0), lambda x: x.type == 2),
                (LESignedIntField("value", 0), lambda x: x.type == 3),
                (LESignedLongField("value", 0), lambda x: x.type == 4),
                (Field("value", 0, fmt="<d"), lambda x: x.type == 7),
                (YesNoByteField("value", 0), lambda x: x.type == 28),
                (StrFixedLenField("value", None, length_from=lambda x: x.length), lambda x: x.type in [29, 30, 33]),
            ],
            StrField("value", ""),
        ),
    ]


class SAPHDBMultiLineOptionPartRow(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Multi-line Option Part

    This packet represents a collection of Option Part Rows
    """
    partkind = None
    name = "SAP HANA SQL Command Network Protocol Multi-line Option Part"
    fields_desc = [
        FieldLenField("argcount", None, count_of="rows", fmt="<h"),
        PacketListField("rows", None, SAPHDBOptionPartRow, count_from=lambda x: x.argcount),
    ]


@hdb_register_part_kind
class SAPHDBPartCommand(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Command Part
    """
    part_kind = 3
    name = "SAP HANA SQL Command Network Protocol Command Part"
    fields_desc = [
        StrField("command", None),
    ]


hdb_error_level_vals = {
    0: "WARNING",
    1: "ERROR",
    2: "FATALERROR",
}
"""SAP HDB Error Level Values"""


@hdb_register_part_kind
class SAPHDBPartError(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Error Part
    """
    part_kind = 6
    name = "SAP HANA SQL Command Network Protocol Error Part"
    fields_desc = [
        LESignedIntField("error_code", 0),
        LESignedIntField("error_position", 0),
        FieldLenField("error_text_length", None, length_of="error_text", fmt="<i"),
        EnumField("error_level", 0, hdb_error_level_vals, fmt="<b"),
        StrFixedLenField("sql_state", "HY000", 5),
        PadField(StrFixedLenField("error_text", "", length_from=lambda pkt: pkt.error_text_length), 8),
    ]


@hdb_register_part_kind
class SAPHDBPartTopologyInformation(SAPHDBMultiLineOptionPartRow):
    """SAP HANA SQL Command Network Protocol Topology Information Part
    """
    part_kind = 15
    name = "SAP HANA SQL Command Network Protocol Topology Information Part"
    option_keys = {
        1: "Host Name",
        2: "Host Port Number",
        3: "Tenant Name",
        4: "Load Factor",
        5: "Site Volume ID",
        6: "Is Master",
        7: "Is Current Session",
        8: "Service Type",
        9: "Network Domain",
        10: "Is Stand-By",
        11: "All IP Addresses",
        12: "All Host Names",
        13: "Site Type"
    }


@hdb_register_part_kind
class SAPHDBPartCommandInfo(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Command Information Part
    """
    part_kind = 27
    name = "SAP HANA SQL Command Network Protocol Command Information Part"
    option_keys = {
        1: "Line Number",
        2: "Source Module",
    }


@hdb_register_part_kind
class SAPHDBPartClientContext(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Client Context Part
    """
    part_kind = 29
    name = "SAP HANA SQL Command Network Protocol Client Context Part"
    option_keys = {
        1: "Client Version",
        2: "Client Type",
        3: "Application Name",
    }


@hdb_register_part_kind
class SAPHDBPartSessionContext(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Session Context Part
    """
    part_kind = 34
    name = "SAP HANA SQL Command Network Protocol Session Context Part"
    option_keys = {
        1: "Primary Connection ID",
        2: "Primary Host Name",
        3: "Primary Host Port Number",
        4: "Master Connection ID",
        5: "Master Host Name",
        6: "Master Host Port Number",
    }


@hdb_register_part_kind
class SAPHDBPartClientId(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Client ID Part
    """
    part_kind = 35
    name = "SAP HANA SQL Command Network Protocol Client ID Part"
    fields_desc = [
        StrField("clientid", None),
    ]


@hdb_register_part_kind
class SAPHDBPartStatementContext(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Statement Context Part
    """
    part_kind = 39
    name = "SAP HANA SQL Command Network Protocol Statement Context Part"
    option_keys = {
        1: "Statement Sequence Info",
        2: "Server Processing Time",
        3: "Schema Name",
        4: "Flag Set",
        5: "Query Time Out",
        6: "Client Reconnection Wait Timeout",
        7: "Server CPU Time",
        8: "Server Memory Usage",
    }


@hdb_register_part_kind
class SAPHDBPartConnectOptions(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Connect Options Part
    """
    part_kind = 42
    name = "SAP HANA SQL Command Network Protocol Connect Options Part"
    option_keys = {
        1: "Connection ID",
        2: "Complete Array Execution",
        3: "Client Locale",
        4: "Supports Large Bulk Operations",
        5: "Distribution Enabled",
        6: "Primary Connection ID",
        7: "Primary Connection Host",
        8: "Primary Connection Port",
        9: "Complete Data Type Support",
        10: "Large Number of Parameters Support",
        11: "System ID",
        12: "Data Format Version",
        13: "ABAP VARCHAR Mode",
        14: "Select for Update Supported",
        15: "Client Distribution Mode",
        16: "Engine Data Format Version",
        17: "Distribution Protocol Version",
        18: "Split Batch Commands",
        19: "Use Transaction Flags Only",
        20: "Row and Column Optimized Format",
        21: "Ignore Unknown Parts",
        22: "Table Output Parameter",               # This is set to True in SAPR3 mode
        23: "Data Format Version 2",
        24: "ITAB Parameter",
        25: "Describe Table Output Parameter",      # This is set to True in SAPR3 mode
        26: "Columnar Result Set",
        27: "Scrollable Result Set",
        28: "Client Info NULL Value Supported",
        29: "Associated Connection ID",
        30: "Non-Transactional Prepare",
        31: "Fast Data Access Enabled",
        32: "OS User",
        33: "Row Slot Image Result",
        34: "Endianness",
        35: "Update Topology Anwhere",
        36: "Enable Array Type",
        37: "Implicit LOB Streaming",
        38: "Cached View Property",
        39: "X OpenXA Protocol Supported",
        40: "Master Commit Redirection Supported",
        41: "Active/Active Protocol Version",
        42: "Active/Active Connection Origin Site",
        43: "Query Timeout Supported",
        44: "Full Version String",
        45: "Database Name",
        46: "Build Platform",
        47: "Implicit XA Session Supported",
        48: "Client Side Column Encryption Version",
        49: "Compression Level And Flags",
        50: "Client Side Re-Execution Supported",
        51: "Client Reconnect Wait Timeout",
        52: "Original Anchor Connection ID",
        53: "Flag Set 1",
        54: "Topology Network Group",
        55: "IP Address",
        56: "LRR Ping Time",
    }


@hdb_register_part_kind
class SAPHDBPartCommitOptions(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Commit Options Part
    """
    part_kind = 43
    name = "SAP HANA SQL Command Network Protocol Commit Options Part"
    option_keys = {
        1: "Hold Cursors Over Commit",
    }


@hdb_register_part_kind
class SAPHDBPartFetchOptions(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Fetch Options Part
    """
    part_kind = 44
    name = "SAP HANA SQL Command Network Protocol Fetch Options Part"
    option_keys = {
        1: "Result Set Pos",
    }


@hdb_register_part_kind
class SAPHDBPartTransactionFlags(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol Transaction Flags Part
    """
    part_kind = 64
    name = "SAP HANA SQL Command Network Protocol Transaction Flags Part"
    option_keys = {
        0: "Rolled Back",
        1: "Commited",
        2: "New Isolation Level",
        3: "DDL Commit Mode Changed",
        4: "Write Transaction Started",
        5: "No Write Transaction Started",
        6: "Session Closing Transaction Error",
    }


@hdb_register_part_kind
class SAPHDBPartDBConnectInfo(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol DB Connect Information Part
    """
    part_kind = 67
    name = "SAP HANA SQL Command Network Protocol DB Connect Information Part"
    option_keys = {
        1: "Database Name",
        2: "Host",
        3: "Port",
        4: "Is Connected",
    }


@hdb_register_part_kind
class SAPHDBPartLOBFlags(SAPHDBOptionPartRow):
    """SAP HANA SQL Command Network Protocol LOB Flags Part
    """
    part_kind = 68
    name = "SAP HANA SQL Command Network Protocol LOB Flags Part"
    option_keys = {
        0: "Implicit Streaming",
    }


class SAPHDBPartAuthenticationField(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Authentication Part Field

    This packet represents a field in the Authentication Part.
    """
    name = "SAP HANA SQL Command Network Protocol Authentication Field"
    fields_desc = [
        AdjustableFieldLenField("length", None, length_of="value"),
        StrFixedLenField("value", None, length_from=lambda pkt: pkt.length),
    ]


@hdb_register_part_kind
class SAPHDBPartAuthentication(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Authentication Part

    This packet represents an Authentication Part. The Authentication part consists of a count value and then
    a number of key/value pairs expressed with field values.

    Authentication methods documented in the specification are:
        - "GSS" - Provides GSS/Kerberos authentication.
        - "PLAINPASSWORD" - Reserved. Do not use.
        - "SAML" - Provides SAML authentication.
        - "SCRAMMD5" - Reserved. Do not use.
        - "SCRAMSHA256" - Provides password-based authentication.

    Non-documented methods are:
        - "JWT"
        - "SAPLogon"
        - "SCRAMPBKDF2SHA256"
        - "SessionCookie"
        - "X509Internal"

    Known authentication key values are:
        - GSS Authentication:
            - USERNAME - User name
            - METHODNAME - Method name
            - CLIENTCHALLENGE - Client challenge
                - KRB5OID - KRB5 object ID
                - TYPEOID - Type object ID
                - CLIENTGSSNAME - Client GSS Name
            - SERVERTOKEN - Server-specific Kerberos tokens
            - CLIENTOKEN - Client-specific Kerberos tokens
        - LDAP Authentication:
            - USERNAME - User name
            - METHODNAME - Method name ("LDAP")
            - CLIENTCHALLENGE - Client challenge
            - SERVERCHALLENGE - Server Challenge
                - CLIENTNONCE - Specifies the client nonce that was sent in the initial request.
                - SERVERNONCE - Specifies the server nonce.
                - SERVERPUBLICKEY - Specifies the server public key.
                - CAPABILITYRESULT - Specifies the capability, chosen by the server, from the client request.
            - CLIENTPROOF - Specifies the client proof.
                - ENCRYPTEDSESSKEY - Specifies the encrypted session key. This is specified as: RSAEncrypt(public key, SESSIONKEY + SERVERNONCE).
                - ENCRYPTEDPASSWORD - Specifies the encrypted password. This is specified as: AES256Encrypt(SESSIONKEY, PASSWORD + SERVERNONCE).
            - SERVERPROOF - Specifies the authentication result from the LDAP server. This is specified as either SUCCESS or FAIL.
        - SAML Authentication:
            - USERNAME - Specifies the user name (always empty user name).
            - METHODNAME - Specifies the method name.
            - SAMLASSERTION - Specifies the SAML assertion.
            - SAMLUSER - Specifies the user name associated with the SAML assertion.
            - FINALDATA - Specifies the final data (this is empty).
            - SESSIONCOOKIE - Specifies the session cookie used for the reconnect.
        - SCRAMSHA256 Authentication:
            - USERNAME - Specifies the user name.
            - METHODNAME - Specifies the method name.
            - CLIENTCHALLENGE - Specifies the client challenge. (64 bytes)
            - SERVERCHALLENGEDATA - Specifies the server challenge.
                - SALT - Specifies the password salt.
                - SERVERCHALLENGE - Specifies the server challenge.
            - CLIENTPROOF - Specifies the client proof. (35 bytes)
                - SCRAMMESSAGE - Specifies the SCRAM HMAC message, the actual Client Proof that is sent to the server. (32 bytes)
            - SERVERPROOF - Specifies the server proof.
        - Session Cookie Authentication:
            - USERNAME - Specifies the user name.
            - METHODNAME - Specifies the method name.
            - SESSIONCOOKIE - Specifies the session cookie, process ID, and hostname.
            - SERVERREPLY - Specifies the server reply (this is empty).
            - FINALDATA - Specifies the final data (this is empty).
    """
    part_kind = 33
    name = "SAP HANA SQL Command Network Protocol Authentication Part"
    fields_desc = [
        FieldLenField("count", None, count_of="auth_fields", fmt="<H"),
        PacketListField("auth_fields", None, SAPHDBPartAuthenticationField, count_from=lambda x: x.count),
    ]


def saphdb_determine_part_class(pkt, lst, cur, remain):
    """Determines the class of the buffer elements based on the Part Kind value.
    """
    if pkt.partkind in hdb_part_kind_classes:
        return hdb_part_kind_classes[pkt.partkind]
    return Raw


class SAPHDBPart(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Part

    This packet represents a part within a HDB packet.

    The part header is comprised of 16 bytes.
    """
    name = "SAP HANA SQL Command Network Protocol Part"
    fields_desc = [
        EnumField("partkind", 0, hdb_partkind_values, fmt="<b"),
        LESignedByteField("partattributes", 0),
        FieldLenField("argumentcount", None, count_of="buffer", fmt="<h"),
        LESignedIntField("bigargumentcount", 0),
        FieldLenField("bufferlength", None, length_of="buffer", fmt="<i"),
        LESignedIntField("buffersize", 2**17 - 32 - 24),
        PadField(PacketListField("buffer", [], next_cls_cb=saphdb_determine_part_class,
                                 count_from=lambda x: x.argumentcount,
                                 length_from=lambda x: x.bufferlength), 8),
    ]


class SAPHDBSegment(PacketNoPadded):
    """SAP HANA SQL Command Network Protocol Segment

    This packet represents a segment within a HDB packet.

    The segment header is comprised of 24 byte, being the first 13 bytes always the same fields and the remaining 11
    bytes depend on the segment kind field.
    """
    name = "SAP HANA SQL Command Network Protocol Segment"
    fields_desc = [
        # Segment length needs to be calculated counting the segment header
        FieldLenField("segmentlength", None, length_of="parts", fmt="<i", adjust=lambda x, l:l+24),
        LESignedIntField("segmentofs", 0),
        FieldLenField("noofparts", None, count_of="parts", fmt="<h"),
        LESignedShortField("segmentno", 1),
        EnumField("segmentkind", 1, hdb_segmentkind_values, fmt="<b"),
        ConditionalField(EnumField("messagetype", 0, hdb_message_type_values, fmt="<b"), hdb_segment_is_request),
        ConditionalField(LESignedByteField("commit", 0), hdb_segment_is_request),
        ConditionalField(LESignedByteField("commandoptions", 0), hdb_segment_is_request),
        ConditionalField(LongField("reserved1", 0), hdb_segment_is_request),
        ConditionalField(ByteField("reserved2", 0), hdb_segment_is_reply),
        ConditionalField(EnumField("functioncode", 0, hdb_function_code_values, fmt="<h"), hdb_segment_is_reply),
        ConditionalField(LongField("reserved3", 0), hdb_segment_is_reply),
        ConditionalField(StrFixedLenField("reserved4", None, 11), lambda pkt: not (hdb_segment_is_reply(pkt) or hdb_segment_is_request(pkt))),
        PacketListField("parts", None, SAPHDBPart, count_from=lambda x: x.noofparts),
    ]


class SAPHDB(Packet):
    """SAP HANA SQL Command Network Protocol packet

    This packet is used for the HANA SQL Command Network Protocol.

    The message header is comprised of 32 bytes.
    """
    name = "SAP HANA SQL Command Network Protocol"
    fields_desc = [
        LESignedLongField("sessionid", -1),
        LESignedIntField("packetcount", 0),
        FieldLenField("varpartlength", None, length_of="segments", fmt="<I"),
        LEIntField("varpartsize", 2**17 - 32),
        FieldLenField("noofsegm", None, count_of="segments", fmt="<h"),
        EnumField("packetoptions", 0, hdb_packetoptions_values, fmt="b"),
        ByteField("reserved1", None),
        LEIntField("compressionvarpartlength", 0),
        IntField("reserved2", None),
        PacketListField("segments", None, SAPHDBSegment, count_from=lambda x: x.noofsegm),
    ]


class SAPHDBInitializationRequest(Packet):
    """SAP HANA SQL Command Network Protocol Initialization Request packet

    This packet is used for the HANA SQL Command Network Protocol during initialization.
    """
    name = "SAP HANA SQL Command Network Protocol Initialization Request"
    fields_desc = [
        StrFixedLenField("initialization", "\xff\xff\xff\xff\x04\x20\x00\x04\x01\x00\x00\x01\x01\x01", 14),
    ]


class SAPHDBInitializationReply(Packet):
    """SAP HANA SQL Command Network Protocol Initialization Reply packet

    This packet is used for the HANA SQL Command Network Protocol during initialization.
    """
    name = "SAP HANA SQL Command Network Protocol Initialization Reply"
    fields_desc = [
        LESignedByteField("product_major", 0),
        ShortField("product_minor", 0),
        LESignedByteField("protocol_major", 0),
        ShortField("protocol_minor", 0),
        ShortField("padding", 0),
    ]


class SAPHDBAuthenticationError(Exception):
    """SAP HDB Authentication exception"""


class SAPHDBAuthMethod(object):
    """SAP HDB Authentication Method

    This is the base class to define the authentication methods to use on
    a connection.
    """

    METHOD = None

    def __init__(self, username):
        """Initialize the authentication method.

        All `AUTHENTICATION` Parts require a field with the username, even if it's empty and not used such as in
        `SAML`, so it needs to be provided.

        :param username: username
        :type username: string
        """
        self.username = username
        self.session_cookie = None

    def craft_authentication_request(self, value=None, connection=None):
        """Craft the initial authentication request and returns the packet to send. If a connection is
        provided, it will include the Client Context part from it (e.g. application name).

        :param value: value to include in the authentication request
        :type value: string

        :param connection: `HDB` connection to get client context values if required
        :type connection: :class:`SAPHDBConnection`

        :return: the initial authentication request
        :rtype: :class:`SAPHDB`
        """
        auth_fields = SAPHDBPartAuthentication(auth_fields=[SAPHDBPartAuthenticationField(value=self.username),
                                                            SAPHDBPartAuthenticationField(value=self.METHOD),
                                                            SAPHDBPartAuthenticationField(value=value or "")])
        auth_part = SAPHDBPart(partkind=33, argumentcount=1, buffer=auth_fields)
        auth_segm = SAPHDBSegment(messagetype=65, parts=[auth_part])

        if connection:
            auth_segm.parts.insert(0, connection.craft_client_context_part())

        return SAPHDB(segments=[auth_segm])

    def craft_authentication_response_part(self, auth_response_part, value=None):
        """Craft the `AUTHENTICATION` Part to use as response during the authentication process.

        :param auth_response_part: Part obtained from the initial authentication response
        :type auth_response_part: :class:`SAPHDBPartAuthentication`

        :param value: value to include in the authentication response
        :type value: string

        :return: the authentication response part
        :rtype: :class:`SAPHDBPart`
        """
        auth_fields = SAPHDBPartAuthentication(auth_fields=[SAPHDBPartAuthenticationField(value=self.username),
                                                            SAPHDBPartAuthenticationField(value=self.METHOD),
                                                            SAPHDBPartAuthenticationField(value=value or "")])
        return SAPHDBPart(partkind=33, argumentcount=1, buffer=auth_fields)

    def authenticate(self, connection):
        """Method to authenticate the client connection. It performs the round trip with the server as required
        by the method implemented, and returns the `AUTHENTICATION` Part.

        :param connection: connection to the server
        :type connection: :class:`SAPHDBConnection`

        :return: authentication part to use in Connect packet
        :rtype: SAPHDBPart

        :raise: SAPHDBAuthenticationError
        """

        auth_request = self.craft_authentication_request(connection=connection)

        auth_response = connection.sr(auth_request)

        # Check if the response is an error
        if auth_response.segments[0].segmentkind == 5:  # If is Error segment kind
            raise SAPHDBAuthenticationError("Authentication failed")

        auth_response_part = auth_response.segments[0].parts[0].buffer[0]

        # Check the method replied by the server
        if self.METHOD != auth_response_part.auth_fields[0].value:
            raise SAPHDBAuthenticationError("Authentication method not supported on server")

        # Craft authentication part and return it
        return self.craft_authentication_response_part(auth_response_part)

    def process_connect_response(self, connect_reponse, connection=None):
        """Process the final response from the authentication process when needed, according to the authentication
        method.
        """
        if len(connect_reponse.segments) and len(connect_reponse.segments[0].parts) and \
           connect_reponse.segments[0].parts[0].partkind == 33 and \
           len(connect_reponse.segments[0].parts[0].buffer) and \
           len(connect_reponse.segments[0].parts[0].buffer[0].auth_fields) and \
           connect_reponse.segments[0].parts[0].buffer[0].auth_fields[0].value == self.METHOD:
            self.session_cookie = connect_reponse.segments[0].parts[0].buffer[0].auth_fields[1].value


class SAPHDBAuthScramMethod(SAPHDBAuthMethod):
    """SAP HDB Authentication using a SCRAM-based algorithm

    This base class is used to abstract the common implementation across different
    algorithms.
    """

    SCRAM_CLASS = None

    def __init__(self, username, password):
        super(SAPHDBAuthScramMethod, self).__init__(username)
        self.password = password
        self.client_key = self.scram = None

    def obtain_client_proof(self, scram, client_key, auth_response_part):
        """Calculates the client proof with the salt and server key obtained from the authentication
        response part.
        """
        # Obtain the salt and the server key from the response
        method_parts = SAPHDBPartAuthentication(auth_response_part.auth_fields[1].value)
        salt = method_parts.auth_fields[0].value
        server_key = method_parts.auth_fields[1].value

        # Calculate the client proof from the password, salt and the server and client key
        # TODO: It might be good to see if this can be moved into a new Packet
        # TODO: We're only considering one server key
        client_proof = b"\x00\x01" + struct.pack('b', scram.CLIENT_PROOF_SIZE)
        client_proof += scram.scramble_salt(self.password, salt, server_key, client_key)

        return client_proof

    def craft_authentication_request(self, value=None, connection=None):
        """Instantiates the SCRAM class and craft the authentication request.
        """
        if value is None:
            self.scram = self.SCRAM_CLASS(default_backend())
            # Craft and send the authentication packet
            self.client_key = self.scram.get_client_key()
            value = self.client_key

        return super(SAPHDBAuthScramMethod, self).craft_authentication_request(value, connection)

    def craft_authentication_response_part(self, auth_response_part, value=None):
        """Calculates the client proof and craft the authentication response part.
        """
        value = self.obtain_client_proof(self.scram, self.client_key, auth_response_part)
        return super(SAPHDBAuthScramMethod, self).craft_authentication_response_part(auth_response_part, value)


class SAPHDBAuthScramSHA256Method(SAPHDBAuthScramMethod):
    """SAP HDB Authentication using SCRAM-SHA256 algorithm.
    """
    METHOD = "SCRAMSHA256"
    SCRAM_CLASS = SCRAM_SHA256


class SAPHDBAuthScramPBKDF2SHA256Method(SAPHDBAuthScramMethod):
    """SAP HDB Authentication using SCRAM-PBKDF2-SHA256 algorithm.
    """
    METHOD = "SCRAMPBKDF2SHA256"
    SCRAM_CLASS = SCRAM_PBKDF2SHA256

    def obtain_client_proof(self, scram, client_key, auth_response_part):
        # Obtain the salt, the server key and the number of rounds from the response
        method_parts = SAPHDBPartAuthentication(auth_response_part.auth_fields[1].value)
        salt = method_parts.auth_fields[0].value
        server_key = method_parts.auth_fields[1].value
        rounds, = struct.unpack('>I', method_parts.auth_fields[2].value)

        # Calculate the client proof from the password, salt, rounds and the server and client key
        # TODO: It might be good to see if this can be moved into a new Packet
        # TODO: We're only considering one server key
        client_proof = b"\x00\x01" + struct.pack('b', scram.CLIENT_PROOF_SIZE)
        client_proof += scram.scramble_salt(self.password, salt, server_key, client_key, rounds)
        return client_proof


class SAPHDBAuthSessionCookieMethod(SAPHDBAuthMethod):
    """SAP HDB Authentication using a Session Cookie.
    """

    METHOD = "SessionCookie"

    def __init__(self, username, session_cookie):
        super(SAPHDBAuthSessionCookieMethod, self).__init__(username)
        self.session_cookie = session_cookie

    def craft_authentication_request(self, value=None, connection=None):
        return super(SAPHDBAuthSessionCookieMethod, self).craft_authentication_request(self.session_cookie + connection.client_id,
                                                                                       connection)


class SAPHDBAuthJWTMethod(SAPHDBAuthMethod):
    """SAP HDB Authentication using JWT (JSON Web Token).
    """

    METHOD = "JWT"

    def __init__(self, username, jwt):
        super(SAPHDBAuthJWTMethod, self).__init__(username)
        self.jwt = jwt
        self.session_cookie = None

    def craft_authentication_request(self, value=None, connection=None):
        return super(SAPHDBAuthJWTMethod, self).craft_authentication_request(self.jwt, connection)


class SAPHDBAuthSAMLMethod(SAPHDBAuthMethod):
    """SAP HDB Authentication using Security Assertion Markup Language (SAML) 2.0 Bearer Assertions.
    """

    METHOD = "SAML"

    def __init__(self, username, saml_assertion):
        super(SAPHDBAuthSAMLMethod, self).__init__(username)
        self.saml_assertion = saml_assertion
        self.session_cookie = None

    def craft_authentication_request(self, value=None, connection=None):
        """The initial authentication request should be performed as usual in other methods,
        with an empty username and the entire SAML Assertion or SAML Response as the value.
        """
        return super(SAPHDBAuthSAMLMethod, self).craft_authentication_request(self.saml_assertion, connection)

    def craft_authentication_response_part(self, auth_response_part, value=None):
        """In SAML, the first round trip with the server returns the SAML username to use when
        authentication, so we must set it at this point.
        """
        self.username = auth_response_part.auth_fields[1].value
        return super(SAPHDBAuthSAMLMethod, self).craft_authentication_response_part(auth_response_part, value)


class SAPHDBAuthGSSMethod(SAPHDBAuthMethod):
    """SAP HDB Authentication using GSS.
    """

    METHOD = "GSS"

    KRB5OID_KERBEROS5 = "1.2.840.113554.1.2.2"
    TYPEOID_GSS_KRB5_NT_PRINCIPAL_NAME = "1.2.840.113554.1.2.2.1"
    TYPEOID_GSS_KRB5_NT_PRINCIPAL_NAME_pre_RFC_1964 = "1.2.840.113554.1.2.2.2"

    def __init__(self, username, krb5ticket=None, krb5ticket_callback=None, krb5oid=None, typeoid=None):
        super(SAPHDBAuthGSSMethod, self).__init__(username)
        self.krb5ticket = krb5ticket
        self.krb5ticket_callback = krb5ticket_callback
        self.krb5oid = krb5oid or self.KRB5OID_KERBEROS5
        self.typeoid = typeoid or self.TYPEOID_GSS_KRB5_NT_PRINCIPAL_NAME
        self.session_cookie = None

    def craft_authentication_request(self, value=None, connection=None):
        """The authentication requests contains an empty username, the method, and a GSS token.

        In most of the cases this GSS token is a SPNego structure, although the HANA implementation
        doesn't use the standard ASN.1 encoding and instead leverage the same Authentication Field
        format.
        """
        auth_fields = SAPHDBPartAuthentication(auth_fields=[SAPHDBPartAuthenticationField(value=""),
                                                            SAPHDBPartAuthenticationField(value=self.METHOD),
                                                            SAPHDBPartAuthenticationField(value=value)])
        auth_part = SAPHDBPart(partkind=33, argumentcount=1, buffer=auth_fields)
        auth_segm = SAPHDBSegment(messagetype=65, parts=[auth_part])

        if connection:
            auth_segm.parts.insert(0, connection.craft_client_context_part())

        return SAPHDB(segments=[auth_segm])

    def craft_authentication_response_part(self, auth_response_part, value=None):
        """In GSS, the final round trip with the server returns a GSS token that is mech specific.

        In the case of Kerberos, the reply to the final roundtrip before the CONNECT contains an
        GSSAPI KRB5 AP-REP structure. At this stage we're not parsing nor validating it.

        The part to include in the CONNECT message is just a confirmation of the authentication handled.
        Note that this CONNECT packet has the username specified as in all other auth mechanisms.
        """
        # gss_token_response = SAPHDBPartAuthentication(auth_response_part.auth_fields[1].value)

        last_gss_token = SAPHDBPartAuthentication(auth_fields=[SAPHDBPartAuthenticationField(value=self.krb5oid),
                                                               SAPHDBPartAuthenticationField(value="\x05")])
        return super(SAPHDBAuthGSSMethod, self).craft_authentication_response_part(auth_response_part, last_gss_token)

    def authenticate(self, connection):
        """Method to authenticate the client connection. It performs the round trip with the server as required
        by the method implemented, and returns the `AUTHENTICATION` Part.

        :param connection: connection to the server
        :type connection: :class:`SAPHDBConnection`

        :return: authentication part to use in Connect packet
        :rtype: SAPHDBPart

        :raise: SAPHDBAuthenticationError
        """

        # The initial GSS token includes the NegTokenInit structure:
        #  * krb5oid: GSS mechs to use
        #  * commtype: communication type ("\x01")
        #  * typeoid: type of the client GSS name
        #  * gssname: the client GSS name (e.g. UPN)
        first_gss_token = SAPHDBPartAuthentication(auth_fields=[SAPHDBPartAuthenticationField(value=self.krb5oid),
                                                                SAPHDBPartAuthenticationField(value="\x01"),
                                                                SAPHDBPartAuthenticationField(value=self.typeoid),
                                                                SAPHDBPartAuthenticationField(value=self.username)])
        first_auth_request = self.craft_authentication_request(first_gss_token, connection=connection)
        first_auth_response = connection.sr(first_auth_request)

        # Check if the response is an error
        if first_auth_response.segments[0].segmentkind == 5:  # If is Error segment kind
            raise SAPHDBAuthenticationError("Authentication failed")

        first_auth_response_part = first_auth_response.segments[0].parts[0].buffer[0]
        # Check the method replied by the server
        if self.METHOD != first_auth_response_part.auth_fields[0].value:
            raise SAPHDBAuthenticationError("Authentication method not supported on server")

        # The initial response from the server includes the NegTokenResp structure:
        #  * krb5oid: GSS mechs to use
        #  * commtype: communication type ("\x02")
        #  * typeoid: type of the client GSS name
        #  * spn: SPN to ask the KDC the ticket for
        #  * username: database username mapped from the UPN
        initial_gss_token = SAPHDBPartAuthentication(first_auth_response_part.auth_fields[1].value)

        # We either assume the GSSAPI KRB5 AP-REQ structure was already provided, or use a callback
        # to obtain it based on the SPN, the UPN and the database username mapped by the server.
        if self.krb5ticket:
            krb5ticket = self.krb5ticket
        else:
            try:
                krb5ticket = self.krb5ticket_callback(initial_gss_token.auth_fields[3].value,
                                                      self.username,
                                                      initial_gss_token.auth_fields[4].value)
            except Exception:
                raise SAPHDBAuthenticationError("Unable to obtain a Kerberos ticket to authenticate")

        # The second GSS token includes the GSSAPI KRB5 AP-REQ structure:
        #  * krb5oid: GSS mechs to use
        #  * commtype: communication type ("\x03")
        #  * krb5ticket: the GSSAPI KRB5 AP-REQ structure to use
        second_gss_value = SAPHDBPartAuthentication(auth_fields=[SAPHDBPartAuthenticationField(value=self.krb5oid),
                                                                 SAPHDBPartAuthenticationField(value="\x03"),
                                                                 SAPHDBPartAuthenticationField(value=krb5ticket)])
        second_auth_request = self.craft_authentication_request(second_gss_value, connection=connection)
        second_auth_response = connection.sr(second_auth_request)

        # Check if the response is an error
        if second_auth_response.segments[0].segmentkind == 5:  # If is Error segment kind
            raise SAPHDBAuthenticationError("Authentication failed")

        second_auth_response_part = second_auth_response.segments[0].parts[0].buffer[0]

        # Check the method replied by the server
        if self.METHOD != second_auth_response_part.auth_fields[0].value:
            raise SAPHDBAuthenticationError("Authentication method not supported on server")

        # Craft authentication part and return it
        return self.craft_authentication_response_part(second_auth_response_part)

    def process_connect_response(self, connect_reponse, connection=None):
        """Process the final response from the authentication process when needed, according to the authentication
        method.
        """
        super(SAPHDBAuthGSSMethod, self).process_connect_response(connect_reponse, connection)
        if self.session_cookie is not None:
            # The final response from the server includes the SessionCookie established:
            #  * krb5oid: GSS mechs to use
            #  * commtype: communication type ("\x07")
            #  * session cookie: the SessionCookie established for the connection
            gss_token = SAPHDBPartAuthentication(self.session_cookie)
            if gss_token.auth_fields[1].value == "\x07":
                self.session_cookie = gss_token.auth_fields[2].value
            else:
                self.session_cookie = None


saphdb_auth_methods = {
    "GSS": SAPHDBAuthGSSMethod,
    "JWT": SAPHDBAuthJWTMethod,
    "SAML": SAPHDBAuthSAMLMethod,
    "SCRAMSHA256": SAPHDBAuthScramSHA256Method,
    "SCRAMPBKDF2SHA256": SAPHDBAuthScramPBKDF2SHA256Method,
    "SessionCookie": SAPHDBAuthSessionCookieMethod,
}
"""SAP HDB Authentication Methods Implemented"""


class SAPHDBConnectionError(Exception):
    """SAP HDB Connection exception
    """


class SAPHDBConnection(object):
    """SAP HDB Connection

    This class represents a basic client connection to a HANA server using
    the SQL Command Network Protocol.
    """

    def __init__(self, host, port, auth_method=None, route=None, pid=None, hostname=None,
                 client_version=None, client_type=None, app_name=None):
        """Creates the connection to the HANA server.

        :param host: remote host to connect to
        :type host: C{string}

        :param port: remote port to connect to
        :type port: ``int``

        :param auth_method: authentication method to use when connecting
        :type auth_method: :class:`SAPHDBAuthMethod`

        :param route: route to use for connecting through a SAP Router
        :type route: C{string}
        """
        self.host = host
        self.port = port
        self.auth_method = auth_method
        self.route = route
        self.pid = pid or "0"
        self.hostname = hostname or socket.gethostname()
        self.client_version = client_version or pysap.__version__
        self.client_type = client_type or "SQLODBC"
        self.app_name = app_name or "pysap"
        self._stream_socket = None
        self.product_version = None
        self.protocol_version = None

    @property
    def client_id(self):
        """Returns the Client Id to use when authenticating and connecting to the
        server. If the PID and the hostname are not specified, by default uses '0@<hostname>'.

        :return: Client ID string
        :rtype: string
        """
        pid = self.pid
        hostname = self.hostname
        return "{}@{}".format(pid, hostname)

    def craft_client_context_part(self):
        """Crafts the client context part that is sent to the server. It contains the
        information about the client version, type and application name.
        If those values are not specified, by default uses pysap's version.

        :return: Client Context Part
        :rtype: :class:`SAPHDBPart`
        """
        client_context = [SAPHDBPartClientContext(key=1, type=29, value=self.client_version),
                          SAPHDBPartClientContext(key=2, type=29, value=self.client_type),
                          SAPHDBPartClientContext(key=3, type=29, value=self.app_name)]
        return SAPHDBPart(partkind=29, buffer=client_context)

    def connect(self):
        """Creates a :class:`SAPNIStreamSocket` connection to the host/port. If a route
        was specified, connect to the target HANA server through the SAP Router.

        :raises: SAPHDBConnectionError
        """
        try:
            self._stream_socket = SAPRoutedStreamSocket.get_nisocket(self.host,
                                                                     self.port,
                                                                     self.route,
                                                                     base_cls=SAPHDB,
                                                                     talk_mode=1)
        except socket.error as e:
            raise SAPHDBConnectionError("Error connecting to the server (%s)" % e)

    def is_connected(self):
        """Returns if the underlying socket is connected.

        :return: If the underlying socket is connected.
        :rtype: bool
        """
        return self._stream_socket is not None

    def send(self, message):
        """Sends a packet to the server
        """
        if not self.is_connected():
            raise SAPHDBConnectionError("Socket not ready")
        self._stream_socket.send(message)

    def sr(self, message):
        """Sends a packet to the server and receives a response."""
        if not self.is_connected():
            raise SAPHDBConnectionError("Socket not ready")
        self.send(message)
        return self.recv()

    def recv(self):
        """Receives a packet from the server.

        As the length of the entire packet is not known, it first tries to get the 32-bytes header and obtain
        the variable length from it.

        :return: the received packet
        :rtype: :class:`SAPHDB`
        """
        if not self.is_connected():
            raise SAPHDBConnectionError("Socket not ready")
        # First we receive the header to obtain the variable length field
        header_raw = self._stream_socket.ins.recv(32)
        header = SAPHDB(header_raw)
        # Then get the payload
        payload = ""
        if header.varpartlength > 0:
            payload = self._stream_socket.ins.recv(header.varpartlength)
        # And finally construct the whole packet with header plus payload
        return SAPHDB(header_raw + payload)

    def initialize(self):
        """Initializes the connection with the server.
        """
        if not self.is_connected():
            raise SAPHDBConnectionError("Socket not ready")

        if self.product_version is not None and self.protocol_version is not None:
            return

        # Send initialization request packet
        init_request = SAPHDBInitializationRequest()
        self.send(init_request)

        # Receive initialization response packet
        init_reply = SAPHDBInitializationReply(self._stream_socket.recv(8))  # We use the raw socket recv here
        self.product_version = init_reply.product_major
        self.protocol_version = init_reply.protocol_major

    def authenticate(self):
        """Authenticates the connection against the server using the selected method.

        :raises: SAPHDBAuthenticationError
        """
        if not self.is_connected():
            raise SAPHDBConnectionError("Socket not ready")

        # Perform the authentication handshake and obtain the final authentication part.
        # Note that this might involve a series of round trips depending on the authentication
        # method.
        auth_part = self.auth_method.authenticate(self)

        # Craft the CONNECT packet
        clientcontext_part = SAPHDBPart(partkind=29)
        clientid_part = SAPHDBPart(partkind=35, buffer=SAPHDBPartClientId(clientid=self.client_id))
        connect_segm = SAPHDBSegment(messagetype=66, parts=[clientcontext_part, auth_part, clientid_part])
        connect_request = SAPHDB(segments=[connect_segm])

        # Send connect packet
        connect_response = self.sr(connect_request)

        if connect_response.segments[0].segmentkind == 5:  # If is Error segment kind
            self.close_socket()
            raise SAPHDBAuthenticationError("Authentication failed")

        # Process the connect response
        self.auth_method.process_connect_response(connect_response, connection=self)

    def connect_authenticate(self):
        """Connects to the server, performs initialization and authenticates the client.
        """
        if not self.is_connected():
            self.connect()
        self.initialize()
        self.authenticate()

    def close(self):
        """Closes the connection with the server

        :raise: SAPHDBConnectionError
        """
        if not self.is_connected():
            raise SAPHDBConnectionError("Connection already closed")

        try:
            disconnect_segm = SAPHDBSegment(messagetype=77)
            disconnect_request = SAPHDB(segments=[disconnect_segm])

            # Send disconnect packet and check the response
            disconnect_response = self.sr(disconnect_request)
            if not hdb_segment_is_reply(disconnect_response.segments[0]) or \
               disconnect_response.segments[0].functioncode != 18:
                raise SAPHDBConnectionError("Connection incorrectly closed")

        except socket.error as e:
            raise SAPHDBConnectionError("Error closing the connection to the server (%s)" % e)

        finally:
            self.close_socket()

    def close_socket(self):
        """Closes the underlaying socket of the connection
        """
        self._stream_socket.close()
        self._stream_socket = None


class SAPHDBTLSConnection(SAPHDBConnection):
    """SAP HDB Connection using TLS

    This class wraps the connection socket with a TLS-enabled one for use as a secure channel.
    We're using TLS 1.2 by default but setting RSA-based cipher suites without (EC)DHE so traffic
    can be easily decrypted in Wireshark.
    """

    TLS_CERT_TRUST = False
    TLS_CERT_FILE = None
    TLS_CHECK_HOSTNAME = False
    TLS_DEFAULT_PROTOCOL = ssl.PROTOCOL_TLS
    TLS_DEFAULT_OPTIONS = ssl.OP_NO_TLSv1 | ssl.OP_NO_TLSv1_1
    TLS_DEFAULT_CIPHERS = "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256:AES256-GCM-SHA384:AES256-CCM8"

    def __init__(self, host, port, auth_method=None, route=None, pid=None, hostname=None,
                 client_version=None, client_type=None, app_name=None, tls_cert_trust=None,
                 tls_cert_file=None, tls_check_hostname=None, tls_protocol=None,
                 tls_options=None, tls_ciphers=None):
        super(SAPHDBTLSConnection, self).__init__(host, port, auth_method, route, pid, hostname,
                                                  client_version, client_type, app_name)
        # Get TLS related parameters or set default values.
        self.tls_cert_trust = tls_cert_trust or self.TLS_CERT_TRUST
        self.tls_cert_file = tls_cert_file or self.TLS_CERT_FILE
        self.tls_check_hostname = tls_check_hostname or self.TLS_CHECK_HOSTNAME
        self.tls_protocol = tls_protocol or self.TLS_DEFAULT_PROTOCOL
        self.tls_options = tls_options or self.TLS_DEFAULT_OPTIONS
        self.tls_ciphers = tls_ciphers or self.TLS_DEFAULT_CIPHERS

    def connect(self):
        # Create a plain socket first
        plain_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        plain_socket.settimeout(10)

        # Create the TLS/SSL Context. We first set the options as specified.
        context = ssl.SSLContext(self.tls_protocol)
        context.options |= self.tls_options
        if self.tls_ciphers:
            context.set_ciphers(self.tls_ciphers)

        # Then set the certificate validation and the path to the certificate(s) if required.
        if self.tls_cert_trust:
            context.verify_mode = ssl.CERT_NONE
        else:
            context.verify_mode = ssl.CERT_REQUIRED
            context.check_hostname = self.tls_check_hostname

        if self.tls_cert_file:
            context.load_verify_locations(cafile=self.tls_cert_file)
        else:
            context.set_default_verify_paths()

        # Wrap the plain socket in a TLS/SSL one
        tls_socket = context.wrap_socket(plain_socket, server_hostname=self.host)
        tls_socket.connect((self.host, self.port))

        # Create the stream socket from the TLS/SSL one. From here treatment should be similar to a plain one.
        self._stream_socket = SSLStreamSocket(tls_socket, basecls=SAPHDB)


# Bind SAP NI with the HDB ports
bind_layers(TCP, SAPHDB, dport=30013)
bind_layers(TCP, SAPHDB, dport=30015)
