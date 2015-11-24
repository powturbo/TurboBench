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

# Standard imports
from struct import unpack
from collections import defaultdict
# External imports
from scapy.layers.inet import TCP
from scapy.packet import Packet, bind_layers
from scapy.fields import (ByteEnumField, IntField, ByteField, LenField,
                          StrFixedLenField, ConditionalField, FieldLenField,
                          PacketListField, BitField, LEIntField, PacketField,
                          SignedIntField, StrField)
# Custom imports
import pysapcompress
from pysap.SAPNI import SAPNI
from pysap.SAPSNC import SAPSNCFrame
from pysap.utils import (PacketNoPadded, ByteEnumKeysField,
                         ByteMultiEnumKeysField, MutablePacketField,
                         SignedShortField, StrNullFixedLenField,
                         StrEncodedPaddedField)
from pysapcompress import DecompressError, CompressError


class SAPDiagDP(Packet):
    """SAP Diag DP Header packet

    This packet is used for initialization of Diag connections. Usually
    there's no need to change any value more that the terminal.
    """
    name = "SAP Diag DP Header"
    fields_desc = [  # DP Header
        SignedIntField("request_id", -1),
        ByteField("retcode", 0x0a),
        ByteField("sender_id", 0),
        ByteField("action_type", 0),
        IntField("req_info", 0),
        SignedIntField("tid", -1),
        SignedShortField("uid", -1),
        ByteField("mode", 0xff),
        SignedIntField("wp_id", -1),
        SignedIntField("wp_ca_blk", -1),
        SignedIntField("appc_ca_blk", -1),
        LenField("length", None, fmt="<I"),  # The length in the DP Header is the length of the Diag Header (8 bytes) + the Diag Message
                                             # (54 bytes for user_connect+support_data). As the DP Header is the layer after NI during
                                             # initialization, a LenField works fine.
        ByteField("new_stat", 0),
        SignedIntField("unused1", -1),
        SignedShortField("rq_id", -1),
        StrFixedLenField("unused2", "\x20" * 40, 40),
        StrFixedLenField("terminal", "\x00" * 15, 15),
        StrFixedLenField("unused3", "\x00" * 10, 10),
        StrFixedLenField("unused4", "\x20" * 20, 20),
        IntField("unused5", 0),
        IntField("unused6", 0),
        SignedIntField("unused7", -1),
        IntField("unused8", 0),
        ByteField("unused9", 0x01),
        StrFixedLenField("unused10", "\x00" * 57, 57)]

# Diag Item Types
diag_item_types = {
    0x01: "SES",
    0x02: "ICO",
    0x03: "TIT",
    0x07: "DiagMessage",
    0x08: "OKC",
    0x09: "CHL",
    0x0b: "SBA",  # SBA/SFE/SLC
    0x0c: "EOM",
    0x10: "APPL",
    0x11: "DIAG_XMLBLOB",
    0x12: "APPL4",
    0x15: "SBA2",
}
"""Diag Item Types"""

# Diag APPL/APPL4 IDs
diag_appl_ids = {
    0x01: "SCRIPT",
    0x02: "GRAPH",
    0x03: "IXOS",
    0x04: "ST_USER",
    0x05: "DYNN",
    0x06: "ST_R3INFO",
    0x07: "POPU",
    0x08: "RFC_TR",
    0x09: "DYNT",
    0x0a: "CONTAINER",
    0x0b: "MNUENTRY",
    0x0c: "VARINFO",
    0x0e: "CONTROL",
    0x0f: "UI_EVENT",
    0x12: "ACC_LIST",
    0x13: "RCUI",
    0x14: "GUI_PACKET",
}
"""Diag APPL/APPL4 IDs"""

