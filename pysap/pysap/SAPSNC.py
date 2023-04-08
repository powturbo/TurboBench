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
from scapy.fields import (ByteField, ShortField, IntField, StrFixedLenField, FieldLenField, StrLenField,
                          ConditionalField, ByteEnumKeysField, ShortEnumKeysField)
# Custom imports
from pysap.utils.fields import (PacketNoPadded)


# SNC Frame Types
snc_frame_type = {
    0x00: "REVERSE_REQ",
    0x01: "INIT_REQ",
    0x02: "INIT",
    0x03: "INIT_ACK",
    0x04: "ACCEPT",
    0x05: "ACCEPT_ACK",
    0x06: "ACCEPT_FAILED",
    0x07: "DATA_OPEN",
    0x08: "DATA_MIC/DATA_SIGNED",
    0x09: "DATA_WRAP/DATA_SEALED",
    0x0a: "SHUTDOWN",
    0x0b: "SHUTDOWN_MSG",
    0x0c: "REJECTED",
    0x0d: "ERROR",
    0x0e: "UNKNOWN",
}

# SNC Mech ID values
snc_mech_id_values = {
    0x00: "No security",
    0x01: "Generic GSS-API v2 Mechanism",
    0x02: "Kerberos 5/GSS-API v2",
    0x03: "Secude 5 GSS-API v2",
    0x04: "SAP's GSS-API v2 over NTLM(SSPI)",
    0x05: "SPKM1 GSS-API v2 library",
    0x06: "SPKM2 GSS-API v2 library",
    0x07: "reserved ID",
    0x08: "itsec",
    0x09: "SDTI Connect Agent",
    0x0a: "AccessMaster DCE",
}

# SNC Quality of protection values
snc_qop = {
    0x00: "INVALID",
    0x01: "OPEN",
    0x02: "INTEGRITY/SIGNED",
    0x03: "PRIVACY/SEALED",
    0x07: "MIN",
    0x08: "DEFAULT",
    0x09: "MAX",
}


class SAPSNCFrame(PacketNoPadded):
    """SAP SNC Frame packet

    This packet is used to contain and wrap SNC Frames.
    """
    name = "SAP SNC Frame"
    fields_desc = [
        StrFixedLenField("eye_catcher", "SNCFRAME", 8),
        ByteEnumKeysField("frame_type", 2, snc_frame_type),
        ByteField("protocol_version", 5),
        ShortField("header_length", 24),
        FieldLenField("token_length", 0, length_of="token", fmt="I"),
        FieldLenField("data_length", 0, length_of="data", fmt="I"),
        ShortEnumKeysField("mech_id", 3, snc_mech_id_values),
        ShortField("flags", 0),
        ConditionalField(IntField("ext_flags", 0), lambda pkt: pkt.header_length > 24),
        ConditionalField(FieldLenField("ext_field_length", 0, length_of="ext_fields", fmt="!H"), lambda pkt: pkt.header_length > 24),
        ConditionalField(StrLenField("ext_fields", "", length_from=lambda pkt:pkt.ext_field_length), lambda pkt: pkt.header_length > 24),
        StrLenField("token", "", length_from=lambda pkt:pkt.token_length),
        StrLenField("data", "", length_from=lambda pkt:pkt.data_length),
    ]


def unwrap_snc(s, offset):
    """Unwraps an SNC frame if it's possible. This is, if the frame type is
    data open or signed but not encrypted. Appends the unwrapped data to the
    end of the string being dissected and adjust the offset properly to skip
    the SNC frame.

    :param s: string to dissect
    :type s: C{string}

    :param offset: offset where the SNC frame starts
    :type offset: int

    :return: tuple of string with the SNC frame wrapped, adjusted offset
    :rtype: tuple of C{string}, int
    """

    snc_frame = SAPSNCFrame(s[offset:])
    if snc_frame.frame_type in [0x07, 0x08] and snc_frame.data_length > 0:
        s = s + snc_frame.data
        offset += snc_frame.header_length + snc_frame.token_length + snc_frame.data_length

    return s, offset


def wrap_snc(s, offset, data):
    """Wraps an SNC frame if it's possible. This is, if the frame type is
    data open. Adds the data to the frame and properly updates the length
    field on the SNC frame.

    :param s: string to dissect
    :type s: string

    :param offset: offset where the SNC frame starts
    :type offset: int

    :param data: data to wrap
    :type data: string

    :return: string with the SNC frame wrapped
    :rtype: string
    """

    snc_frame = SAPSNCFrame(s[offset:])
    if snc_frame.frame_type == 0x07:
        snc_frame.data = data
        snc_frame.data_length = len(data)
        snc_frame_length = snc_frame.header_length + snc_frame.token_length + snc_frame.data_length
        s = s[:offset] + str(snc_frame)[:snc_frame_length]

    return s
