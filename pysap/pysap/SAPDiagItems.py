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
from struct import pack
from binascii import unhexlify as unhex
# External imports
from scapy.packet import Packet
from scapy.fields import (IntField, BitField, ShortField, ByteField,
                          ConditionalField, StrLenField, StrField,
                          StrNullField, FieldLenField, PacketListField,
                          FieldListField, ByteEnumKeysField,
                          ShortEnumKeysField)
# Custom imports
from pysap.utils.fields import PacketNoPadded
from pysap.SAPDiag import bind_diagitem, SAPDiagItem


# Diag User connect item
#
# Protocol version (int):   100200  0x00018768 SAP Diag Compressed
#                           000200  0x000000c8 SAP Diag No Compressed
# CodePage (int):           1100    0x0000044c Code Page 1100
# WS Type (int):            5001    0x00001389 SAP GUI Java
#                           3000    0x00000bb8 SAP GUI Windows
#
class SAPDiagUserConnect(Packet):
    name = "SAP Diag User Connect"
    fields_desc = [IntField("protocol_version", 100200),
                   IntField("code_page", 1100),
                   IntField("ws_type", 5001)]


bind_diagitem(SAPDiagUserConnect, "APPL", 0x04, 0x02)

user_connect_compressed = \
    SAPDiagItem(item_type="APPL",
                item_id="ST_USER",
                item_sid=0x02,
                item_value=SAPDiagUserConnect())

user_connect_uncompressed = \
    SAPDiagItem(item_type="APPL",
                item_id="ST_USER",
                item_sid=0x02,
                item_value=SAPDiagUserConnect(protocol_version=200))


# Diag Dialog step item
class SAPDiagStep(Packet):
    name = "SAP Diag Dialog Step"
    fields_desc = [IntField("step", 0)]


bind_diagitem(SAPDiagStep, "APPL", 0x04, 0x26)