# Diag APPL/APPL4 SIDs
diag_appl_sids = {
    0x01:    # SCRIPT
    {0x01: "SCRIPT_OTF",
     0x02: "SCRIPT_SCREEN",
     0x03: "SCRIPT_POSTSCRIPT",
     0x04: "SCRIPT_ITF"},
    0x02:     # GRAPH
    {0x03: "GRAPH RELEASE 3",
     0x05: "GRAPH RELEASE 5"},
    0x03:    # IXOS
    {0x01: "ABLAGE",
     0x02: "ANZEIGE",
     0x03: "IXOS_COMMAND"},
    0x04:     # ST_USER
    {0x01: "V1",
     0x02: "CONNECT",
     0x03: "SELECTEDRECT",
     0x04: "FONTMETRIC",
     0x05: "TABLEMETRIC",
     0x06: "GUITIME",
     0x07: "GUITIMEZONE",
     0x08: "TURNTIME",
     0x09: "GUIVERSION",
     0x0b: "SUPPORTDATA",
     0x0c: "RFC_CONNECT",
     0x0d: "WSIZE",
     0x0e: "V2",
     0x0f: "TURNTIME2",
     0x10: "RFC_PARENT_UUID",
     0x11: "RFC_NEW_UUID",
     0x12: "RFC_UUIDS",
     0x13: "RFC_UUIDS2",
     0x14: "XML_LOGIN",
     0x15: "XML_TRANSACTION",
     0x16: "SCROLLBAR_WIDTH",
     0x17: "TOOLBAR_HEIGHT",
     0x18: "PASSPORT_DATA",
     0x19: "GUI_STATE",
     0x1a: "DECIMALPOINT",
     0x1b: "LANGUAGE",
     0x1c: "USERNAME",
     0x1d: "GUIPATCHLEVEL",
     0x1e: "WSIZE_PIXEL",
     0x1f: "GUI_OS_VERSION",
     0x20: "BROWSER_VERSION",
     0x21: "OFFICE_VERSION",
     0x22: "JDK_VERSION",
     0x23: "GUIXT_VERSION",
     0x24: "DISPLAY_SIZE",
     0x25: "GUI_TYPE",
     0x26: "DIALOG_STEP_NUMBER"},
    0x05:    # DYNN
    {0x01: "CHL",
     0x03: "XMLPROP DYNPRO"},
    0x06:    # ST_R3INFO
    {0x01: "MODENUMBER",
     0x02: "DBNAME",
     0x03: "CPUNAME",
     0x04: "RFC_TRIGGER",
     0x05: "GUI_LABEL",
     0x06: "DIAGVERSION",
     0x07: "TCODE",
     0x08: "RFC_WAITING",
     0x09: "RFC_REFRESH",
     0x0a: "IMODENUMBER",
     0x0b: "MESSAGE",
     0x0c: "CLIENT",
     0x0d: "DYNPRONAME",
     0x0e: "DYNPRONUMBER",
     0x0f: "CUANAME",
     0x10: "CUASTATUS",
     0x11: "SUPPORTDATA",
     0x12: "RFC_CONNECT_OK",
     0x13: "GUI_FKEY",
     0x14: "GUI_FKEYT",
     0x15: "STOP_TRANS",
     0x16: "RFC_DIAG_BLOCK_SIZE",
     0x17: "USER_CHECKED",
     0x18: "FLAGS",
     0x19: "USERID",
     0x1a: "ROLLCOUNT",
     0x1b: "GUI_XT_VAR",
     0x1c: "IMODEUUID",
     0x1d: "IMODEUUID_INVALIDATE",
     0x1e: "IMODEUUIDS",
     0x1f: "IMODEUUIDS2",
     0x20: "CODEPAGE",
     0x21: "CONTEXTID",
     0x22: "AUTOLOGOUT_TIME",
     0x23: "CODEPAGE_DIAG_GUI",
     0x24: "CODEPAGE_APP_SERVER",
     0x25: "GUI_THEME",
     0x26: "GUI_USER_SCRIPTING",
     0x27: "CODEPAGE_APP_SERVER_1",
     0x28: "TICKET4GUI",
     0x29: "KERNEL_VERSION",
     0x2a: "STD_TOOLBAR_ITEMS", },
    0x07:    # POPU
    {0x02: "DEST"},
    0x08:    # RFC_TR
    {0x00: "RFC_TR_REQ",
     0x01: "RFC_TR_RET",
     0x02: "RFC_TR_ERR",
     0x03: "RFC_TR_RQT",
     0x04: "RFC_TR_MOR",
     0x05: "RFC_TR_MOB",
     0x06: "RFC_TR_RNB",
     0x07: "RFC_TR_RNT",
     0x08: "RFC_TR_DIS",
     0x09: "RFC_TR_CALL",
     0x0a: "RFC_TR_CALL_END",
     0x0b: "RFC_TR_RES"},
    0x09:    # DYNT
    {0x01: "DYNT_FOCUS",
     0x02: "DYNT_ATOM",
     0x03: "DYNT_EVENT_UNUSED",
     0x04: "TABLE_ROW_REFERENCE",
     0x05: "TABLE_ROW_DAT_INPUT_DUMMY",
     0x06: "TABLE_INPUT_HEADER",
     0x07: "TABLE_OUTPUT_HEADER",
     0x08: "TABLE_ROW_DATA_INPUT",
     0x09: "TABLE_ROW_DATA_OUTPUT",
     0x0a: "DYNT_NOFOCUS",
     0x0b: "DYNT_FOCUS_1",
     0x0c: "TABLE_ROW_REFERENCE_1",
     0x0d: "TABLE_FIELD_NAMES",
     0x0e: "TABLE_HEADER",
     0x0f: "DYNT_TABSTRIP_HEADER",
     0x10: "DYNT_TABSTRIP_BUTTONS",
     0x11: "TABLE_ROW_REFERENCE_2",
     0x12: "DYNT_CONTROL_FOCUS",
     0x15: "DYNT_TC_COLUMN_TITLE_XMLP",
     0x16: "DYNT_TC_ROW_SELECTOR_NAME",
     0x17: "DYNT_FOCUS_FRAME"},
    0x0a:    # CONTAINER
    {0x01: "RESET",
     0x02: "DEFAULT",
     0x03: "SUBSCREEN",
     0x04: "LOOP",
     0x05: "TABLE",
     0x06: "NAME",
     0x08: "TABSTRIP",
     0x09: "TABSTRIP_PAGE",
     0x0a: "CONTROL",
     0x0c: "XMLPROP",
     0x0d: "SPLITTER",
     0x0e: "SPLITTER_CELL"},
    0x0b:    # MNUENTRY
    {0x01: "MENU_ACT",
     0x02: "MENU_MNU",
     0x03: "MENU_PFK",
     0x04: "MENU_KYB"},
    0x0c:    # VARINFO
    {0x01: "MESTYPE",
     0x02: "SCROLL_INFOS",
     0x03: "MESTYPE2",
     0x04: "OKCODE",
     0x05: "CONTAINER",
     0x06: "SCROLL_INFOS2",
     0x07: "AREASIZE",
     0x08: "AREA_PIXELSIZE",
     0x09: "SESSION_TITLE",
     0x0a: "SESSION_ICON",
     0x0b: "LIST_CELL_TEXT",
     0x0c: "CONTAINER_LOOP",
     0x0d: "LIST_FOCUS",
     0x0e: "MAINAREA_PIXELSIZE",
     0x0f: "SERVICE_REQUEST"},
    0x0e:    # CONTROL
    {0x01: "CONTROL_PROPERTIES"},
    0x0f:    # UI_EVENT
    {0x01: "UI_EVENT_SOURCE"},
    0x12:    # ACC_LIST
    {0x01: "ACC_LIST_INFO4FIELD",
     0x02: "ACC_LIST_CONTAINER"},
    0x13:    # RCUI
    {0x01: "RCUI_STREAM",
     0x02: "RCUI_SYSTEM_ERROR",
     0x03: "RCUI_SPAGPA",
     0x04: "RCUI_MEMORYID",
     0x05: "RCUI_TXOPTION",
     0x06: "RCUI_VALUE",
     0x07: "RCUI_COMMAND",
     0x08: "RCUI_BDCMSG",
     0x09: "RCUI_CONNECT_DATA"},
    0x14:    # GUI_PACKET
    {0x01: "GUI_PACKET_STATE",
     0x02: "GUI_PACKET_DATA"}
}
"""Diag APPL/APPL4 SIDs"""


