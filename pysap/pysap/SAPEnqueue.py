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
import struct
import logging
from scapy.layers.inet import TCP
from scapy.packet import bind_layers
from scapy.fields import (IntField, IntEnumField, PacketListField,
                          StrFixedLenField, ConditionalField, ByteField,
                          FieldLenField, LenField, StrNullField)
# Custom imports
from pysap.SAPNI import SAPNI, SAPNIStreamSocket
from pysap.utils import ByteEnumKeysField, PacketNoPadded, StrNullFixedLenField


# Create a logger for the SAEnqueue layer
log_sapenqueue = logging.getLogger("pysap.sapenqueue")


# Enqueue Server Type values
enqueue_type_values = {
    0x00: "SYNC_REQUEST",
    0x01: "ASYNC_REQUEST",
    0x02: "RESPONSE",
}
"""Enqueue Server Type values"""


# Enqueue Server Destination values
enqueue_dest_values = {
    0x01: "SYNC_ENQUEUE",
    0x02: "ASYNC_ENQUEUE",
    0x03: "SERVER_ADMIN",
    0x05: "STAT_QUERY",
    0x06: "CONECTION_ADMIN",
    0x07: "ENQ_TO_REP",
    0x08: "REP_TO_ENQ",
}
"""Enqueue Server Destination values"""


# Enqueue Server Admin Opcode values
enqueue_server_admin_opcode_values = {
    0x01: "EnAdmDummyRequest",
    0x02: "EnAdmShutdownRequest",
    0x04: "EnAdmGetReplInfoRequest",
    0x06: "EnAdmTraceRequest",
}
"""Enqueue Server Admin Opcode values"""


# Enqueue Server Admin Trace Action values
enqueue_server_admin_trace_action_values = {
    0x01: "Raise level",
    0x02: "Lower level",
    0x03: "Get trace state",
    0x04: "Set trace status",
    0x05: "Reset trace files",
}
"""Enqueue Server Admin Trace Action values"""


# Enqueue Server Admin Trace Limit values
enqueue_server_admin_trace_limit_values = {
    0x00: "Globally",
    0x01: "Only in enserver",
    0x02: "Only in repserver",
    0x03: "Only in threads of type",
    0x04: "Only in one thread of type",
}
"""Enqueue Server Admin Trace Limit values"""


# Enqueue Server Admin Trace Thread values
enqueue_server_admin_trace_thread_values = {
    0x00: "All threads",
    0x01: "All I/O threads",
    0x02: "Enqueue Worker thread",
    0x03: "Replication thread",
    0x04: "ADM thread",
    0x05: "Signal thread",
    0x06: "Listener thread",
}
"""Enqueue Server Admin Trace Thread values"""


# Enqueue Server Connection Admin Opcode values
enqueue_conn_admin_opcode_values = {
    0x00: "Loopback packet",
    0x01: "Parameter Request",
    0x02: "Parameter Response",
    0x03: "Shutdown Read",
    0x04: "Shutdown Write",
    0x05: "Shutdown Both",
    0x06: "Keepalive",
}
"""Enqueue Server Opcode values"""


# Enqueue Server Connection Admin Parameter values
enqueue_param_values = {
    0x00: "ENCPARAM_RECV_LEN",
    0x01: "ENCPARAM_SEND_LEN",
    0x02: "ENCPARAM_MSG_TYPE",
    0x03: "ENCPARAM_SET_NAME",
    0x04: "ENCPARAM_SET_NOSUPP",
    0x05: "ENCPARAM_SET_VERSION",
    0x06: "ENCPARAM_SET_UCSUPPORT",
}
"""Enqueue Server Connection Admin Parameter values"""


class SAPEnqueueTracePattern(PacketNoPadded):
    """SAP Enqueue Server Admin Trace Pattern
    """
    name = "SAP Enqueue Server Admin Trace Pattern"
    fields_desc = [
        FieldLenField("len", None, length_of="pattern", fmt="B"),
        StrNullFixedLenField("pattern", "", length_from=lambda pkt:pkt.len, max_length=0xff),
    ]


class SAPEnqueueParam(PacketNoPadded):
    """SAP Enqueue Server Connection Admin Parameter packet
    """
    name = "SAP Enqueue Connection Admin Parameter"
    fields_desc = [
        IntEnumField("param", 0, enqueue_param_values),
        ConditionalField(IntField("len", 0), lambda pkt:pkt.param in [0x06]),
        ConditionalField(IntField("value", 0), lambda pkt:pkt.param not in [0x03, 0x04]),
        ConditionalField(StrNullField("set_name", ""), lambda pkt:pkt.param in [0x03]),
    ]