# Diag Support data (support bits)
class SAPDiagSupportBits(Packet):
    name = "SAP Diag Support Bits"
    fields_desc = [
        BitField("SAPGUI_IMODE", 0, 1),  # 7
        BitField("SAPGUI_COMPR_ENHANCED", 0, 1),  # 6
        BitField("SAPGUI_FONT_METRIC", 0, 1),  # 5
        BitField("SAPGUI_SYMBOL_RIGHT", 0, 1),  # 4
        BitField("SAPGUI_SELECT_RECT", 0, 1),  # 3
        BitField("SAPGUI_DIAGVERSION", 0, 1),  # 2
        BitField("SAPGUI_LABELS", 0, 1),  # 1
        BitField("PROGRESS_INDICATOR", 0, 1),  # 0

        BitField("RFC_DIALOG", 0, 1),  # 15
        BitField("INPUT_UPPERCASE", 0, 1),  # 14
        BitField("SAPGUI_TABPROPERTY", 0, 1),  # 13
        BitField("UPPERCASE", 0, 1),  # 12
        BitField("SAPGUI_PUSHBUTTON_1", 0, 1),  # 11
        BitField("SAPGUI_FOCUS_1", 0, 1),  # 10
        BitField("SAPGUI_TABLE", 0, 1),  # 9
        BitField("SAPGUI_LONG_MESSAGE", 0, 1),  # 8

        BitField("DLGH_FLAGS", 0, 1),  # 23
        BitField("CONTAINER_TYPE", 0, 1),  # 22
        BitField("OBJECT_NAMES", 0, 1),  # 21
        BitField("FULL_MENU", 0, 1),  # 20
        BitField("STOP_TRANS", 0, 1),  # 19
        BitField("MENU_SHORTCUT", 0, 1),  # 18
        BitField("FKEY_TABLE", 0, 1),  # 17
        BitField("LIST_HOTSPOT", 0, 1),  # 16

        BitField("NORABAX", 0, 1),  # 31
        BitField("NOMESSAGES", 0, 1),  # 30
        BitField("NOGRAPH", 0, 1),  # 29
        BitField("GUIAPI", 0, 1),  # 28
        BitField("TABSEL_ATTRIB", 0, 1),  # 27
        BitField("MESDUM_FLAG1", 0, 1),  # 26
        BitField("MESSAGE_INFO", 0, 1),  # 25
        BitField("APPL_MNU", 0, 1),  # 24

        BitField("TABLE_SIZE_OK", 0, 1),  # 39
        BitField("SCROLL_INFOS", 0, 1),  # 38
        BitField("OCX_SUPPORT", 0, 1),  # 37
        BitField("MESSAGE_VARS", 0, 1),  # 36
        BitField("NEW_BSD_JUSTRIGHT", 0, 1),  # 35
        BitField("NORFC", 0, 1),  # 34
        BitField("NOSAPSCRIPT", 0, 1),  # 33
        BitField("NOSYSMSG", 0, 1),  # 32

        BitField("TABSCROLL_INFOS", 0, 1),  # 47
        BitField("UNKNOWN_1", 0, 1),  # 46 (Unknown support bit)
        BitField("TABSTRIP", 0, 1),  # 45
        BitField("PUSHBUTTON_2", 0, 1),  # 44
        BitField("CONN_WSIZE", 0, 1),  # 43
        BitField("CURR_TCODE", 0, 1),  # 42
        BitField("VARINFO_OKCODE", 0, 1),  # 41
        BitField("MESSAGE_INFO2", 0, 1),  # 40

        BitField("NEED_STDDYNPRO", 0, 1),  # 55
        BitField("R3INFO_USER_CHECKED", 0, 1),  # 54
        BitField("APPTOOLBAR_FIXED", 0, 1),  # 53
        BitField("CONTROL_CONTAINER", 0, 1),  # 52
        BitField("MULTI_LOGIN_USER", 0, 1),  # 51
        BitField("RFCBLOB_DIAG_PARSER", 0, 1),  # 50
        BitField("NEW_MODE_REQUEST", 0, 1),  # 49
        BitField("TABLE_FIELD_NAMES", 0, 1),  # 48

        BitField("R3INFO_USERID", 0, 1),  # 63
        BitField("CHECKRADIO_EVENTS", 0, 1),  # 62
        BitField("R3INFO_FLAGS", 0, 1),  # 61
        BitField("COMBOBOX_TABLE", 0, 1),  # 60
        BitField("ISO_LANGUAGE", 0, 1),  # 59
        BitField("INPUT_REQUIRED", 0, 1),  # 58
        BitField("COMBOBOX", 0, 1),  # 57
        BitField("TYPE_SERVER", 0, 1),  # 56

        BitField("LABEL_OWNER", 0, 1),  # 71
        BitField("EVENT_DESCRIPTION", 0, 1),  # 70
        BitField("SCROLLABLE_TABSTRIP_PAGE", 0, 1),  # 69
        BitField("CONTEXT_MENU", 0, 1),  # 68
        BitField("WIN16", 0, 1),  # 67
        BitField("NUM_FIELD", 0, 1),  # 66
        BitField("USER_TURNTIME2", 0, 1),  # 65
        BitField("R3INFO_ROLLCOUNT", 0, 1),  # 64

        BitField("NOTGUI", 0, 1),  # 79
        BitField("R3INFO_IMODEUUID", 0, 1),  # 78
        BitField("VARINFO_CONTAINER", 0, 1),  # 77
        BitField("PROPFONT_VALID", 0, 1),  # 76
        BitField("TABLE_ROW_REFERENCES_2", 0, 1),  # 75
        BitField("UNUSED_1", 0, 1),  # 74
        BitField("PROPERTY_BAG", 0, 1),  # 73
        BitField("CLICKABLE_FIELD", 0, 1),  # 72

        BitField("ENJOY_IMODEUUID", 0, 1),  # 87
        BitField("CTL_PROPCACHE", 0, 1),  # 86
        BitField("DPLOADONDEMAND", 0, 1),  # 85
        BitField("JAVA_BEANS", 0, 1),  # 84
        BitField("RFC_COMPRESS", 0, 1),  # 83
        BitField("RFC_QUEUE", 0, 1),  # 82
        BitField("XML_BLOBS", 0, 1),  # 81
        BitField("WAN", 0, 1),  # 80

        BitField("HAS_CACHE", 0, 1),  # 95
        BitField("HEX_FIELD", 0, 1),  # 94
        BitField("UNUSED_4", 0, 1),  # 93
        BitField("XML_PROPERTIES", 0, 1),  # 92
        BitField("UNUSED_3", 0, 1),  # 91
        BitField("UNUSED_2", 0, 1),  # 90
        BitField("KEEP_SCROLLPOS", 0, 1),  # 89
        BitField("RFC_ASYNC_BLOB", 0, 1),  # 88

        BitField("UNUSED_6", 0, 1),  # 103
        BitField("COOKIE", 0, 1),  # 102
        BitField("PROPERTYPUMP", 0, 1),  # 101
        BitField("NO_EASYACCESS", 0, 1),  # 100
        BitField("ITS", 0, 1),  # 99
        BitField("ENJOY_IMODEUUID2", 0, 1),  # 98
        BitField("UNUSED_5", 0, 1),  # 97
        BitField("XML_PROP_TABLE", 0, 1),  # 96

        BitField("AREA2FRONT", 0, 1),  # 111
        BitField("EMERGENCY_REPAIR", 0, 1),  # 110
        BitField("CACHED_VSETS", 0, 1),  # 109
        BitField("AUTO_CODEPAGE", 0, 1),  # 108
        BitField("ENTRY_HISTORY", 0, 1),  # 107
        BitField("CONTROL_FOCUS", 0, 1),  # 106
        BitField("DPLOADONDEMAND_WRITE", 0, 1),  # 105
        BitField("SUPPBIT_AREA_SIZE", 0, 1),  # 104

        BitField("NOTIFY_NEWMODE", 0, 1),  # 119
        BitField("SINGLE_SESSION", 0, 1),  # 118
        BitField("UNKNOWN_2", 0, 1),  # 117 (Unknown support bit)
        BitField("PRINTDATA", 0, 1),  # 116
        BitField("WORKPLACE", 0, 1),  # 115
        BitField("EDIT_VARLEN", 0, 1),  # 114
        BitField("AUTORESIZE", 0, 1),  # 113
        BitField("SCROLLBAR_WIDTH", 0, 1),  # 112

        BitField("CONTROL_FOCUS_ON_LIST", 0, 1),  # 127
        BitField("WEBGUI_HELPMODE", 0, 1),  # 126
        BitField("WEBGUI", 0, 1),  # 125
        BitField("DYNAMIC_PASSPORT", 0, 1),  # 124
        BitField("DP_HTTP_PUT", 0, 1),  # 123
        BitField("XMLPROP_DYNPRO", 0, 1),  # 122
        BitField("XMLPROP_CONTAINER", 0, 1),  # 121
        BitField("TOOLBAR_HEIGHT", 0, 1),  # 120

        BitField("ENABLE_UTF8", 0, 1),  # 135
        BitField("ENJOY_IMODEUUID3", 0, 1),  # 134
        BitField("ECATT", 0, 1),  # 133
        BitField("ACCESSIBILITY", 0, 1),  # 132
        BitField("SLC", 0, 1),  # 131
        BitField("GUI_USER_SCRIPTING", 0, 1),  # 130
        BitField("EOKDUMMY_1", 0, 1),  # 129
        BitField("CBU_RBUDUMMY_2", 0, 1),  # 128

        BitField("CBURBU_NEW_STATE", 0, 1),  # 143
        BitField("GUIPATCHLEVEL", 0, 1),  # 142
        BitField("ENABLE_APPL4", 0, 1),  # 141
        BitField("R3INFO_CODEPAGE_APP", 0, 1),  # 140
        BitField("ENABLE_UTF16LE", 0, 1),  # 139
        BitField("ENABLE_UTF16BE", 0, 1),  # 138
        BitField("VARINFO_ICON_TITLE_LIST", 0, 1),  # 137
        BitField("R3INFO_AUTOLOGOUT_TIME", 0, 1),  # 136

        BitField("GUI_USER_SCRIPTING_INFO", 0, 1),  # 151
        BitField("ACC_LIST", 0, 1),  # 150
        BitField("VALUE_4_HISTORY", 0, 1),  # 149
        BitField("SPLITTER", 0, 1),  # 148
        BitField("EVENT_DESCRIPTION_1", 0, 1),  # 147
        BitField("TOP_WINDOW", 0, 1),  # 146
        BitField("GUI_THEME", 0, 1),  # 145
        BitField("BINARY_EVENTID", 0, 1),  # 144

        BitField("DEFAULT_TOOLTIP", 0, 1),  # 159
        BitField("TABSEL_ATTRIB_INPUT", 0, 1),  # 158
        BitField("ACC_LIST_PROPS", 0, 1),  # 157
        BitField("TICKET4GUI", 0, 1),  # 156
        BitField("FRAME_1", 0, 1),  # 155
        BitField("R3INFO_CODEPAGE_APP_1", 0, 1),  # 154
        BitField("DYNT_NOFOCUS", 0, 1),  # 153
        BitField("TEXTEDIT_STREAM", 0, 1),  # 152

        BitField("RFC_SERVER_4_GUI", 0, 1),  # 167
        BitField("OBJECT_NAMES_4_LOGIN_PROCESS", 0, 1),  # 166
        BitField("ITS_PLUGIN", 0, 1),  # 165
        BitField("TABLE_COLUMNWIDTH_INPUT", 0, 1),  # 164
        BitField("CONTROL_FOCUS_ON_LIST_2", 0, 1),  # 163
        BitField("CELLINFO", 0, 1),  # 162
        BitField("CBU_RBUDUMMY_3", 0, 1),  # 161
        BitField("XML_PROP_TABLE_2", 0, 1),  # 160

        BitField("MESSAGE_INFO3", 0, 1),  # 175
        BitField("EOKDUMMY_2", 0, 1),  # 174
        BitField("VARINFO_CONTAINER_LOOP", 0, 1),  # 173
        BitField("R3INFO_KERNEL_VERSION", 0, 1),  # 172
        BitField("WEBSAPCONSOLE", 0, 1),  # 171
        BitField("MENUENTRY_WITH_FCODE", 0, 1),  # 170
        BitField("RCUI", 0, 1),  # 169
        BitField("R3INFO_FLAGS_2", 0, 1),  # 168

        BitField("MESSAGE_SERVICE_REQUEST", 0, 1),  # 183
        BitField("TC_KEEP_SCROLL_POSITION", 0, 1),  # 182
        BitField("DIALOG_STEP_NUMBER", 0, 1),  # 181
        BitField("GUI_PACKET", 0, 1),  # 180
        BitField("DISPLAY_SIZE", 0, 1),  # 179
        BitField("GUIPATCHLEVEL_2", 0, 1),  # 178
        BitField("MAINAREA_SIZE", 0, 1),  # 177
        BitField("SBA2", 0, 1),  # 176

        BitField("ALLWAYS_SEND_CLIENT", 0, 1),  # 191
        BitField("LIST_FULLWIDTH", 0, 1),  # 190
        BitField("TRACE_GUI_CONNECT", 0, 1),  # 189
        BitField("XMLPROP_LIST_DYNPRO", 0, 1),  # 188
        BitField("STD_TOOLBAR_ITEMS", 0, 1),  # 187
        BitField("VARINFO_CONTAINER_1", 0, 1),  # 186
        BitField("MAX_STRING_LEN", 0, 1),  # 185
        BitField("DYNT_FOCUS_FRAME", 0, 1),  # 184

        BitField("EXTMODE_FONT_METRIC", 0, 1),  # 199
        BitField("AMC", 0, 1),  # 198
        BitField("IDA_ALV_FRAGMENTS", 0, 1),  # 197
        BitField("IDA_ALV", 0, 1),  # 196
        BitField("SAP_PERSONAS", 0, 1),  # 195
        BitField("MAX_WSIZE", 0, 1),  # 194
        BitField("GUI_SIGNATURE_COLOR", 0, 1),  # 193
        BitField("UNKNOWN_3", 0, 1),  # 192 (Unknown support bit)

        BitField("AGI_ID_TC", 0, 1),  # 207
        BitField("AGI_ID", 0, 1),  # 206
        BitField("MSGINFO_WITH_CODEPAGE", 0, 1),  # 205
        BitField("CONNECT_CHECK_DONE", 0, 1),  # 204
        BitField("FIORI_MODE", 0, 1),  # 203
        BitField("NO_FOCUS_ON_LIST", 0, 1),  # 202
        BitField("AGI_ID_TS_BUTTON", 0, 1),  # 201
        BitField("GROUPBOX", 0, 1),  # 200

        BitField("padding_bits", 0, 48), ]