def diag_item_is_short(item):
    """Returns if the item has a short length field

    @param item: item to look at
    @type item: L{SAPDiagItem}

    @return: if the item has a short length field (2 bytes)
    @rtype: C{bool}
    """
    return item.item_type == 0x10  # APPL


def diag_item_is_long(item):
    """Returns if the item has a long length field

    @param item: item to look at
    @type item: L{SAPDiagItem}

    @return: if the item has a long length field (4 bytes)
    @rtype: C{bool}
    """
    return item.item_type in [0x11, 0x12]    # DIAG_XMLBLOB or APPL4


def diag_item_is_appl_appl4(item):
    """Returns if an item is APPL or APPL4

    @param item: item to look a
    @type item: L{SAPDiagItem}

    @return: whether the item is a APPL or APPL4 item
    @rtype: C{bool}
    """
    return item.item_type in [0x10, 0x12]    # APPL or APPL4


def diag_item_get_length(item):
    """Returns the item length according to the item_type

    @param item: item to look at
    @type item: L{SAPDiagItem}

    @return: the item length
    @rtype: C{int}
    """
    diag_item_sizes = {
        0x01: 16,                   # SES
        0x02: 20,                   # ICO
        0x03: 3,                    # TIT
        0x07: 76,                   # DiagMessage (old format)
        0x08: 0,                    # OKC XXX: Never seen this, check proper size
        0x09: 22,                   # CHL
        0x0b: 9,                    # SBA/SFE/SLC
        0x0c: 0,                    # EOM
        0x10: item.item_length,     # APPL
        0x11: item.item_length4,    # DIAG_XMLBLOB
        0x12: item.item_length4,    # APPL4
        0x15: 36,                   # SBA2
    }
    return diag_item_sizes[item.item_type]