class SAPEnqueue(PacketNoPadded):
    """SAP Enqueue Server packet

    This packet is used for general Enqueue packets.
    """

    name = "SAP Enqueue"
    fields_desc = [
        StrFixedLenField("magic_bytes", "\xab\xcd\xe1\x23", 4),
        IntField("id", 0),
        LenField("len", None, fmt="!I"),
        LenField("len_frag", None, fmt="!I"),
        ByteEnumKeysField("dest", 0x00, enqueue_dest_values),
        ByteEnumKeysField("opcode", 0x00, enqueue_conn_admin_opcode_values),
        ByteField("more_frags", 0),
        ByteEnumKeysField("type", 0x00, enqueue_type_values),

        # Server Admin fields
        ConditionalField(StrNullFixedLenField("adm_eyecatcher1", "ENC", 3), lambda pkt:pkt.dest == 3),
        ConditionalField(ByteField("adm_version", 1), lambda pkt:pkt.dest == 3),
        ConditionalField(ByteField("adm_padd1", 0), lambda pkt:pkt.dest == 3),
        ConditionalField(ByteField("adm_padd2", 0), lambda pkt:pkt.dest == 3),
        ConditionalField(ByteField("adm_padd3", 0), lambda pkt:pkt.dest == 3),
        ConditionalField(StrFixedLenField("adm_eyecatcher2", "#EAA", 4), lambda pkt:pkt.dest == 3),
        ConditionalField(ByteField("adm_1", 1), lambda pkt:pkt.dest == 3),
        ConditionalField(IntField("adm_len", 0), lambda pkt:pkt.dest == 3),
        ConditionalField(ByteEnumKeysField("adm_opcode", 0, enqueue_server_admin_opcode_values), lambda pkt:pkt.dest == 3),
        ConditionalField(ByteField("adm_flags", 0), lambda pkt:pkt.dest == 3),
        ConditionalField(IntField("adm_rc", 0), lambda pkt:pkt.dest == 3),
        ConditionalField(StrFixedLenField("adm_eyecatcher3", "#EAE", 4), lambda pkt:pkt.dest == 3),

        # Server Admin Trace fields
        ConditionalField(ByteField("adm_trace_protocol_version", 1), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(ByteEnumKeysField("adm_trace_action", 3, enqueue_server_admin_trace_action_values), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(ByteEnumKeysField("adm_trace_limit", 0, enqueue_server_admin_trace_limit_values), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(ByteEnumKeysField("adm_trace_thread", 0, enqueue_server_admin_trace_thread_values), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(IntField("adm_trace_unknown1", 0), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(IntField("adm_trace_level", 1), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(IntField("adm_trace_level1", 1), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(ByteField("adm_trace_logging", 0), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(IntField("adm_trace_max_file_size", 20 * 1024 * 1024), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(FieldLenField("adm_trace_nopatterns", 0, count_of="adm_trace_patterns", fmt="!I"), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(FieldLenField("adm_trace_nopatterns1", 0, count_of="adm_trace_patterns", fmt="!I"), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(IntField("adm_trace_unknown3", 37), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(StrFixedLenField("adm_trace_eyecatcher4", "#EAH", 4), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(PacketListField("adm_trace_patterns", None, SAPEnqueueTracePattern, count_from=lambda pkt:pkt.adm_trace_nopatterns), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),
        ConditionalField(StrFixedLenField("adm_trace_eyecatcher5", "#EAD", 4), lambda pkt:pkt.dest == 3 and pkt.adm_opcode in [0x06]),

        # Connection Admin fields
        ConditionalField(FieldLenField("params_count", None, count_of="params", fmt="!I"), lambda pkt:pkt.dest == 6 and pkt.opcode in [1, 2]),
        ConditionalField(PacketListField("params", None, SAPEnqueueParam, count_from=lambda pkt:pkt.params_count), lambda pkt:pkt.dest == 6 and pkt.opcode in [1, 2]),
    ]

    def post_build(self, pkt, pay):
        """Adjust the len and len_frags fields after the build of the whole
        packet. """
        l = struct.pack("!I", len(pkt) + len(pay))
        pkt = pkt[:8] + l + l + pkt[16:]
        return pkt + pay


class SAPEnqueueStreamSocket(SAPNIStreamSocket):
    """Stream socket implementation of the Enqueue Server protocol. It performs
    reassemble of received fragmented packets to ease use in upper layers.
    """
    # TODO: Add support for sending fragmented packets.
    # TODO: Add recv/send max length values as parameters.

    desc = "Enqueue Stream socket"

    def __init__(self, sock, keep_alive=True, base_cls=None):
        """Initialization defaults to SAPEnqueue as base class"""
        if base_cls is None:
            base_cls = SAPEnqueue
        SAPNIStreamSocket.__init__(self, sock, keep_alive=keep_alive, base_cls=base_cls)

    def recv(self):
        """Receive a packet at the Enqueue layer, performing reassemble of
        fragmented packets if necessary.

        @return: received L{SAPEnqueue} packet
        @rtype: L{SAPEnqueue}

        @raise socket.error: if the connection was close
        """
        # Receive the NI packet
        packet = SAPNIStreamSocket.recv(self)

        if SAPEnqueue in packet and packet[SAPEnqueue].more_frags:
            log_sapenqueue.debug("Received Enqueue fragmented packet")

            head = str(packet[SAPEnqueue])[:20]
            data = str(packet[SAPEnqueue])[20:]
            total_length = packet[SAPEnqueue].len - 20
            recvd_length = len(packet[SAPEnqueue]) - 20
            log_sapenqueue.debug("Received %d up to %d bytes", recvd_length, total_length)
            while (recvd_length < total_length and packet[SAPEnqueue].more_frags == 1):
                response = SAPNIStreamSocket.recv(self)[SAPEnqueue]
                data += str(response)[20:]
                recvd_length += len(response) - 20
                log_sapenqueue.debug("Received %d up to %d bytes", recvd_length, total_length)

            packet = SAPEnqueue(head + data)

        return packet


# Bind SAP NI with the SAP Enqueue port
bind_layers(TCP, SAPNI, dport=3200)