bind_diagitem(SAPDiagSupportBits, "APPL", 0x04, 0x0b)
bind_diagitem(SAPDiagSupportBits, "APPL", 0x06, 0x11)


# Support Bits for common SAP Software versions
#
# SAPGUI 7.02 Java rev 5:           ff7ffe2ddab737d674087e1305971597eff23f8d0770ff0f0000000000000000
# SAPGUI 7.02 Java rev 2:           ff7ffe2ddab737d674087e1305971597eff23f8d0770ff030000000000000000
# SAPGUI 7.40 Java rev 8:           ff7ffe2ddab737f674087e9305971597eff2bf8f4f71ff9f8606000000000000
# SAPGUI 7.02 Windows:              ff7ffa0d78b737def6196e9325bf1593ef73feebdb51ed010000000000000000
# SAPGUI 7.01 Windows:              ff7ffa0d78b737def6196e9325bf1593ef73feebdb5501000000000000000000
# SAPGUI 7.30 Windows:              ff7ffa0d78b737def6196e9325bf1597ef73feebdb51ed910200000000000000
# SAPGUI 7.40 Windows:              ff7ffa0d78b737def6196e9325bf1597ef73feebdb51ed91ca00000000000000
# SAPGUI 7.50 Windows:              ff7ffa0d78b737def6196e9325bf1597ef73feebdb51fd91ce2c010000000000
#
# SAP EHP 1 for SAP Netweaver 7.0:  ff7ffe2dd8b737d674087e1305971597ebf22f8d03300f000000000000000000
# SAP EHP 2 for SAP NetWeaver 7.0:  ff7ffe2dd8b737d674087e1305971597ebf23f8d0370ff0f0000000000000000
# SAP NetWeaver AS ABAP 7.50 SP02:  ff7ffe2dd8b737f674087e9305971597ebf2bf8f4b71ff9f8606000000000000
# SAP NetWeaver AS ABAP 7.52 SP01:  ff7ffa0d78b737de76186e9325b71597eb73feebdb51fd91ce24010000000000