diag_item_classes = {}
"""Dictionary for registering Diag item classes """


diag_item_appl_classes = defaultdict(defaultdict)
"""Dictionary for registering Diag APPL/APPL4 item classes """


def bind_diagitem(item_class, item_type, item_id=None, item_sid=None):
    """Registers a Diag item class associated to a given type, ID and SID.

    @param item_class: item class to associate
    @type item_class: L{SAPDiagItem} class

    @param item_id: item ID to associate
    @type item_id: C{int}

    @param item_sid: item SID to associate
    @type item_sid: C{int}
    """
    if item_type in [0x10, 0x12, "APPL", "APPL4"]:
        diag_item_appl_classes[item_id][item_sid] = item_class
    else:
        diag_item_classes[item_type] = item_class


def diag_item_get_class(pkt, item_type, item_id, item_sid):
    """Obtains the Diag item class according to the type, ID and SID of the packet.
    If the Type/ID/SID is not registered, returns None.

    @param pkt: the item to look at
    @type pkt: L{SAPDiagItem}

    @param item_type: function that returns the item type
    @type item_type: C{int}

    @param item_id: function that returns the item ID
    @type item_id: C{int}

    @param item_sid: functions that returns the item SID
    @type item_sid: C{int}

    @return: the associated L{SAPDiagItem} class if registered or None
    """
    if item_type in [0x10, 0x12, "APPL", "APPL4"]:
        if item_id in diag_item_appl_classes and item_sid in diag_item_appl_classes[item_id]:
            return diag_item_appl_classes[item_id][item_sid]
        else:
            return None
    else:
        return diag_item_classes.get(item_type)


