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

# External imports
from scapy.layers.inet import TCP
from scapy.packet import Packet, bind_layers
from scapy.fields import (ByteField, ConditionalField, IPField, IntField,
                          StrFixedLenField)
# Custom imports
from pysap.SAPNI import SAPNI
from pysap.utils import (SignedShortField, ByteEnumKeysField,
                         StrFixedLenPaddedField)
from scapy.layers.inet6 import IP6Field


# RFC Request Type values
rfc_req_type_values = {
    0x00: "GW_UNDEF_TYPE",
    0x01: "CHECK_GATEWAY",
    0x02: "GW_CONNECT_GWWP",
    0x03: "GW_NORMAL_CLIENT",
    0x04: "GW_REMOTE_GATEWAY",
    0x05: "STOP_GATEWAY",
    0x06: "GW_LOCAL_R3",
    0x07: "GW_SEND_INTERNAL_ERROR",  # Requires NiLocalCheck
    0x08: "GW_SEND_INFO",
    0x09: "GW_SEND_CMD",
    0x0a: "GW_WORKPROCESS_DIED",  # Requires NiLocalCheck
    0x0b: "GW_REGISTER_TP",
    0x0c: "GW_UNREGISTER_TP",
    0x0d: "GW_CONNECT_DISP",  # Requires NiLocalCheck
    0x0e: "GW_GET_NO_REGISTER_TP",
    0x0f: "GW_SAP_WP_CLIENT",  # Requires NiLocalCheck
    0x10: "GW_CANCEL_REGISTER_TP",
    0x11: "REMOTE_GATEWAY",
    0x12: "GW_CONTAINER_RECEIVED",
}
"""RFC Request Type values"""

# RFC Monitor Command values
rfc_monitor_cmd_values = {
    0x01: "NOOP",
    0x02: "DELETE_CONN",
    0x03: "CANCEL_CONN",
    0x04: "RST_SINGLE_ERR_CNT",
    0x05: "RST_ALL_ERR_CNT",
    0x06: "INCREASE_TRACE",
    0x07: "DECREASE_TRACE",
    0x08: "READ_SEC_INFO",
    0x09: "REFRESH_SEC_INFO",
    0x0a: "READ_GWSYS_TBL",
    0x0b: "READ_CONN_TBL",
    0x0c: "READ_PROC_TBL",
    0x0d: "READ_CONN_ATTR",
    0x0e: "READ_MEMORY",
    0x0f: "READ_REQ_BLK",
    0x10: "ACT_STATISTIC",
    0x11: "DEACT_STATISTIC",
    0x12: "READ_STATISTIC",
    0x13: "RESET_STATISTIC",
    0x14: "READ_PARAMETER",
    0x19: "DUMP_NIBUFFER",
    0x20: "RESET_NIBUFFER",
    0x21: "ACT_EXTPGM_TRACE",
    0x22: "DEACT_EXTPGM_TRACE",
    0x23: "ACT_CONN_TRACE",
    0x24: "DEACT_CONN_TRACE",
    0x25: "RESET_TRACE",
    0x26: "SUICIDE",
    0x27: "READ_SEC_INFO2",
    0x28: "CANCEL_REG_TP",
    0x29: "DUMP",
    0x2a: "READ_GWSYS_TBL2",
    0x2b: "CHANGE_PARAMETER",
    0x2c: "GET_CONN_PARTNER",
    0x2d: "DELETE_CLIENT",
    0x2e: "DELETE_REMGW",
    0x2f: "DISCONNECT",
    0x30: "ENABLE_RESTART",
    0x31: "DISABLE_RESTART",
    0x32: "NI_TRACE",
    0x33: "CLI_INFO",
    0x34: "GW_INFO",
    0x35: "CONVID_INFO",
    0x36: "GET_NO_REG_TP",
    0x37: "CV_INFO",
    0x38: "SO_KEEPALIVE",
    0x39: "READ_CONN_TBL2",
    0x40: "READ_GWSYS_TBL3",
    0x41: "RELOAD_ACL",
}
"""RFC Monitor Command values"""


# APPC Header versions length:
# 1: 4Ch
# 2/3: 64h
# 4: 8Ah
# 5: 4Eh
# 6: 50h

class SAPRFC(Packet):
    """SAP Remote Function Call packet

    This packet is used for the Remote Function Call (RFC) protocol.
    """
    name = "SAP Remote Function Call"
    fields_desc = [
        ByteField("version", 3),  # If the version is 3, the packet has a size > 88h, versions 1 and 2 are 40h
        ByteEnumKeysField("req_type", 0, rfc_req_type_values),

        # Normal client fields (GW_NORMAL_CLIENT)
        ConditionalField(IPField("address", "0.0.0.0"), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(IntField("padd1", 0), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("service", "", length=10), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(StrFixedLenField("codepage", "1100", length=4), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(StrFixedLenField("padd2", "\x00" * 6, length=6), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("lu", "", length=8), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("tp", "", length=8), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("conversation_id", "", length=8), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(ByteField("appc_header_version", 6), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(ByteField("accept_info", 0xcb), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(SignedShortField("idx", -1), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(IP6Field("address6", "::"), lambda pkt:pkt.req_type == 0x03 and pkt.version == 3),

        ConditionalField(IntField("rc", 0), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(ByteField("echo_data", 0), lambda pkt:pkt.req_type == 0x03),
        ConditionalField(ByteField("filler", 0), lambda pkt:pkt.req_type == 0x03),

        # Monitor Command fields (GW_SEND_CMD)
        ConditionalField(ByteEnumKeysField("cmd", 0, rfc_monitor_cmd_values), lambda pkt:pkt.req_type == 0x09),
    ]


# Bind SAP NI with the RFC port
bind_layers(TCP, SAPNI, dport=3300)