support_data_sapgui_701_win = SAPDiagSupportBits(unhex("ff7ffa0d78b737def6196e9325bf1593ef73feebdb5501000000000000000000"))
support_data_sapgui_702_win = SAPDiagSupportBits(unhex("ff7ffa0d78b737def6196e9325bf1593ef73feebdb51ed010000000000000000"))
support_data_sapgui_730_win = SAPDiagSupportBits(unhex("ff7ffa0d78b737def6196e9325bf1597ef73feebdb51ed910200000000000000"))
support_data_sapgui_740_win = SAPDiagSupportBits(unhex("ff7ffa0d78b737def6196e9325bf1597ef73feebdb51ed91ca00000000000000"))
support_data_sapgui_750_win = SAPDiagSupportBits(unhex("ff7ffa0d78b737def6196e9325bf1597ef73feebdb51fd91ce2c010000000000"))
support_data_sapgui_702_java2 = SAPDiagSupportBits(unhex("ff7ffe2ddab737d674087e1305971597eff23f8d0770ff030000000000000000"))
support_data_sapgui_702_java5 = SAPDiagSupportBits(unhex("ff7ffe2ddab737d674087e1305971597eff23f8d0770ff0f0000000000000000"))
support_data_sapgui_740_java8 = SAPDiagSupportBits(unhex("ff7ffe2ddab737f674087e9305971597eff2bf8f4f71ff9f8606000000000000"))
support_data_sapnw_701 = SAPDiagSupportBits(unhex("ff7ffe2dd8b737d674087e1305971597ebf22f8d03300f000000000000000000"))
support_data_sapnw_702 = SAPDiagSupportBits(unhex("ff7ffe2dd8b737d674087e1305971597ebf23f8d0370ff0f0000000000000000"))
support_data_sapnw_750 = SAPDiagSupportBits(unhex("ff7ffe2dd8b737f674087e9305971597ebf2bf8f4b71ff9f8606000000000000"))
support_data_sapnw_752 = SAPDiagSupportBits(unhex("ff7ffa0d78b737de76186e9325b71597eb73feebdb51fd91ce24010000000000"))