class SAPDiagItem(PacketNoPadded):
    """SAP Diag Item packet

    This packet holds the different types of Diag items. The value field is
    interpreted according to the Type/ID/SID specified for the item.
    """
    name = "SAP Diag Item"
    fields_desc = [ByteEnumKeysField("item_type", 0, diag_item_types),
                   ConditionalField(ByteEnumKeysField("item_id", 0, diag_appl_ids), diag_item_is_appl_appl4),
                   ConditionalField(ByteMultiEnumKeysField("item_sid", 0, diag_appl_sids, depends_on=lambda item:item.item_id, fmt="B"), diag_item_is_appl_appl4),
                   ConditionalField(FieldLenField("item_length", None, length_of="item_value", fmt="!H"), diag_item_is_short),
                   ConditionalField(FieldLenField("item_length4", None, length_of="item_value", fmt="!I"), diag_item_is_long),
                   MutablePacketField("item_value", None,
                                      length_from=diag_item_get_length,
                                      get_class=diag_item_get_class,
                                      evaluators=[lambda item:item.item_type,
                                                  lambda item:item.item_id,
                                                  lambda item:item.item_sid],
                                      )
                   ]


# SAP Diag Items container
class SAPDiagItems(Packet):
    """SAP Diag Items container

    Container for L{SAPDiagItem} packets.
    """
    name = "SAP Diag Items"
    fields_desc = [PacketListField("message", None, SAPDiagItem)]

# Compression Flag values
diag_compress_values = {0: "Compression switched off",
                        1: "Compression switched on",
                        2: "Data encrypted",
                        3: "Data encrypted wrap"}
"""Compression Flag values"""


