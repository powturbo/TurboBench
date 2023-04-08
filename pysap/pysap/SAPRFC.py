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
from scapy.packet import bind_layers
from scapy.fields import (ByteField, ConditionalField, IPField, IntField,
                          StrFixedLenField, SignedShortField, ShortField,
                          ByteEnumKeysField, IntEnumKeysField, SignedIntField,
                          FieldLenField, StrLenField, FlagsField, PacketField,
                          StrField, PacketListField)
# External imports
from scapy.layers.inet6 import IP6Field
# Custom imports
from pysap.SAPNI import SAPNI
from pysap.utils.fields import StrFixedLenPaddedField, PacketNoPadded


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
    0x11: "GW_FROM_REMOTE_GATEWAY",
    0x12: "GW_CONTAINER_RECEIVED",
}


rfc_func_type_values = {
    0: "F_NO_REQUEST",
    1: "F_INITIALIZE_CONVERSATION",
    3: "F_ACCEPT_CONVERSATION",
    5: "F_ALLOCATE",
    7: "F_SEND_DATA",
    8: "F_ASEND_DATA",
    9: "F_RECEIVE",
    10: "F_ARECEIVE",
    11: "F_DEALLOCATE",
    13: "F_SET_TP_NAME",
    15: "F_SET_PARTNER_LU_NAME",
    17: "F_SET_SECURITY_PASSWORD",
    19: "F_SET_SECURITY_USER_ID",
    21: "F_SET_SECURITY_TYPE",
    23: "F_SET_CONVERSATION_TYPE",
    25: "F_EXTRACT_TP_NAME",
    27: "F_FLUSH",
    0xc9: "F_SAP_ALLOCATE",
    0xca: "F_SAP_INIT",
    0xcb: "F_SAP_SEND",
    0xcc: "F_ASAP_SEND",
    0xcd: "F_SAP_SYNC",
    0xce: "F_SAP_PING",
    0xcf: "F_SAP_REGTP",
    0xd0: "F_SAP_UNREGTP",
    0xd1: "F_SAP_ACCPTP",
    0xd2: "F_SAP_UNACCPTP",
    0xd3: "F_SAP_CANCTP",
    0xd4: "F_SAP_SET_UID",
    0xd5: "F_SAP_CANCEL",
    0xd6: "F_SAP_CANCELED",
    0xd7: "F_SAP_STOP_STREAMING",
    0xd8: "F_SAP_CONT_STREAMING",
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


appc_protocol_values = {
    0x3: "CPIC",
}
"""RFC APPC Protocol values"""


appc_rc_values = {
    0x0: "CM_OK",
    0x1: "CM_ALLOCATE_FAILURE_NO_RETRY",
    0x2: "CM_ALLOCATE_FAILURE_RETRY",
    0x3: "CM_CONVERSATION_TYPE_MISMATCH",
    0x5: "CM_PIP_NOT_SPECIFIED_CORRECTLY",
    0x6: "CM_SECURITY_NOT_VALID",
    0x7: "CM_SYNC_LVL_NOT_SUPPORTED_SYS",
    0x8: "CM_SYNC_LVL_NOT_SUPPORTED_PGM",
    0x9: "CM_TPN_NOT_RECOGNIZED",
    0xa: "CM_TP_NOT_AVAILABLE_NO_RETRY",
    0xb: "CM_TP_NOT_AVAILABLE_RETRY",
    0x11: "CM_DEALLOCATED_ABEND",
    0x12: "CM_DEALLOCATED_NORMAL",
    0x13: "CM_PARAMETER_ERROR",
    0x14: "CM_PRODUCT_SPECIFIC_ERROR",
    0x15: "CM_PROGRAM_ERROR_NO_TRUNC",
    0x16: "CM_PROGRAM_ERROR_PURGING",
    0x17: "CM_PROGRAM_ERROR_TRUNC",
    0x18: "CM_PROGRAM_PARAMETER_CHECK",
    0x19: "CM_PROGRAM_STATE_CHECK",
    0x1a: "CM_RESOURCE_FAILURE_NO_RETRY",
    0x1b: "CM_RESOURCE_FAILURE_RERTY",
    0x1c: "CM_UNSUCCESSFUL",
    0x1e: "CM_DEALLOCATED_ABEND_SVC",
    0x1f: "CM_DEALLOCATED_ABEND_TIMER",
    0x20: "CM_SVC_ERROR_NO_TRUNC",
    0x21: "CM_SVC_ERROR_PURGING",
    0x22: "CM_SVC_ERROR_TRUNC",
    0x23: "CM_OPERATION_INCOMPLETE",
    0x24: "CM_SYSTEM_EVENT",
    0x25: "CM_OPERATION_NOT_ACCEPTED",
    0x26: "CM_CONVERSATION_ENDING",
    0x27: "CM_SEND_RCV_MODE_NOT_SUPPORTED",
    0x28: "CM_BUFFER_TOO_SMALL",
    0x29: "CM_EXP_DATA_NOT_SUPPORTED",
    0x2a: "CM_DEALLOC_CONFIRM_REJECT",
    0x2b: "CM_ALLOCATION_ERROR",
    0x2c: "CM_RETRY_LIMIT_EXCEEDED",
    0x2d: "CM_NO_SECONDARY_INFORMATION",
    0x2e: "CM_SECURITY_NOT_SUPPORTED",
    0x2f: "CM_SECURITY_MUTUAL_FAILED",
    0x30: "CM_CALL_NOT_SUPPORTED",
    0x31: "CM_PARM_VALUE_NOT_SUPPORTED",
    0x64: "CM_TAKE_BACKOUT",
    0x82: "CM_DEALLOCATED_ABEND_BO",
    0x83: "CM_DEALLOCATED_ABEND_SVC_BO",
    0x84: "CM_DEALLOCATED_ABEND_TIMER_BO",
    0x85: "CM_RESOURCE_FAIL_NO_RETRY_BO",
    0x86: "CM_RESOURCE_FAILURE_RETRY_BO",
    0x87: "CM_DEALLOCATED_NORMAL_BO",
    0x88: "CM_CONV_DEALLOC_AFTER_SYNCPT",
    0x89: "CM_INCLUDE_PARTNER_REJECT_BO",
    0x2711: "CM_SAP_TIMEOUT_RETRY",
    0x2712: "CM_CANCEL_REQUEST",
}
"""RFC APPC Return Code values"""


cpic_ctypes = {
    0x43: "R_2",
    0x49: "R_3",
    0x45: "STARTED_PRG",
    0x52: "REGISTRED_PRG",
}
"""RFC CPIC CType values"""


rfc_rfc_types = {
    0x32: "R_2_CONN",
    0x33: "ABAP_CONN",
    0x49: "INTERNAL_CONN",
    0x4c: "LOGIC_CONN",
    0x4d: "CMC_CONN",
    0x53: "SNA_CPIC_CONN",
    0x54: "TCP_CONN",
    0x58: "ABAP_DRIVER_CONN",
}
"""RFC Type values"""


sap_rc_values = {
}
"""RFC Return Code values"""


rfc_start_type = {
    0x0: 'DEFAULT',
    0x1: 'REMOTE_SHELL',
    0x2: 'REXEC',
    0x3: 'DISABLED',
    0x4: 'SECURE_SHELL',
}
"""RFC Start Type values"""


# APPC Header versions length:
# 1: 4Ch
# 2/3: 64h
# 4: 8Ah
# 5: 4Eh
# 6: 50h


cpic_padd = {
    "cpic_start_padd": "\x01\x01\x00\x08",
    "cpic_unk02_padd": "\x01\x01\x01\x01",
    "cpic_unk01_padd": "\x01\x01\x01\x03",
    "cpic_unk00_padd": "\x01\x03\x01\x06",
    "cpic_ip_padd": "\x01\x06\x00\x07",
    "cpic_ip_padd2": "\x00\x07\x00\x18",
    "cpic_host_sid_inbr_padd": "\x00\x18\x00\x08",
    "cpic_rfc_type_padd": "\x00\x08\x00\x11",
    "cpic_kernel1_padd": "\x00\x11\x00\x13",
    "cpic_kernel2_padd": "\x00\x13\x00\x12",
    "cpic_dest_padd": "\x00\x12\x00\x06",
    "cpic_program_padd": "\x00\x06\x01\x30",
    "cpic_username1_padd": "\x01\x30\x01\x11",
    "cpic_cli_nbr1_padd": "\x01\x11\x01\x14",
    "cpic_unk1_padd": "\x01\x14\x01\x15",
    "cpic_username2_padd": "\x01\x15\x00\x09",
    "cpic_cli_nbr2_padd": "\x00\x09\x01\x34",
    "cpic_unk2_padd": "\x01\x34\x05\x01",
    "cpic_some_params_0_padd": "\x05\x01",
    "cpic_some_params_1_padd": "\x01\x36",
    "cpic_convid_label_padd": "\x01\x36\x05\x02",
    "cpic_kernel3_padd": "\x05\x02\x00\x0b",
    "cpic_RFC_f_padd": "\x00\x0b\x01\x02",
    "cpic_unk4_padd": "\x01\x02\x05\x03",
    "cpic_th_struct_padd": "\x05\x03\x01\x31",
    "cpic_some_params2_padd": "\x01\x31\x05\x14",
    "cpic_unk6_padd": "\x05\x14\x04\x20",
    "cpic_unk7_padd": "\x04\x20\x05\x12",
    "cpic_suff_padd": "\x03\x02\x01\x04",
    "cpic_end_padd": "\x01\x04\xff\xff",
}
"""RFC CPIC Padding values"""


cpic_suff_padd = {
    "suff_padd1": "\x10\x04\x02",
    "suff_padd2": "\x10\x04\x0b",
    "suff_padd3": "\x10\x04\x04",
    "suff_padd4": "\x10\x04\x0d",
    "suff_padd5": "\x10\x04\x16",
    "suff_padd6": "\x10\x04\x17",
    "suff_padd7": "\x10\x04\x19",
    "suff_padd8": "\x10\x04\x1e",
    "suff_padd9": "\x10\x04\x25",
    "suff_padd10k": "\x10\x04\x09",
    "suff_padd10": "\x10\x04\x1d",
    "suff_padd11": "\x10\x04\x1f",
    "suff_padd12": "\x10\x04\x20",
    "suff_padd13": "\x10\x04\x21",
    "suff_padd14": "\x10\x04\x24",
    "suff_padd15": "\x10\x04\x24",
}
"""RFC CPIC Stuff Padding values"""


class SAPRFCEXTEND(PacketNoPadded):
    """SAP RFC EXTEND INFO structure
    """
    name = "SAP EXTEND INFO structure"
    fields_desc = [
        StrFixedLenPaddedField("short_dest_name", "", length=8),
        StrFixedLenPaddedField("ncpic_lu", "", length=8),
        StrFixedLenPaddedField("ncpic_tp", "", length=8),
        ByteEnumKeysField("ctype", 0x45, cpic_ctypes),
        ByteField("clientInfo", 0x01),
        StrFixedLenField("ncpic_parameters_padd", "\x00\x00", length=2),
        ShortField("comm_idx", 0x0),
        ShortField("conn_idx", 0x0),
    ]


class SAPRFCDTStruct(PacketNoPadded):
    """SAP RFC DT structure.

    This structure is used to setup started program.
    """
    name = "SAP RFC DT structure"
    fields_desc = [
        ByteField("version", 0x60),
        StrFixedLenField("padd1", "\x00" * 8, length=8),
        StrFixedLenField("root_id", "\x00" * 16, length=16),
        StrFixedLenField("conn_id", "\x00" * 16, length=16),
        IntField("conn_id_suff", 0),
        SignedIntField("timeout", -1),
        SignedIntField("keepalive_timeout", -1),
        ByteField("export_trace", 2),
        ByteEnumKeysField("start_type", 0x00, rfc_start_type),
        ByteField("net_protocol", 0x00),
        IP6Field("local_addrv6", "::1"),
        StrFixedLenPaddedField("long_lu", "", padd="\x00", length=128),
        StrFixedLenField("padd3", "\x00" * 16, length=16),
        StrFixedLenPaddedField("user", "", length=12),
        StrFixedLenField("padd4", "\x20" * 8, length=8),
        StrFixedLenField("padd5", "\x00" * 4, length=4),
        StrFixedLenField("padd6", "\x20" * 12, length=12),
        StrFixedLenField("padd7", "\x00" * 16, length=16),
        IPField("addr_ipv4", "0.0.0.0"),
        StrFixedLenField("padd8", "\x00" * 4, length=4),
        StrFixedLenPaddedField("long_tp", "", padd="\x00", length=64),
    ]


class SAPCPICSUFFIX(PacketNoPadded):
    """SAP CPIC SUFFIX structure
    """
    name = "SAP CPIC SUFFIX"
    fields_desc = [
        StrFixedLenField("suff_padd1", "\x10\x04\x02", length=3),
        FieldLenField("suff_unk1_len", None, length_of="suff_unk1", fmt="!H"),
        StrLenField("suff_unk1", "", length_from=lambda pkt: pkt.suff_unk1_len),
        StrFixedLenField("suff_padd2", "\x10\x04\x0b", length=3),
        FieldLenField("suff_unk2_len", None, length_of="suff_unk2", fmt="!H"),
        StrLenField("suff_unk2", "", length_from=lambda pkt: pkt.suff_unk2_len),
        StrFixedLenField("suff_padd3", "\x10\x04\x04", length=3),
        FieldLenField("suff_unk3_len", None, length_of="suff_unk3", fmt="!H"),
        StrLenField("suff_unk3", "", length_from=lambda pkt: pkt.suff_unk3_len),
        StrFixedLenField("suff_padd4", "\x10\x04\x0d", length=3),
        FieldLenField("suff_unk4_len", None, length_of="suff_unk4", fmt="!H"),
        StrLenField("suff_unk4", "", length_from=lambda pkt: pkt.suff_unk4_len),
        StrFixedLenField("suff_padd5", "\x10\x04\x16", length=3),
        FieldLenField("suff_unk5_len", None, length_of="suff_unk5", fmt="!H"),
        StrLenField("suff_unk5", "", length_from=lambda pkt: pkt.suff_unk5_len),
        StrFixedLenField("suff_padd6", "\x10\x04\x17", length=3),
        FieldLenField("suff_unk6_len", None, length_of="suff_unk6", fmt="!H"),
        StrLenField("suff_unk6", "", length_from=lambda pkt: pkt.suff_unk6_len),
        StrFixedLenField("suff_padd7", "\x10\x04\x19", length=3),
        FieldLenField("suff_unk7_len", None, length_of="suff_unk7", fmt="!H"),
        StrLenField("suff_unk7", "", length_from=lambda pkt: pkt.suff_unk7_len),
        StrFixedLenField("suff_padd8", "\x10\x04\x1e", length=3),
        FieldLenField("suff_unk8_len", None, length_of="suff_unk8", fmt="!H"),
        StrLenField("suff_unk8", "", length_from=lambda pkt: pkt.suff_unk8_len),
        StrFixedLenField("suff_padd9", "\x10\x04\x25", length=3),
        FieldLenField("suff_unk9_len", None, length_of="suff_unk9", fmt="!H"),
        StrLenField("suff_unk9", "", length_from=lambda pkt: pkt.suff_unk9_len),
        StrFixedLenField("suff_padd10k", "\x10\x04\x09", length=3),
        FieldLenField("suff_kernel_len", None, length_of="suff_kernel", fmt="!H"),
        StrLenField("suff_kernel", "720", length_from=lambda pkt: pkt.suff_kernel_len),

        # next fields exist only in win versions of clients suff_unk9 == "\x00\x01" (??)
        ConditionalField(StrFixedLenField("suff_padd10", "\x10\x04\x1d", length=3), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(FieldLenField("suff_unk10_len", None, length_of="suff_unk10", fmt="!H"), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(StrLenField("suff_unk10", "", length_from=lambda pkt: pkt.suff_unk10_len), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(StrFixedLenField("suff_padd11", "\x10\x04\x1f", length=3), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(FieldLenField("suff_cli1_len", None, length_of="suff_cli1", fmt="!H"), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(StrLenField("suff_cli1", "", length_from=lambda pkt: pkt.suff_cli1_len), lambda pkt: pkt.suff_unk9 == "\x00\x01"),  # ip or OS name here
        ConditionalField(StrFixedLenField("suff_padd12", "\x10\x04\x20", length=3), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(FieldLenField("suff_cli2_len", None, length_of="suff_cli2", fmt="!H"), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(StrLenField("suff_cli2", "", length_from=lambda pkt: pkt.suff_cli2_len), lambda pkt: pkt.suff_unk9 == "\x00\x01"),  # browser name here
        ConditionalField(StrFixedLenField("suff_padd13", "\x10\x04\x21", length=3), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(FieldLenField("suff_cli3_len", None, length_of="suff_cli3", fmt="!H"), lambda pkt: pkt.suff_unk9 == "\x00\x01"),
        ConditionalField(StrLenField("suff_cli3", "", length_from=lambda pkt: pkt.suff_cli3_len), lambda pkt: pkt.suff_unk9 == "\x00\x01"),  # office name here

        StrFixedLenField("suff_padd14", "\x10\x04\x24", length=3),
        FieldLenField("suff_unk14_len", None, length_of="suff_unk14", fmt="!H"),
        StrLenField("suff_unk14", "", length_from=lambda pkt: pkt.suff_unk14_len),
        StrFixedLenField("suff_padd15", "\x10\x04\x24", length=3),
        FieldLenField("suff_unk15_len", None, length_of="suff_unk15", fmt="!H"),
        StrLenField("suff_unk15", "", length_from=lambda pkt: pkt.suff_unk15_len),  # ip here
    ]


class SAPCPICPARAM(PacketNoPadded):
    """SAP CPIC PARAM Structure
    """
    name = "CPIC Params1"  # ??? may be not params :)
    fields_desc = [
        StrFixedLenField("pref", "\x01\x00\x0c\x29", length=4),
        StrFixedLenField("param1", "", length=4),
        StrFixedLenField("param2", "", length=11),
        StrFixedLenField("param_sess_1", "", length=2),
        StrFixedLenField("param_sess_2", "", length=4),
        IPField("mask", ""),
        IPField("ip", ""),
        IntField("flag", 1),
    ]


class SAPCPICPARAM2(PacketNoPadded):
    """SAP CPIC PARAM Structure
    """
    name = "CPIC Prams2"  # ??? may be not params :)
    fields_desc = [
        StrFixedLenField("param1", "", length=8),
        IPField("mask", ""),
        IPField("ip", ""),
    ]


class SAPRFCTHStruct(PacketNoPadded):
    """SAP RFC TH structure.
    """
    name = "SAP RFC TH structure"
    fields_desc = [
        StrFixedLenField("th_eyec1", "*TH*", length=4),
        ByteField("th_version", 3),
        ShortField("th_len", 230),
        ShortField("th_trace_flag", None),
        StrFixedLenPaddedField("th_sysid", "", length=32),
        ShortField("th_serevice", 1),
        StrFixedLenPaddedField("th_userid", "SAP*", length=32),
        StrFixedLenPaddedField("th_action", "", length=40),
        StrFixedLenPaddedField("th_presysid", "", length=32),
        ShortField("th_acttype", 1),
        StrFixedLenPaddedField("th_id", "", length=35),
        ByteField("th_unused_comm1", 0,),
        PacketListField("th_some_cpic_params", None, SAPCPICPARAM, count_from=lambda pkt: 1),
        StrFixedLenField("th_unused_comm2", "\x00\x00\x00\xe2", length=4),
        StrFixedLenField("th_eyec2", "*TH*", length=4),
    ]


class SAPRFXPG(PacketNoPadded):
    """SAP Started program packets.
    """
    name = "SAP Started program packets"
    fields_desc = [
        StrFixedLenField("xpg_padd100", "\x05\x12\x02\x05", length=4),
        FieldLenField("xpg_convid_l_len", None, length_of="xpg_convid_l", fmt="!H"),
        StrLenField("xpg_convid_l", "CONVID", length_from=lambda pkt: pkt.xpg_convid_l_len),

        StrFixedLenField("xpg_padd101", "\x02\x05\x02\x05", length=4),
        FieldLenField("xpg_strstat_l_len", None, length_of="xpg_strstat_l", fmt="!H"),
        StrLenField("xpg_strstat_l", "STRTSTAT", length_from=lambda pkt: pkt.xpg_strstat_l_len),

        StrFixedLenField("xpg_padd102", "\x02\x05\x02\x05", length=4),
        FieldLenField("xpg_xpgid_l_len", None, length_of="xpg_xpgid_l", fmt="!H"),
        StrLenField("xpg_xpgid_l", "XPGID", length_from=lambda pkt: pkt.xpg_xpgid_l_len),

        StrFixedLenField("xpg_padd103", "\x02\x05\x02\x01", length=4),
        FieldLenField("xpg_extprog_l_len", None, length_of="xpg_extprog_l", fmt="!H"),
        StrLenField("xpg_extprog_l", "EXTPROG", length_from=lambda pkt: pkt.xpg_extprog_l_len),

        StrFixedLenField("xpg_padd104", "\x02\x01\x02\x03", length=4),
        FieldLenField("xpg_extprog_val_len", None, length_of="xpg_extprog_val", fmt="!H"),
        StrLenField("xpg_extprog_val", "whoami", length_from=lambda pkt: pkt.xpg_extprog_val_len),

        StrFixedLenField("xpg_padd105", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_longparam_l_len", None, length_of="xpg_longparam_l", fmt="!H"),
        StrLenField("xpg_longparam_l", "LONG_PARAMS", length_from=lambda pkt: pkt.xpg_longparam_l_len),

        StrFixedLenField("xpg_padd106", "\x02\x01\x02\x03", length=4),
        FieldLenField("xpg_longparam_val_len", None, length_of="xpg_longparam_val", fmt="!H"),
        StrLenField("xpg_longparam_val", "", length_from=lambda pkt: pkt.xpg_longparam_val_len),

        StrFixedLenField("xpg_padd107", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_param_l_len", None, length_of="xpg_param_l", fmt="!H"),
        StrLenField("xpg_param_l", "PARAMS", length_from=lambda pkt: pkt.xpg_param_l_len),

        StrFixedLenField("xpg_padd108", "\x02\x01\x02\x03", length=4),
        FieldLenField("xpg_param_val_len", None, length_of="xpg_param_val", fmt="!H"),
        StrLenField("xpg_param_val", "", length_from=lambda pkt: pkt.xpg_param_val_len),

        StrFixedLenField("xpg_padd109", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_stderrcntl_l_len", None, length_of="xpg_stderrcntl_l", fmt="!H"),
        StrLenField("xpg_stderrcntl_l", "STDERRCNTL", length_from=lambda pkt: pkt.xpg_stderrcntl_l_len),

        StrFixedLenField("xpg_padd110", "\x02\x01\x02\x03", length=4),
        FieldLenField("xpg_stderrcntl_val_len", None, length_of="xpg_stderrcntl_val", fmt="!H"),
        StrLenField("xpg_stderrcntl_val", "", length_from=lambda pkt: pkt.xpg_stderrcntl_val_len),

        StrFixedLenField("xpg_padd111", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_stdincntl_l_len", None, length_of="xpg_stdincntl_l", fmt="!H"),
        StrLenField("xpg_stdincntl_l", "STDINCNTL", length_from=lambda pkt: pkt.xpg_stdincntl_l_len),

        StrFixedLenField("xpg_padd112", "\x02\x01\x02\x03", length=4),
        FieldLenField("xpg_stdincntl_val_len", None, length_of="xpg_stdincntl_val", fmt="!H"),
        StrLenField("xpg_stdincntl_val", "PARAMS", length_from=lambda pkt: pkt.xpg_stdincntl_val_len),

        StrFixedLenField("xpg_padd113", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_stdoutcntl_l_len", None, length_of="xpg_stdoutcntl_l", fmt="!H"),
        StrLenField("xpg_stdoutcntl_l", "STDOUTCNTL", length_from=lambda pkt: pkt.xpg_stdoutcntl_l_len),

        StrFixedLenField("xpg_padd114", "\x02\x01\x02\x03", length=4),
        FieldLenField("xpg_stdoutcntl_val_len", None, length_of="xpg_stdoutcntl_val", fmt="!H"),
        StrLenField("xpg_stdoutcntl_val", "", length_from=lambda pkt: pkt.xpg_stdoutcntl_val_len),

        StrFixedLenField("xpg_padd115", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_termcntl_l_len", None, length_of="xpg_termcntl_l", fmt="!H"),
        StrLenField("xpg_termcntl_l", "TERMCNTL", length_from=lambda pkt: pkt.xpg_termcntl_l_len),

        StrFixedLenField("xpg_padd116", "\x02\x01\x02\x03", length=4),
        FieldLenField("xpg_termcntl_val_len", None, length_of="xpg_termcntl_val", fmt="!H"),
        StrLenField("xpg_termcntl_val", "", length_from=lambda pkt: pkt.xpg_termcntl_val_len),

        StrFixedLenField("xpg_padd117", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_tracecntl_l_len", None, length_of="xpg_tracecntl_l", fmt="!H"),
        StrLenField("xpg_tracecntl_l", "TRACECNTL", length_from=lambda pkt: pkt.xpg_tracecntl_l_len),

        StrFixedLenField("xpg_padd118", "\x02\x03\x02\x01", length=4),
        FieldLenField("xpg_tracecntl_val_len", None, length_of="xpg_tracecntl_val", fmt="!H"),
        StrLenField("xpg_tracecntl_val", "", length_from=lambda pkt: pkt.xpg_tracecntl_val_len),

        StrFixedLenField("xpg_padd119", "\x02\x03\x03\x01", length=4),
        FieldLenField("xpg_log_l_len", None, length_of="xpg_log_l", fmt="!H"),
        StrLenField("xpg_log_l", "LOG", length_from=lambda pkt: pkt.xpg_log_l_len),

        StrFixedLenField("xpg_padd120", "\x03\x01\x03\x30", length=4),
        FieldLenField("xpg_log_val1_len", None, length_of="xpg_log_val1", fmt="!H"),
        StrLenField("xpg_log_val1", "", length_from=lambda pkt: pkt.xpg_log_val1_len),

        StrFixedLenField("xpg_padd121", "\x03\x30\x03\x02", length=4),
        FieldLenField("xpg_unk1_len", None, length_of="xpg_unk1", fmt="!H"),
        StrLenField("xpg_unk1", "", length_from=lambda pkt: pkt.xpg_unk1_len),
    ]


class DEF_FIELDS(PacketNoPadded):
    """SAP RFC Def Fields structure
    """
    fields_desc = [
        StrFixedLenField("start_padd", "", length=4),
        FieldLenField("start_field1_len", None, length_of="start_field1", fmt="!H"),
        StrLenField("start_field1", "", length_from=lambda pkt: pkt.start_field1_len),
    ]


class SAPRFCPING(PacketNoPadded):
    """SAP Started program packets RFCPING.
    """
    name = "SAP Started RFCPING packets"
    fields_desc = [
        PacketListField("fields_test", None, DEF_FIELDS, count_from=lambda pkt: 7),
    ]


class SAPRFXPG_END(PacketNoPadded):
    """SAP Started program packets SAPRFXPG_END.
    """
    name = "SAP Started SAPRFXPG_END packets"
    fields_desc = [
        StrFixedLenField("xpg_end_padd001", "\x05\x12\x02\x05", length=4),
        FieldLenField("xpg_end_ecode_l_len", None, length_of="xpg_end_ecode_l", fmt="!H"),
        StrLenField("xpg_end_ecode_l", "EXITCODE", length_from=lambda pkt: pkt.xpg_end_ecode_l_len),

        StrFixedLenField("xpg_end_padd002", "\x02\x05\x02\x05", length=4),
        FieldLenField("xpg_end_estat_l_len", None, length_of="xpg_end_estat_l", fmt="!H"),
        StrLenField("xpg_end_estat_l", "STRTSTAT", length_from=lambda pkt: pkt.xpg_end_estat_l_len),

        StrFixedLenField("xpg_end_padd003", "\x02\x05\x03\x01", length=4),
        FieldLenField("xpg_end_log_l_len", None, length_of="xpg_end_log_l", fmt="!H"),
        StrLenField("xpg_end_log_l", "LOG", length_from=lambda pkt: pkt.xpg_end_log_l_len),

        StrFixedLenField("xpg_end_padd004", "\x03\x01\x03\x30", length=4),
        FieldLenField("xpg_end_unk1_len", None, length_of="xpg_end_unk1", fmt="!H"),
        StrLenField("xpg_end_unk1", "\x00\x00\x00\x01", length_from=lambda pkt: pkt.xpg_end_unk1_len),

        StrFixedLenField("xpg_end_padd005", "\x03\x30\x03\x02", length=4),
        FieldLenField("xpg_end_unk2_len", None, length_of="xpg_end_unk2", fmt="!H"),
        StrLenField("xpg_end_unk2", "\x00\x00\x00\x80\x00\x00\x00\x00", length_from=lambda pkt: pkt.xpg_end_unk2_len),
    ]


class SAPCPIC2(PacketNoPadded):
    """SAP CPIC2 Packet
    """
    name = "SAP CPIC Packet"
    fields_desc = [
        StrFixedLenField("cpic_padd015_1", "", length=2),
        ConditionalField(FieldLenField("cpic_some_params_len", None, length_of="some_cpic_params", fmt="!H"), lambda pkt: pkt.cpic_padd015_1 == cpic_padd["cpic_some_params_1_padd"]),
        ConditionalField(PacketListField("some_cpic_params", None, SAPCPICPARAM, length_from=lambda pkt: pkt.cpic_some_params_len), lambda pkt: pkt.cpic_padd015_1 == cpic_padd["cpic_some_params_1_padd"]),

        StrFixedLenField("cpic_padd016", "", length=4),
        ConditionalField(FieldLenField("cpic_convid_label_len", None, length_of="cpic_convid_label", fmt="!H"), lambda pkt: pkt.cpic_padd016 == cpic_padd["cpic_convid_label_padd"]),
        ConditionalField(StrLenField("cpic_convid_label", "", length_from=lambda pkt: pkt.cpic_convid_label_len), lambda pkt: pkt.cpic_padd016 == cpic_padd["cpic_convid_label_padd"]),

        StrFixedLenField("cpic_padd017", "", length=4),
        ConditionalField(FieldLenField("cpic_kernel3_len", None, length_of="cpic_kernel3", fmt="!H"), lambda pkt: pkt.cpic_padd017 == cpic_padd["cpic_kernel3_padd"]),
        ConditionalField(StrLenField("cpic_kernel3", "", length_from=lambda pkt: pkt.cpic_kernel3_len), lambda pkt: pkt.cpic_padd017 == cpic_padd["cpic_kernel3_padd"]),

        StrFixedLenField("cpic_padd018", "", length=4),
        ConditionalField(FieldLenField("cpic_RFC_f_len", None, length_of="cpic_RFC_f", fmt="!H"), lambda pkt: pkt.cpic_padd018 == cpic_padd["cpic_RFC_f_padd"]),
        ConditionalField(StrLenField("cpic_RFC_f", "", length_from=lambda pkt: pkt.cpic_RFC_f_len), lambda pkt: pkt.cpic_padd018 == cpic_padd["cpic_RFC_f_padd"]),

        StrFixedLenField("cpic_padd019", "", length=4),
        ConditionalField(FieldLenField("cpic_unk4_len", None, length_of="cpic_unk4", fmt="!H"), lambda pkt: pkt.cpic_padd019 == cpic_padd["cpic_unk4_padd"]),
        ConditionalField(StrLenField("cpic_unk4", "", length_from=lambda pkt: pkt.cpic_unk4_len), lambda pkt: pkt.cpic_padd019 == cpic_padd["cpic_unk4_padd"]),

        # StrFixedLenField("cpic_padd020", "", length=4), #TODO: we send this field in original request
        # ConditionalField(FieldLenField("cpic_th_struct_len", None, length_of="cpic_th_struct", fmt="!H"), lambda pkt: pkt.cpic_padd020 == cpic_padd["cpic_th_struct_padd"]),
        # ConditionalField(PacketListField("cpic_th_struct", None, SAPRFCTHStruct, length_from=lambda pkt: pkt.cpic_th_struct_len), lambda pkt: pkt.cpic_padd020 == cpic_padd["cpic_th_struct_padd"]),

        StrFixedLenField("cpic_padd021", "", length=4),
        ConditionalField(FieldLenField("cpic_some_params2_len", None, length_of="some_cpic_params2", fmt="!H"), lambda pkt: pkt.cpic_padd021[2:] == cpic_padd["cpic_some_params2_padd"][2:]),
        ConditionalField(PacketListField("some_cpic_params2", None, SAPCPICPARAM2, length_from=lambda pkt: pkt.cpic_some_params2_len), lambda pkt: pkt.cpic_padd021[2:] == cpic_padd["cpic_some_params2_padd"][2:]),

        StrFixedLenField("cpic_padd022", "", length=4),
        ConditionalField(FieldLenField("cpic_unk6_len", None, length_of="cpic_unk6", fmt="!H"), lambda pkt: pkt.cpic_padd022 == cpic_padd["cpic_unk6_padd"]),
        ConditionalField(StrLenField("cpic_unk6", "", length_from=lambda pkt: pkt.cpic_unk6_len), lambda pkt: pkt.cpic_padd022 == cpic_padd["cpic_unk6_padd"]),

        StrFixedLenField("cpic_padd023", "", length=4),
        ConditionalField(FieldLenField("cpic_unk7_len", None, length_of="cpic_unk7", fmt="!H"), lambda pkt: pkt.cpic_padd023 == cpic_padd["cpic_unk7_padd"]),
        ConditionalField(StrLenField("cpic_unk7", "", length_from=lambda pkt: pkt.cpic_unk7_len), lambda pkt: pkt.cpic_padd023 == cpic_padd["cpic_unk7_padd"]),

        # Started PRG SAPXPG_START_XPG_LONG
        ConditionalField(PacketField("xpg_p", None, SAPRFXPG), lambda pkt: pkt.cpic_RFC_f == 'SAPXPG_START_XPG_LONG'),
        # End
        ConditionalField(PacketField("xpg_end", None, SAPRFXPG_END), lambda pkt: pkt.cpic_RFC_f in ['SAPXPG_END_XPG']),

        # Started PRG RFC_PING
        ConditionalField(PacketField("rfc_ping", None, SAPRFCPING), lambda pkt: pkt.cpic_RFC_f in ['RFC_PING']),

        StrFixedLenField("cpic_padd024", "", length=4),
        ConditionalField(FieldLenField("cpic_suff_len", None, length_of="cpic_suff", fmt="!H"), lambda pkt: pkt.cpic_padd024 == cpic_padd["cpic_suff_padd"]),
        ConditionalField(PacketListField("cpic_suff", None, SAPCPICSUFFIX, length_from=lambda pkt: pkt.cpic_suff_len), lambda pkt: pkt.cpic_padd024 == cpic_padd["cpic_suff_padd"]),

        StrFixedLenField("cpic_end_padd", "", length=4),
        ConditionalField(FieldLenField("cpic_end_len", None, length_of="cpic_end", fmt="!H"), lambda pkt: pkt.cpic_end_padd == cpic_padd["cpic_end_padd"]),
        ConditionalField(StrLenField("cpic_end", "", length_from=lambda pkt: pkt.cpic_end_len), lambda pkt: pkt.cpic_end_padd == cpic_padd["cpic_end_padd"]),

        StrFixedLenField("cpic_end_sig", "\x00\x00\xff\xff", length=4),
    ]


class SAPCPIC(PacketNoPadded):
    """SAP CPIC Packet
    """
    name = "SAP CPIC Packet"
    fields_desc = [
        StrFixedLenField("cpic_start_padd", "", length=4),
        ConditionalField(ShortField("cpic_cpic_length", None), lambda pkt: pkt.cpic_start_padd == cpic_padd["cpic_start_padd"]),  # don't no what it is

        StrFixedLenField("cpic_padd0003", "", length=4),
        ConditionalField(FieldLenField("cpic_unk02_len", None, length_of="cpic_unk02", fmt="!H"), lambda pkt: pkt.cpic_padd0003 == cpic_padd["cpic_unk02_padd"]),
        ConditionalField(StrLenField("cpic_unk02", "", length_from=lambda pkt: pkt.cpic_unk02_len), lambda pkt: pkt.cpic_padd0003 == cpic_padd["cpic_unk02_padd"]),

        StrFixedLenField("cpic_padd0002", "", length=4),
        ConditionalField(FieldLenField("cpic_unk01_len", None, length_of="cpic_unk01", fmt="!H"), lambda pkt: pkt.cpic_padd0002 == cpic_padd["cpic_unk01_padd"]),
        ConditionalField(StrLenField("cpic_unk01", "", length_from=lambda pkt: pkt.cpic_unk01_len), lambda pkt: pkt.cpic_padd0002 == cpic_padd["cpic_unk01_padd"]),

        StrFixedLenField("cpic_padd0001", "", length=4),
        ConditionalField(FieldLenField("cpic_unk00_len", None, length_of="cpic_unk00", fmt="!H"), lambda pkt: pkt.cpic_padd0001 == cpic_padd["cpic_unk00_padd"]),
        ConditionalField(StrLenField("cpic_unk00", "", length_from=lambda pkt: pkt.cpic_unk00_len), lambda pkt: pkt.cpic_padd0001 == cpic_padd["cpic_unk00_padd"]),

        StrFixedLenField("cpic_padd001", "", length=4),
        ConditionalField(FieldLenField("cpic_ip_len", None, length_of="cpic_ip", fmt="!H"), lambda pkt: pkt.cpic_padd001 == cpic_padd["cpic_ip_padd"]),
        ConditionalField(StrLenField("cpic_ip", "", length_from=lambda pkt: pkt.cpic_ip_len), lambda pkt: pkt.cpic_padd001 == cpic_padd["cpic_ip_padd"]),

        StrFixedLenField("cpic_padd002", "", length=4),
        ConditionalField(FieldLenField("cpic_ip2_len", None, length_of="cpic_ip2", fmt="!H"), lambda pkt: pkt.cpic_padd002 == cpic_padd["cpic_ip_padd2"]),
        ConditionalField(StrLenField("cpic_ip2", "", length_from=lambda pkt: pkt.cpic_ip2_len), lambda pkt: pkt.cpic_padd002 == cpic_padd["cpic_ip_padd2"]),

        StrFixedLenField("cpic_padd003", "", length=4),
        ConditionalField(FieldLenField("cpic_host_sid_inbr_len", None, length_of="cpic_host_sid_inbr", fmt="!H"), lambda pkt: pkt.cpic_padd003 == cpic_padd["cpic_host_sid_inbr_padd"]),
        ConditionalField(StrLenField("cpic_host_sid_inbr", "", length_from=lambda pkt: pkt.cpic_host_sid_inbr_len), lambda pkt: pkt.cpic_padd003 == cpic_padd["cpic_host_sid_inbr_padd"]),

        StrFixedLenField("cpic_padd004", "", length=4),
        ConditionalField(FieldLenField("cpic_rfc_type_len", None, length_of="cpic_rfc_type", fmt="!H"), lambda pkt: pkt.cpic_padd004 == cpic_padd["cpic_rfc_type_padd"]),
        ConditionalField(StrLenField("cpic_rfc_type", "", length_from=lambda pkt: pkt.cpic_rfc_type_len), lambda pkt: pkt.cpic_padd004 == cpic_padd["cpic_rfc_type_padd"]),

        StrFixedLenField("cpic_padd005", "", length=4),
        ConditionalField(FieldLenField("cpic_kernel1_len", None, length_of="cpic_kernel1", fmt="!H"), lambda pkt: pkt.cpic_padd005 == cpic_padd["cpic_kernel1_padd"]),
        ConditionalField(StrLenField("cpic_kernel1", "", length_from=lambda pkt: pkt.cpic_kernel1_len), lambda pkt: pkt.cpic_padd005 == cpic_padd["cpic_kernel1_padd"]),

        StrFixedLenField("cpic_padd006", "", length=4),
        ConditionalField(FieldLenField("cpic_kernel2_len", None, length_of="cpic_kernel2", fmt="!H"), lambda pkt: pkt.cpic_padd006 == cpic_padd["cpic_kernel2_padd"]),
        ConditionalField(StrLenField("cpic_kernel2", "", length_from=lambda pkt: pkt.cpic_kernel2_len), lambda pkt: pkt.cpic_padd006 == cpic_padd["cpic_kernel2_padd"]),

        StrFixedLenField("cpic_padd007", "", length=4),
        ConditionalField(FieldLenField("cpic_dest_len", None, length_of="cpic_dest", fmt="!H"), lambda pkt: pkt.cpic_padd007 == cpic_padd["cpic_dest_padd"]),
        ConditionalField(StrLenField("cpic_dest", "", length_from=lambda pkt: pkt.cpic_dest_len), lambda pkt: pkt.cpic_padd007 == cpic_padd["cpic_dest_padd"]),

        StrFixedLenField("cpic_padd008", "", length=4),
        ConditionalField(FieldLenField("cpic_program_len", None, length_of="cpic_program", fmt="!H"), lambda pkt: pkt.cpic_padd008 == cpic_padd["cpic_program_padd"]),
        ConditionalField(StrLenField("cpic_program", "", length_from=lambda pkt: pkt.cpic_program_len), lambda pkt: pkt.cpic_padd008 == cpic_padd["cpic_program_padd"]),

        StrFixedLenField("cpic_padd009", "", length=4),
        ConditionalField(FieldLenField("cpic_username1_len", None, length_of="cpic_username1", fmt="!H"), lambda pkt: pkt.cpic_padd009 == cpic_padd["cpic_username1_padd"]),
        ConditionalField(StrLenField("cpic_username1", "", length_from=lambda pkt: pkt.cpic_username1_len), lambda pkt: pkt.cpic_padd009 == cpic_padd["cpic_username1_padd"]),

        StrFixedLenField("cpic_padd010", "", length=4),
        ConditionalField(FieldLenField("cpic_cli_nbr1_len", None, length_of="cpic_cli_nbr1", fmt="!H"), lambda pkt: pkt.cpic_padd010 == cpic_padd["cpic_cli_nbr1_padd"]),
        ConditionalField(StrLenField("cpic_cli_nbr1", "", length_from=lambda pkt: pkt.cpic_cli_nbr1_len), lambda pkt: pkt.cpic_padd010 == cpic_padd["cpic_cli_nbr1_padd"]),

        StrFixedLenField("cpic_padd011", "", length=4),
        ConditionalField(FieldLenField("cpic_unk1_len", None, length_of="cpic_unk1", fmt="!H"), lambda pkt: pkt.cpic_padd011 == cpic_padd["cpic_unk1_padd"]),
        ConditionalField(StrLenField("cpic_unk1", "", length_from=lambda pkt: pkt.cpic_unk1_len), lambda pkt: pkt.cpic_padd011 == cpic_padd["cpic_unk1_padd"]),

        StrFixedLenField("cpic_padd012", "", length=4),
        ConditionalField(FieldLenField("cpic_username2_len", None, length_of="cpic_username2", fmt="!H"), lambda pkt: pkt.cpic_padd012 == cpic_padd["cpic_username2_padd"]),
        ConditionalField(StrLenField("cpic_username2", "", length_from=lambda pkt: pkt.cpic_username2_len), lambda pkt: pkt.cpic_padd012 == cpic_padd["cpic_username2_padd"]),

        StrFixedLenField("cpic_padd013", "", length=4),
        ConditionalField(FieldLenField("cpic_cli_nbr2_len", None, length_of="cpic_cli_nbr2", fmt="!H"), lambda pkt: pkt.cpic_padd013 == cpic_padd["cpic_cli_nbr2_padd"]),
        ConditionalField(StrLenField("cpic_cli_nbr2", "", length_from=lambda pkt: pkt.cpic_cli_nbr2_len), lambda pkt: pkt.cpic_padd013 == cpic_padd["cpic_cli_nbr2_padd"]),

        StrFixedLenField("cpic_padd014", "", length=4),
        ConditionalField(FieldLenField("cpic_unk2_len", None, length_of="cpic_unk2", fmt="!H"), lambda pkt: pkt.cpic_padd014 == cpic_padd["cpic_unk2_padd"]),
        ConditionalField(StrLenField("cpic_unk2", "", length_from=lambda pkt: pkt.cpic_unk2_len), lambda pkt: pkt.cpic_padd014 == cpic_padd["cpic_unk2_padd"]),

        # dirty fix for the last packet
        StrFixedLenField("cpic_padd015_0", "", length=2),  # <---- last packets starts here
        StrFixedLenField("cpic_padd015_1", "", length=2),
        ConditionalField(FieldLenField("cpic_some_params_len", None, length_of="some_cpic_params", fmt="!H"), lambda pkt: pkt.cpic_padd015_1 == cpic_padd["cpic_some_params_1_padd"]),
        ConditionalField(PacketListField("some_cpic_params", None, SAPCPICPARAM, length_from=lambda pkt: pkt.cpic_some_params_len), lambda pkt: pkt.cpic_padd015_1 == cpic_padd["cpic_some_params_1_padd"]),

        StrFixedLenField("cpic_padd016", "", length=4),
        ConditionalField(FieldLenField("cpic_convid_label_len", None, length_of="cpic_convid_label", fmt="!H"), lambda pkt: pkt.cpic_padd016 == cpic_padd["cpic_convid_label_padd"]),
        ConditionalField(StrLenField("cpic_convid_label", "", length_from=lambda pkt: pkt.cpic_convid_label_len), lambda pkt: pkt.cpic_padd016 == cpic_padd["cpic_convid_label_padd"]),

        StrFixedLenField("cpic_padd017", "", length=4),
        ConditionalField(FieldLenField("cpic_kernel3_len", None, length_of="cpic_kernel3", fmt="!H"), lambda pkt: pkt.cpic_padd017 == cpic_padd["cpic_kernel3_padd"]),
        ConditionalField(StrLenField("cpic_kernel3", "", length_from=lambda pkt: pkt.cpic_kernel3_len), lambda pkt: pkt.cpic_padd017 == cpic_padd["cpic_kernel3_padd"]),

        StrFixedLenField("cpic_padd018", "", length=4),
        ConditionalField(FieldLenField("cpic_RFC_f_len", None, length_of="cpic_RFC_f", fmt="!H"), lambda pkt: pkt.cpic_padd018 == cpic_padd["cpic_RFC_f_padd"]),
        ConditionalField(StrLenField("cpic_RFC_f", "", length_from=lambda pkt: pkt.cpic_RFC_f_len), lambda pkt: pkt.cpic_padd018 == cpic_padd["cpic_RFC_f_padd"]),

        StrFixedLenField("cpic_padd019", "", length=4),
        ConditionalField(FieldLenField("cpic_unk4_len", None, length_of="cpic_unk4", fmt="!H"), lambda pkt: pkt.cpic_padd019 == cpic_padd["cpic_unk4_padd"]),
        ConditionalField(StrLenField("cpic_unk4", "", length_from=lambda pkt: pkt.cpic_unk4_len), lambda pkt: pkt.cpic_padd019 == cpic_padd["cpic_unk4_padd"]),

        StrFixedLenField("cpic_padd020", "", length=4),
        ConditionalField(FieldLenField("cpic_th_struct_len", None, length_of="cpic_th_struct", fmt="!H"), lambda pkt: pkt.cpic_padd020 == cpic_padd["cpic_th_struct_padd"]),
        ConditionalField(PacketListField("cpic_th_struct", None, SAPRFCTHStruct, length_from=lambda pkt: pkt.cpic_th_struct_len), lambda pkt: pkt.cpic_padd020 == cpic_padd["cpic_th_struct_padd"]),

        StrFixedLenField("cpic_padd021", "", length=4),
        ConditionalField(FieldLenField("cpic_some_params2_len", None, length_of="some_cpic_params2", fmt="!H"), lambda pkt: pkt.cpic_padd021 == cpic_padd["cpic_some_params2_padd"]),
        ConditionalField(PacketListField("some_cpic_params2", None, SAPCPICPARAM2, length_from=lambda pkt: pkt.cpic_some_params2_len), lambda pkt: pkt.cpic_padd021 == cpic_padd["cpic_some_params2_padd"]),

        StrFixedLenField("cpic_padd022", "", length=4),
        ConditionalField(FieldLenField("cpic_unk6_len", None, length_of="cpic_unk6", fmt="!H"), lambda pkt: pkt.cpic_padd022 == cpic_padd["cpic_unk6_padd"]),
        ConditionalField(StrLenField("cpic_unk6", "", length_from=lambda pkt: pkt.cpic_unk6_len), lambda pkt: pkt.cpic_padd022 == cpic_padd["cpic_unk6_padd"]),

        StrFixedLenField("cpic_padd023", "", length=4),
        ConditionalField(FieldLenField("cpic_unk7_len", None, length_of="cpic_unk7", fmt="!H"), lambda pkt: pkt.cpic_padd023 == cpic_padd["cpic_unk7_padd"]),
        ConditionalField(StrLenField("cpic_unk7", "", length_from=lambda pkt: pkt.cpic_unk7_len), lambda pkt: pkt.cpic_padd023 == cpic_padd["cpic_unk7_padd"]),

        # Started PRG SAPXPG_START_XPG_LONG
        ConditionalField(PacketField("xpg_p", None, SAPRFXPG), lambda pkt: pkt.cpic_RFC_f == 'SAPXPG_START_XPG_LONG'),

        # Started PRG RFC_PING
        ConditionalField(PacketField("rfc_ping", None, SAPRFCPING), lambda pkt: pkt.cpic_RFC_f == 'RFC_PING'),

        StrFixedLenField("cpic_padd024", "", length=4),
        ConditionalField(FieldLenField("cpic_suff_len", None, length_of="cpic_suff", fmt="!H"), lambda pkt: pkt.cpic_padd024 == cpic_padd["cpic_suff_padd"]),
        ConditionalField(PacketListField("cpic_suff", None, SAPCPICSUFFIX, length_from=lambda pkt: pkt.cpic_suff_len), lambda pkt: pkt.cpic_padd024 == cpic_padd["cpic_suff_padd"]),

        StrFixedLenField("cpic_end_padd", "", length=4),
        ConditionalField(FieldLenField("cpic_end_len", None, length_of="cpic_end", fmt="!H"), lambda pkt: pkt.cpic_end_padd == cpic_padd["cpic_end_padd"]),
        ConditionalField(StrLenField("cpic_end", "", length_from=lambda pkt: pkt.cpic_end_len), lambda pkt: pkt.cpic_end_padd == cpic_padd["cpic_end_padd"]),

        StrFixedLenField("cpic_end_sig", "\x00\x00\xff\xff", length=4),
    ]


class SAPCPIC_CUT(PacketNoPadded):
    """SAP CPIC CUT structure.
    """
    name = "SAP CUT"
    fields_desc = [
        # StrLenField("keke1", ""),
        StrFixedLenField("keke1", "", 475),
    ]


class SAPRFC(PacketNoPadded):
    """SAP Remote Function Call packet

    This packet is used for the Remote Function Call (RFC) protocol.
    """
    name = "SAP Remote Function Call"
    fields_desc = [
        ByteField("version", 3),  # If the version is 3, the packet has a size > 88h, versions 1 and 2 are 40h
        ConditionalField(ByteEnumKeysField("req_type", 0, rfc_req_type_values), lambda pkt: pkt.version != 0x06),
        ConditionalField(ByteEnumKeysField("func_type", 0, rfc_func_type_values), lambda pkt: pkt.version == 0x06),

        # Normal client fields (GW_NORMAL_CLIENT)
        ConditionalField(IPField("address", "0.0.0.0"), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(IntField("padd1", 0), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("service", "", length=10), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(StrFixedLenField("codepage", "1100", length=4), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(StrFixedLenField("padd2", "\x00" * 6, length=6), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("lu", "", length=8), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("tp", "", length=8), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(StrFixedLenPaddedField("conversation_id", "", length=8), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(ByteField("appc_header_version", 6), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(FlagsField("accept_info", 0xcb, 8,
                                    ["EINFO", "PING", "SNC", "CONN_EINFO", "CODE_PAGE", "NIPING", "EXTINITOPT",
                                     "GW_ACCEPT_DIST_TRACE"]), lambda pkt: pkt.req_type == 0x03),  # chipik
        ConditionalField(SignedShortField("idx", -1), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(IP6Field("address6", "::"), lambda pkt: pkt.req_type == 0x03 and pkt.version == 3),

        ConditionalField(IntField("rc", 0), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(ByteField("echo_data", 0), lambda pkt: pkt.req_type == 0x03),
        ConditionalField(ByteField("filler", 0), lambda pkt: pkt.req_type == 0x03),

        # Monitor Command fields (GW_SEND_CMD)
        ConditionalField(ByteEnumKeysField("cmd", 0, rfc_monitor_cmd_values), lambda pkt: pkt.req_type == 0x09),

        # General padding for non implemented request types
        ConditionalField(StrFixedLenField("padd_v12", "\x00" * 61, length=61),
                         lambda pkt: pkt.version < 3 and pkt.req_type == 0x09),
        ConditionalField(StrFixedLenField("padd_v12", "\x00" * 62, length=62),
                         lambda pkt: pkt.version < 3 and pkt.req_type not in [0x03, 0x09]),
        ConditionalField(StrFixedLenField("padd_v3", "\x00" * 133, length=133),
                         lambda pkt: pkt.version == 3 and pkt.req_type == 0x09),
        ConditionalField(StrFixedLenField("padd_v3", "\x00" * 134, length=134),
                         lambda pkt: pkt.version == 3 and pkt.req_type not in [0x03, 0x09]),

        # APPC layer POC for remote function call
        ConditionalField(ByteEnumKeysField("protocol", 0x3, appc_protocol_values), lambda pkt: pkt.version == 0x6),
        ConditionalField(ByteField("mode", 0x0), lambda pkt: pkt.version == 0x6),
        ConditionalField(ShortField("uid", 0x13), lambda pkt: pkt.version == 0x6),
        ConditionalField(ShortField("gw_id", 0x0), lambda pkt: pkt.version == 0x6),
        ConditionalField(ShortField("err_len", 0x0), lambda pkt: pkt.version == 0x6),
        ConditionalField(FlagsField("info2", 0, 8,
                                    ["WITH_LONG_LU_NAME", "GW_IMMEDIATE", "GW_SNC_ACTIVE", "GW_WAIT_LOOK_UP",
                                     "SNC_INIT_PHASE", "GW_STATELESS"]), lambda pkt: pkt.version == 0x6),  # chipik
        ConditionalField(ByteField("trace_level", 0x1), lambda pkt: pkt.version == 0x6),
        ConditionalField(IntField("time", 0x0), lambda pkt: pkt.version == 0x6),
        ConditionalField(FlagsField("info3", 0, 8,
                                    ["GW_WITH_CODE_PAGE", "GW_ASYNC_RFC", "GW_CANCEL_HARD", "GW_CANCEL_SOFT",
                                     "GW_WITH_GUI_TIMEOUT", "GW_TERMIO_ERROR", "GW_EXTENDED_INIT_OPTIONS",
                                     "GW_DIST_TRACE"]), lambda pkt: pkt.version == 0x6),  # chipik
        ConditionalField(SignedIntField("timeout", -1), lambda pkt: pkt.version == 0x6),
        ConditionalField(ByteField("info4", 0x0), lambda pkt: pkt.version == 0x6),  # bitfield
        ConditionalField(IntField("seq_no", 0x0), lambda pkt: pkt.version == 0x6),
        ConditionalField(FieldLenField("sap_param_len", None, length_of="sap_param", fmt="!H"),
                         lambda pkt: pkt.version == 0x6),
        ConditionalField(ByteField("padd_appc", 0x0), lambda pkt: pkt.version == 0x6),  # bitfield
        ConditionalField(FlagsField("info", 0, 16,
                                    ["SYNC_CPIC_FUNCTION", "WITH_HOSTADDR", "WITH_GW_SAP_PARAMS_HDR", "CPIC_SYNC_REQ",
                                     "WITH_ERR_INFO", "DATA_WITH_TERM_OUTPUT", "DATA_WITH_TERM_INPUT",
                                     "R3_CPIC_LOGIN_WITH_TERM"]), lambda pkt: pkt.version == 0x6),  # chipik       C
        ConditionalField(FlagsField("vector", 0, 8,
                                    ["F_V_INITIALIZE_CONVERSATION", "F_V_ALLOCATE", "F_V_SEND_DATA", "F_V_RECEIVE",
                                     "F_V_FLUSH"]), lambda pkt: pkt.version == 0x6),  # chipik       C
        ConditionalField(IntEnumKeysField("appc_rc", 0x0, appc_rc_values), lambda pkt: pkt.version == 0x6),
        ConditionalField(IntEnumKeysField("sap_rc", 0x0, sap_rc_values), lambda pkt: pkt.version == 0x6),
        ConditionalField(StrFixedLenField("conv_id", 0, 8), lambda pkt: pkt.version == 0x6),
        ConditionalField(PacketField("sap_ext_header", None, SAPRFCEXTEND), lambda pkt: pkt.version == 0x6 and 'GW_EXTENDED_INIT_OPTIONS' in str(pkt.info3)),  # chipik
        ConditionalField(StrFixedLenField("cm_ok_padd", 0, 32), lambda pkt: pkt.version == 0x6 and "SYNC_CPIC_FUNCTION" in str(pkt.info) and "GW_WITH_CODE_PAGE" not in str(pkt.info3)),  # chipik
        ConditionalField(IntField("codepage_size1", 0,), lambda pkt: pkt.version == 0x6 and "GW_WITH_CODE_PAGE" in str(pkt.info3)),  # chipik
        ConditionalField(StrFixedLenField("codepage_padd1", 0, 4), lambda pkt: pkt.version == 0x6 and "GW_WITH_CODE_PAGE" in str(pkt.info3)),  # chipik
        ConditionalField(IntField("codepage_size2", 0,), lambda pkt: pkt.version == 0x6 and "GW_WITH_CODE_PAGE" in str(pkt.info3)),  # chipik
        ConditionalField(StrFixedLenField("codepage_padd2", 0, 4), lambda pkt: pkt.version == 0x6 and "GW_WITH_CODE_PAGE" in str(pkt.info3)),  # chipik
        ConditionalField(StrFixedLenField("codepage_padd3", 0, 4), lambda pkt: pkt.version == 0x6 and "GW_WITH_CODE_PAGE" in str(pkt.info3)),  # chipik
        ConditionalField(StrFixedLenField("codepage", 0, 5), lambda pkt: pkt.version == 0x6 and "GW_WITH_CODE_PAGE" in str(pkt.info3)),  # chipik
        ConditionalField(StrFixedLenField("codepage_padd2", 0, 7), lambda pkt: pkt.version == 0x6 and "GW_WITH_CODE_PAGE" in str(pkt.info3)),  # chipik

        ConditionalField(PacketField("sap_param", None, SAPRFCDTStruct), lambda pkt: pkt.version == 0x6 and 'GW_DIST_TRACE' in str(pkt.info3)),  # chipik

        # error message
        ConditionalField(StrField("error_msg", ""), lambda pkt: pkt.version == 0x6 and 'WITH_ERR_INFO' in str(pkt.info)),

        # F_V_SEND_DATA
        ConditionalField(PacketField("sap_cpic", None, SAPCPIC), lambda pkt: pkt.version == 0x6 and 'F_V_SEND_DATA' in str(pkt.vector) and not pkt.codepage_size2),  # chipik
        ConditionalField(PacketField("sap_cpic_cut", None, SAPCPIC2), lambda pkt: pkt.version == 0x6 and 'F_V_SEND_DATA' in str(pkt.vector) and not pkt.codepage_size2 and not pkt.sap_cpic),  # chipik

        # answer from Anon GW
        ConditionalField(StrFixedLenField("anon_repl_sign", '\x05\x00\x00\x00', 4), lambda pkt: pkt.version == 0x6 and 'F_V_RECEIVE' == str(pkt.vector)),
        ConditionalField(PacketListField("repl", None, DEF_FIELDS, length_from=lambda pkt: pkt.codepage_size2), lambda pkt: pkt.version == 0x6 and 'F_V_RECEIVE' in str(pkt.vector) and pkt.codepage_size2 > 0),

        ConditionalField(ShortField("cpic_packet_size", 0x0), lambda pkt: pkt.version == 0x6 and 'F_V_SEND_DATA' in str(pkt.vector)),  # chipik
        ConditionalField(IntField("rfc_packet_size", 0x0), lambda pkt: pkt.version == 0x6 and 'F_V_SEND_DATA' in str(pkt.vector)),  # chipik
    ]


# Bind SAP NI with the RFC port
bind_layers(TCP, SAPNI, dport=3300)