support_data = SAPDiagItem(item_type="APPL",
                           item_id="ST_USER",
                           item_sid="SUPPORTDATA",
                           item_value=support_data_sapgui_702_java5)


# Dynt Atom item types
diag_atom_etypes = {
    101: "DIAG_DGOTYP_EFIELD",
    102: "DIAG_DGOTYP_OFIELD",
    103: "DIAG_DGOTYP_KEYWORD",
    104: "DIAG_DGOTYP_CHECKBUTTON_4",
    105: "DIAG_DGOTYP_RADIOBUTTON_0",
    106: "DIAG_DGOTYP_PUSHBUTTON_3",
    107: "DIAG_DGOTYP_FRAME_3",
    108: "DIAG_DGOTYP_LOOP_6",
    109: "DIAG_DGOTYP_SUBSCREEN",
    111: "DIAG_DGOTYP_PROPERTY",
    112: "DIAG_DGOTYP_ICON_0",
    113: "DIAG_DGOTYP_PUSHBUTTON_1",
    114: "DIAG_DGOTYP_FNAME",
    115: "DIAG_DGOTYP_PUSHBUTTON_2",
    116: "DIAG_DGOTYP_TABSTRIP_BUTTON",
    117: "DIAG_DGOTYP_COMBOBOX",
    118: "DIAG_DGOTYP_CHECKBUTTON_1",
    119: "DIAG_DGOTYP_RADIOBUTTON_1",
    120: "DIAG_DGOTYP_XMLPROP",
    121: "DIAG_DGOTYP_EFIELD_1",
    122: "DIAG_DGOTYP_OFIELD_1",
    123: "DIAG_DGOTYP_KEYWORD_1_1",
    124: "DIAG_DGOTYP_CHECKBUTTON_2",
    125: "DIAG_DGOTYP_RADIOBUTTON__0",
    126: "DIAG_DGOTYP_COMBOBOX_1",
    127: "DIAG_DGOTYP_FRAME_1",
    128: "DIAG_DGOTYP_CHECKBUTTON_3",
    129: "DIAG_DGOTYP_RADIOBUTTON_3",
    130: "DIAG_DGOTYP_EFIELD_2",
    131: "DIAG_DGOTYP_OFIELD_2",
    132: "DIAG_DGOTYP_KEYWORD_2",
}
"""Dynt Atom item types"""