# SAP Diag packet
class SAPDiag(PacketNoPadded):
    """SAP Diag packet

    This packet holds the Diag Header and serve as a container for
    L{SAPDiagItem} items. It handles compression/decompression, adding the
    appropriate Compression Header when necessary.
    """
    name = "SAP Diag"
    fields_desc = [
        ByteField("mode", 0),

        # Communication flags
        BitField("com_flag_TERM_GRA", 0, 1),
        BitField("com_flag_TERM_NNM", 0, 1),
        BitField("com_flag_TERM_CAS", 0, 1),
        BitField("com_flag_TERM_INI", 0, 1),
        BitField("com_flag_TERM_EOP", 0, 1),
        BitField("com_flag_TERM_NOP", 0, 1),
        BitField("com_flag_TERM_EOC", 0, 1),
        BitField("com_flag_TERM_EOS", 0, 1),

        ByteField("mode_stat", 0),
        ByteField("err_flag", 0),
        ByteField("msg_type", 0),
        ByteField("msg_info", 0),
        ByteField("msg_rc", 0),
        ByteEnumKeysField("compress", 0, diag_compress_values),

        # Compression Header
        ConditionalField(LEIntField("uncompress_length", None), lambda pkt:pkt.compress == 1),
        ConditionalField(ByteEnumField("algorithm", 0x12, {0x12: "LZH", 0x10: "LZC"}), lambda pkt: pkt.compress == 1),
        ConditionalField(StrFixedLenField("magic_bytes", "\x1f\x9d", 2), lambda pkt: pkt.compress == 1),
        ConditionalField(ByteField("special", 2), lambda pkt: pkt.compress == 1),

        # SNC Frame
        ConditionalField(PacketField("snc_frame", None, SAPSNCFrame), lambda pkt: pkt.compress in [2, 3]),

        # Message info
        ConditionalField(StrEncodedPaddedField("info", None), lambda pkt: pkt.err_flag != 0),

        # Payload
        PacketListField("message", None, SAPDiagItem)]

    def do_compress(self, s):
        """Compress a string using SAP compression C++ extension.

        @param s: string to compress
        @type s: C{string}

        @return: string compression header plus the compressed string
        @rtype: C{string}

        @raise pysapcompress.Error: when a compression error is raised
        """
        if (len(s) > 0):
            # Compress the payload and return the output
            (_, _, outbuffer) = pysapcompress.compress(s, pysapcompress.ALG_LZH)
            return outbuffer

    def do_decompress(self, s, length):
        """Decompress a string using SAP compression C++ extension.

        @param s: compression header plus compressed string
        @type s: C{string}

        @param length: reported compressed length
        @type length: C{int}

        @return: decompressed string
        @rtype: C{string}

        @raise pysapcompress.Error: when a decompression error is raised
        """
        if (len(s) > 0):
            # Decompress the payload and return the output
            (_, _, outbuffer) = pysapcompress.decompress(s, length)
            return outbuffer

    def pre_dissect(self, s):
        """Prepares the packet for dissection. If the compression flag is set,
        decompress the payload.
        """
        # If the compression flag is set, decompress everything after the headers
        if s[7] == "\x01":
            # First need to get the reported decompressed length
            (reported_length, ) = unpack("<I", s[8:12])

            # Then return the headers (Diag and Compression) and the payload (message field)
            try:
                return s[:16] + self.do_decompress(s[8:], reported_length)
            except DecompressError:
                return s
        # Uncompressed packet, just return them
        return s

    def post_build(self, p, pay):
        """Compress the payload. If the compression flag is set, compress both
        the message field and the payload.
        """
        if pay is None:
            pay = ''
        if self.compress == 1:
            payload = "".join([str(item) for item in self.message]) + pay
            if (len(payload) > 0):
                try:
                    return p[:8] + self.do_compress(payload)
                except CompressError:
                    return p + pay
        return p + pay

    def get_item(self, item_type=None, item_id=None, item_sid=None):
        """Get an item from the packet's message. Returns None if the message
        is not found, or a list if the item is found multiple times.

        @param item_type: item type byte or string value
        @type item_type: C{int} or C{string} or C{list}

        @param item_id: item ID byte or string value
        @type item_id: C{int} or C{string} or C{list}

        @param item_sid: item SID byte or string value
        @type item_sid: C{int} or C{string} or C{list}

        @return: list of items found on the packet or None
        @rtype: C{list} of L{SAPDiagItem}
        """
        # Expand list lookups
        items = []
        if item_type is not None and type(item_type) == list:
            for itype in item_type:
                items.extend(self.get_item(itype, item_id, item_sid))
            return items
        if item_id is not None and type(item_id) == list:
            for iid in item_id:
                items.extend(self.get_item(item_type, iid, item_sid))
            return items
        if item_sid is not None and type(item_sid) == list:
            for isid in item_sid:
                items.extend(self.get_item(item_type, item_id, isid))
            return items

        # Perform name lookups
        if item_type is not None and isinstance(item_type, str):
            item_type = list(diag_item_types.keys())[list(diag_item_types.values()).index(item_type)]
        if item_id is not None and isinstance(item_id, str):
            item_id = list(diag_appl_ids.keys())[list(diag_appl_ids.values()).index(item_id)]
        if item_sid is not None and isinstance(item_sid, str):
            item_sid = list(diag_appl_sids[item_id].keys())[list(diag_appl_sids[item_id].values()).index(item_sid)]

        # Filter and return items
        if item_sid is None and item_id is None:
            items = [item for item in self.message if hasattr(item, "item_type") and item.item_type == item_type]
        elif item_sid is None:
            items = [item for item in self.message if hasattr(item, "item_type") and item.item_type == item_type and item.item_id == item_id]
        else:
            items = [item for item in self.message if hasattr(item, "item_type") and item.item_type == item_type and item.item_id == item_id and item.item_sid == item_sid]

        return items


class SAPDiagError(PacketNoPadded):
    """SAP Diag Error packet

    This packet holds Diag error packets.
    """
    name = "SAP Diag Error"
    # TODO: Need to figure out the meaning of the packets
    fields_desc = [
        StrNullFixedLenField("msg", "**DPTMMSG**", length=12),
        StrField("padd", None),
    ]


# Bind SAP NI with the Diag port
bind_layers(TCP, SAPNI, dport=3200)


def diag_guess_diagdp_header(self, payload):
    """Guess if the payload is a L{SAPDiag} or L{SAPDiagDP}, base on the mode field.
    Use this function as guess_payload_class for the L{SAPNI} packet if need to
    dissect L{SAPDiag} packets.
    """
    if self.length == 14 and payload.startswith("**DPTMMSG**\x00"):
        return SAPDiagError
    elif self.length == 17 and payload.startswith("**DPTMOPC**\x00"):
        return SAPDiagError
    elif self.length > 200 + 8 and payload[0] == "\xff":
        return SAPDiagDP
    else:
        return SAPDiag