# Diag Dynt Atom items
class SAPDiagDyntAtomItem(PacketNoPadded):
    name = "SAP Diag Dynt Atom item"
    fields_desc = [
        ShortField("atom_length", 0),
        ByteField("dlg_flag_1", 0),
        ByteField("dlg_flag_2", 0),
        ByteEnumKeysField("etype", 0, diag_atom_etypes),
        ByteField("area", 0),
        ByteField("block", 0),
        ByteField("group", 0),
        ShortField("row", 0),
        ShortField("col", 0),
        # Attr flags
        BitField("attr_DIAG_BSD_COMBOSTYLE", 0, 1),  # 80
        BitField("attr_DIAG_BSD_YES3D", 0, 1),       # 40
        BitField("attr_DIAG_BSD_PROPFONT", 0, 1),    # 20
        BitField("attr_DIAG_BSD_MATCHCODE", 0, 1),   # 10
        BitField("attr_DIAG_BSD_JUSTRIGHT", 0, 1),   # 08
        BitField("attr_DIAG_BSD_INTENSIFY", 0, 1),   # 04
        BitField("attr_DIAG_BSD_INVISIBLE", 0, 1),   # 02
        BitField("attr_DIAG_BSD_PROTECTED", 0, 1),   # 01

        # DIAG_DGOTYP_FNAME
        ConditionalField(StrLenField("name_text", "", length_from=lambda pkt:pkt.atom_length - 13), lambda pkt:pkt.etype == 114),
        # DIAG_DGOTYP_PUSHBUTTON_2 */
        ConditionalField(ByteField("pushbutton_v_length", 0), lambda pkt:pkt.etype in [115]),
        ConditionalField(ByteField("pushbutton_v_height", 0), lambda pkt:pkt.etype in [115]),
        ConditionalField(ShortField("pushbutton_function_code_offset", 0), lambda pkt:pkt.etype in [115]),
        ConditionalField(ShortField("pushbutton_text_offset", 0), lambda pkt:pkt.etype in [115]),
        ConditionalField(StrField("pushbutton_text", ""), lambda pkt:pkt.etype in [115]),
        ConditionalField(StrField("pushbutton_function_code", ""), lambda pkt:pkt.etype in [115]),
        # DIAG_DGOTYP_TABSTRIP_BUTTON
        ConditionalField(ByteField("tabstripbutton_v_length", 0), lambda pkt:pkt.etype in [116]),
        ConditionalField(ByteField("tabstripbutton_v_height", 0), lambda pkt:pkt.etype in [116]),
        ConditionalField(ByteField("tabstripbutton_page_id", 0), lambda pkt:pkt.etype in [116]),
        ConditionalField(ShortField("tabstripbutton_function_code_offset", 0), lambda pkt:pkt.etype in [116]),
        ConditionalField(ShortField("tabstripbutton_text_offset", 0), lambda pkt:pkt.etype in [116]),
        ConditionalField(ShortField("tabstripbutton_id_offset", 0), lambda pkt:pkt.etype in [116]),
        ConditionalField(StrNullField("tabstripbutton_text", ""), lambda pkt:pkt.etype in [116]),
        ConditionalField(StrNullField("tabstripbutton_function_code", ""), lambda pkt:pkt.etype in [116]),
        ConditionalField(StrNullField("tabstripbutton_id", ""), lambda pkt:pkt.etype in [116]),
        # DIAG_DGOTYP_XMLPROP
        ConditionalField(StrLenField("xmlprop_text", "", length_from=lambda pkt:pkt.atom_length - 13), lambda pkt:pkt.etype == 120),
        # DIAG_DGOTYP_EFIELD_1 or DIAG_DGOTYP_OFIELD_1 or DIAG_DGOTYP_KEYWORD_1
        ConditionalField(ByteField("field1_flag1", 0), lambda pkt:pkt.etype in [121, 122, 123]),
        ConditionalField(FieldLenField("field1_dlen", None, fmt="B", length_of="field1_text"), lambda pkt:pkt.etype in [121, 122, 123]),
        ConditionalField(ByteField("field1_mlen", 0), lambda pkt:pkt.etype in [121, 122, 123]),
        ConditionalField(ShortField("field1_maxnrchars", 0), lambda pkt:pkt.etype in [121, 122, 123]),
        ConditionalField(StrLenField("field1_text", "", length_from=lambda pkt:pkt.field1_dlen), lambda pkt:pkt.etype in [121, 122, 123]),
        # DIAG_DGOTYP_FRAME_1
        ConditionalField(ShortField("frame_drows", 0), lambda pkt:pkt.etype in [127]),
        ConditionalField(ShortField("frame_dcols", 0), lambda pkt:pkt.etype in [127]),
        ConditionalField(StrLenField("frame_text", "", length_from=lambda pkt:pkt.atom_length - 17), lambda pkt:pkt.etype in [127]),
        # DIAG_DGOTYP_RADIOBUTTON_3
        ConditionalField(ByteField("radiobutton_button", 0), lambda pkt:pkt.etype in [129]),
        ConditionalField(ShortField("radiobutton_visible_label_length", 0), lambda pkt:pkt.etype in [129]),
        ConditionalField(ShortField("radiobutton_event_id_off", 0), lambda pkt:pkt.etype in [129]),
        ConditionalField(ByteField("radiobutton_event_id_len", 0), lambda pkt:pkt.etype in [129]),
        ConditionalField(ShortField("radiobutton_text_off", 0), lambda pkt:pkt.etype in [129]),
        ConditionalField(ShortField("radiobutton_text_length", 0), lambda pkt:pkt.etype in [129]),
        ConditionalField(StrLenField("radiobutton_text", "", length_from=lambda pkt:pkt.radiobutton_event_id_len + pkt.radiobutton_text_length), lambda pkt:pkt.etype in [129]),
        # DIAG_DGOTYP_EFIELD_2 or DIAG_DGOTYP_OFIELD_2 or DIAG_DGOTYP_KEYWORD_2
        ConditionalField(ShortField("field2_flag1", 0), lambda pkt:pkt.etype in [130, 131, 132]),
        ConditionalField(FieldLenField("field2_dlen", None, fmt="B", length_of="field2_text"), lambda pkt:pkt.etype in [130, 131, 132]),
        ConditionalField(ByteField("field2_mlen", 0), lambda pkt:pkt.etype in [130, 131, 132]),
        ConditionalField(ShortField("field2_maxnrchars", 0), lambda pkt:pkt.etype in [130, 131, 132]),
        ConditionalField(StrLenField("field2_text", "", length_from=lambda pkt:pkt.field2_dlen), lambda pkt:pkt.etype in [130, 131, 132]),
        # Remaining types
        ConditionalField(StrLenField("value", "", length_from=lambda pkt:pkt.atom_length - 13), lambda pkt:pkt.etype not in [114, 115, 116, 120, 121, 122, 123, 127, 129, 130, 131, 132]),
    ]

    def post_build(self, p, pay):
        if pay is None:
            pay = ''
        # Update the atom_length field (first 2 bytes) with the packet length
        p = pack("!H", len(p)) + p[2:]
        return p + pay


# Diag Dynt Atom container
class SAPDiagDyntAtom(Packet):
    name = "SAP Diag Dynt Atom"
    fields_desc = [
        PacketListField("items", None, SAPDiagDyntAtomItem)
    ]


bind_diagitem(SAPDiagDyntAtom, "APPL", 0x09, 0x02)
bind_diagitem(SAPDiagDyntAtom, "APPL", 0x09, 0x10)


# Diag Area Size
class SAPDiagAreaSize(PacketNoPadded):
    name = "SAP Diag Area Size"
    fields_desc = [
        IntField("window_height", 0),
        IntField("window_width", 0),
        IntField("area_height", 0),
        IntField("area_width", 0)
    ]


bind_diagitem(SAPDiagAreaSize, "APPL", 0x0c, 0x07)


# Diag Window Size
class SAPDiagWindowSize(PacketNoPadded):
    name = "SAP Diag Window Size"
    fields_desc = [
        IntField("window_height", 0),
        IntField("window_width", 0),
        IntField("area_height", 0),
        IntField("area_width", 0)
    ]


bind_diagitem(SAPDiagWindowSize, "APPL", 0x04, 0x0d)


# Diag Menu Entry
class SAPDiagMenuEntry(PacketNoPadded):
    name = "SAP Diag Menu Entry"
    fields_desc = [
        ShortField("length", 0),
        ByteField("position_1", 0),
        ByteField("position_2", 0),
        ByteField("position_3", 0),
        ByteField("position_4", 0),
        # Menu Entry Flags
        BitField("flag_TERM_??8", 0, 1),  # 80
        BitField("flag_TERM_??7", 0, 1),  # 40
        BitField("flag_TERM_??6", 0, 1),  # 20
        BitField("flag_TERM_VKEY", 0, 1),  # 10
        BitField("flag_TERM_SEP", 0, 1),  # 8
        BitField("flag_TERM_MEN", 0, 1),  # 4
        BitField("flag_TERM_SEL", 0, 1),  # 2
        BitField("flag_TERM_??1", 0, 1),  # 1
        ByteField("virtual_key", 0),
        ByteField("return_code_1", 0),
        ByteField("return_code_2", 0),
        ByteField("return_code_3", 0),
        ByteField("return_code_4", 0),
        ByteField("return_code_5", 0),
        ByteField("return_code_6", 0),
        ByteField("function_code_1", 0),
        ByteField("function_code_2", 0),
        ByteField("function_code_3", 0),
        ByteField("function_code_4", 0),
        ByteField("function_code_5", 0),
        ByteField("function_code_6", 0),
        StrNullField("text", ""),
        StrNullField("accelerator", ""),
        StrNullField("info", ""),
    ]


# Diag Menu Entries Container
class SAPDiagMenuEntries(Packet):
    name = "SAP Diag Menu Entries"
    fields_desc = [
        PacketListField("entries", None, SAPDiagMenuEntry)
    ]


bind_diagitem(SAPDiagMenuEntries, "APPL", 0x0b, 0x01)
bind_diagitem(SAPDiagMenuEntries, "APPL", 0x0b, 0x02)
bind_diagitem(SAPDiagMenuEntries, "APPL", 0x0b, 0x03)
bind_diagitem(SAPDiagMenuEntries, "APPL", 0x0b, 0x04)


# Diag UI Event Type values
diag_ui_event_type_values = {
    0x01: "SELECT",
    0x02: "HE",
    0x03: "VALUEHELP",
    0x06: "RESIZE",
    0x07: "FUNCTIONKEY",
    0x08: "SCROLL",
    0x09: "BUTTONPRESSED",
    0x0a: "VALUECHANGED",
    0x0b: "STATECHANGED",
    0x0c: "NAVIGATION",
}
"""Diag UI Event Type values"""

# Diag UI Event Control Type values
diag_ui_event_control_values = {
    0x00: "NONE",
    0x01: "FIELD",
    0x02: "RADIOBUTTON",
    0x03: "CHECKBUTTON",
    0x04: "MENUBUTTON",
    0x05: "TOOLBARBUTTON",
    0x06: "STANDARDTOOLBARBUTTON",
    0x07: "PUSHBUTTON",
    0x08: "TABLEVIEW",
    0x09: "TABSTRIP",
    0x0a: "DYNPRO",
    0x0b: "CUSTOM_CONTROL",
    0x0d: "FRAME",
    0x0e: "TABLEVIEW_COLSEL_BUTTON",
    0x0f: "TABLEVIEW_ROWSEL_BUTTON",
    0x10: "TABLEVIEW_CELL",
    0x11: "CONTEXTMENU",
    0x12: "SPLITTER",
    0x13: "MESSAGE",
    0x14: "OKCODE",
    0x15: "ACC_CONTAINER",
}
"""Diag UI Event Control Type values"""

# Diag UI Event Navigation Data values
diag_ui_event_navigation_data_values = {
    0x01: "TAB",
    0x02: "TAB_BACK",
    0x03: "JUMP_OVER",
    0x04: "JUMP_OVER_BACK",
    0x05: "JUMP_OUT",
    0x06: "JUMP_OUT_BACK",
    0x07: "JUMP_SECTION",
    0x08: "JUMP_SECTION_BACK",
    0x09: "FIRST_FIELD",
    0x0a: "LAST_FIELD",
}
"""Diag UI Event Navigation Data values"""


# Diag UI Event Source
class SAPDiagUIEventSource(PacketNoPadded):
    name = "UI Event Source"
    fields_desc = [
        BitField("valid_unused", 0, 4),
        BitField("valid_functionkey_data", 0, 1),
        BitField("valid_navigation_data", 0, 1),
        BitField("valid_control_pos", 0, 1),
        BitField("valid_menu_pos", 0, 1),
        ShortEnumKeysField("event_type", 0, diag_ui_event_type_values),
        ShortEnumKeysField("control_type", 0, diag_ui_event_control_values),
        ConditionalField(ByteEnumKeysField("navigation_data", 0, diag_ui_event_navigation_data_values), lambda pkt:pkt.valid_navigation_data),
        ConditionalField(ByteField("event_data", 0), lambda pkt:not pkt.valid_navigation_data),
        ShortField("control_row", 0),
        ShortField("control_col", 0),
        FieldLenField("container_nrs", None, count_of="containers"),
        FieldListField("containers", None, ByteField("container", 0), count_from=lambda x:x.container_nrs)
    ]


bind_diagitem(SAPDiagUIEventSource, "APPL", 0x0f, 0x01)


# Diag SES
class SAPDiagSES(PacketNoPadded):
    name = "SES"
    fields_desc = [
        ByteField("eventarray", 0),
        ByteField("eventid1", 0),
        ByteField("eventid2", 0),
        ByteField("eventid3", 0),
        ByteField("eventid4", 0),
        ByteField("eventid5", 0),
        ByteField("screen_flag", 0),
        ByteField("modal_no", 0),
        ByteField("x_pos", 0),
        ByteField("y_pos", 0),
        ByteField("imode", 0),
        ByteField("flag_1", 0),
        ShortField("padd", 0),
        ByteField("dim_row", 0),
        ByteField("dim_col", 0)
    ]


bind_diagitem(SAPDiagSES, 0x01)
