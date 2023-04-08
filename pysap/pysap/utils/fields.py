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
import struct
from datetime import datetime
# External imports
from scapy.config import conf
from scapy.packet import Packet
from scapy.asn1fields import (ASN1F_CHOICE, ASN1F_field, ASN1_Error, ASN1F_badsequence, BER_Decoding_Error)
from scapy.volatile import (RandNum, RandTermString, RandBin)
from scapy.fields import (MultiEnumField, StrLenField, Field, StrFixedLenField, StrField, PacketListField, LongField)


def saptimestamp_to_datetime(timestamp):
    """Converts a timestamp in "SAP format" to a datetime object. Time zone
    looks to be fixed at GMT+1."""
    return datetime.utcfromtimestamp((int(timestamp) & 0xFFFFFFFF) + 1000000000)


class PacketNoPadded(Packet):
    """Regular scapy packet with no padding.
    """
    def extract_padding(self, s):
        return '', s


class RandByteReduced(RandNum):
    """RandByte that only returns random values between 0 and x2a. Used while
    performing some fuzz to reduce the test cases space.

    """
    def __init__(self):
        RandNum.__init__(self, 0, 0x2a)


class ByteMultiEnumKeysField(MultiEnumField):
    """MultiEnumField that picks a reduced number of values. Used for fuzzing
    Byte fields with reduced number of values.

    """
    def randval(self):
        return RandByteReduced()


class MutablePacketField(StrLenField):
    """Packet field that mutates the class according to a list of evaluators.
    The evaluators are run against the packet and given to a class getter.

    If the class can't be found, the field is treated as a StrLenField.
    """
    __slots__ = ["length_from", "evaluators", "_get_class"]

    def __init__(self, name, default, length_from, get_class, evaluators=None):
        """
        :param length_from: function to obtain the field length
        :type length_from: C{callable}

        :param get_class: function to obtain the class
        :type get_class: C{callable}

        :param evaluators: evaluators
        :type evaluators: ``list`` of C{callable}
        """
        StrLenField.__init__(self, name, default, length_from=length_from)
        self.evaluators = evaluators or []
        self._get_class = get_class

    def get_class(self, pkt):
        # Run the evaluators on the actual packet
        values = [evaluator(pkt) for evaluator in self.evaluators]
        # Return the class using the function provided
        return self._get_class(pkt, *values)

    def i2m(self, pkt, i):
        cls = self.get_class(pkt)
        if cls is not None:
            return str(i)
        else:
            return StrLenField.i2m(self, pkt, i)

    def m2i(self, pkt, m):
        cls = self.get_class(pkt)
        if cls is not None:
            return cls(m)
        else:
            return StrLenField.m2i(self, pkt, m)


class StrNullFixedLenField(StrFixedLenField):
    """Packet field that has a fixed length and is conditionally null-terminated.
    """
    __slots__ = ["length_from", "max_length", "null_terminated"]

    def __init__(self, name, default, length=None, length_from=None, max_length=None, null_terminated=None):
        if null_terminated:
            self.null_terminated = null_terminated
        else:
            self.null_terminated = lambda pkt: True
        self.max_length = max_length or 200
        StrFixedLenField.__init__(self, name, default, length=length, length_from=length_from)

    def i2repr(self, pkt, v):
        if self.null_terminated(pkt):
            if type(v) is str:
                v = v.rstrip("\0")
            return repr(v)
        return StrFixedLenField.i2repr(self, pkt, v)

    def getfield(self, pkt, s):
        if self.null_terminated(pkt):
            l = self.length_from(pkt) - 1
            return s[l + 1:], self.m2i(pkt, s[:l])
        return StrFixedLenField.getfield(self, pkt, s)

    def addfield(self, pkt, s, val):
        if self.null_terminated(pkt):
            l = self.length_from(pkt) - 1
            return s + struct.pack("%is" % l, self.i2m(pkt, val)) + "\x00"
        return StrFixedLenField.addfield(self, pkt, s, val)

    def randval(self):
        if self.null_terminated:
            try:
                l = self.length_from(None) - 1
            except:
                l = RandTermString(RandNum(0, self.max_length), "\x00")
            return RandBin(l)
        return StrFixedLenField.randval(self)


class StrFixedLenPaddedField(StrFixedLenField):
    """Packet field that has a fixed length and is padded with a
    given character.
    """
    __slots__ = ["length_from", "padd"]

    def __init__(self, name, default, length=None, length_from=None, padd=" "):
        StrFixedLenField.__init__(self, name, default, length, length_from)
        self.padd = padd

    def getfield(self, pkt, s):
        l = self.length_from(pkt)
        return s[l:], self.m2i(pkt, s[:l])

    def addfield(self, pkt, s, val):
        l = self.length_from(pkt)
        val += self.padd * l
        return StrFixedLenField.addfield(self, pkt, s, val)


class StrNullFixedLenPaddedField(StrFixedLenField):
    """Packet field that has a fixed length and is padded with a
    given character and null terminated.
    """
    __slots__ = ["length_from", "padd"]

    def __init__(self, name, default, length=None, length_from=None, padd=" "):
        StrFixedLenField.__init__(self, name, default, length, length_from)
        self.padd = padd

    def getfield(self, pkt, s):
        l = self.length_from(pkt)
        lz = s.find("\x00")
        if lz < l:
            return s[l + 1:], self.m2i(pkt, s[:lz])
        return s[l + 1:], self.m2i(pkt, s[:l])

    def addfield(self, pkt, s, val):
        l = self.length_from(pkt)
        val += self.padd * l
        return StrFixedLenField.addfield(self, pkt, s, val)


class IntToStrField(Field):
    """Custom field from int to str values, with a variable length
    """
    __slots__ = ["length", "format"]

    def __init__(self, name, default, length=11):
        """Initialize the field with a variable length. The 'machine'
        representation is a string field and the 'internal' repr.
        is a numeric value.
        """
        Field.__init__(self, name, default, "%ds" % length)
        # Stores the length of the field
        self.length = length
        # Stores the conversion format between representations
        self.format = "%" + "%d" % length + "d"

    def m2i(self, pkt, x):
        return str(x)

    def i2m(self, pkt, x):
        return self.format % int(x)

    def i2count(self, pkt, x):
        return x


class StrEncodedPaddedField(StrField):
    __slots__ = ["remain", "encoding", "padd"]

    def __init__(self, name, default, encoding="utf-16", padd="\x0c",
                 fmt="H", remain=0):
        StrField.__init__(self, name, default, fmt, remain)
        self.encoding = encoding
        self.padd = padd

    def h2i(self, pkt, x):
        if x:
            x = x.encode(self.encoding)
        return x

    def i2h(self, pkt, x):
        if x:
            x = x.decode(self.encoding)
        return x

    def addfield(self, pkt, s, val):
        return s + self.i2m(pkt, val) + self.padd

    def getfield(self, pkt, s):
        l = s.find(self.padd)
        if l < 0:
            return "", s
        return s[l + 1:], self.m2i(pkt, s[:l])


class PacketListStopField(PacketListField):
    """Custom field that contains a list of packets until a 'stop' condition is met.
    """
    __slots__ = ["count_from", "length_from", "stop"]

    def __init__(self, name, default, cls, count_from=None, length_from=None, stop=None):
        PacketListField.__init__(self, name, default, cls, count_from=count_from, length_from=length_from)
        self.stop = stop

    def getfield(self, pkt, s):
        c = l = None
        if self.length_from is not None:
            l = self.length_from(pkt)
        elif self.count_from is not None:
            c = self.count_from(pkt)

        lst = []
        ret = ""
        remain = s
        if l is not None:
            remain, ret = s[:l], s[l:]
        while remain:
            if c is not None:
                if c <= 0:
                    break
                c -= 1
            try:
                p = self.m2i(pkt, remain)
            except Exception:
                if conf.debug_dissector:
                    raise
                p = conf.raw_layer(load=remain)
                remain = ""
            else:
                if conf.padding_layer in p:
                    pad = p[conf.padding_layer]
                    remain = pad.load
                    del (pad.underlayer.payload)
                else:
                    remain = ""
            lst.append(p)
            # Evaluate the stop condition
            if self.stop and self.stop(p):
                break
        return remain + ret, lst


class AdjustableFieldLenField(Field):
    __slots__ = ["length_of", "count_of", "adjust"]

    def __init__(self, name, default, length_of=None):
        Field.__init__(self, name, default, ">H")
        self.length_of = length_of

    def i2m(self, pkt, x):
        if x is None:
            fld, fval = pkt.getfield_and_val(self.length_of)
            x = fld.i2len(pkt, fval)
        return x

    def addfield(self, pkt, s, val):
        i2m = self.i2m(pkt, val)
        fmt = "B"
        padd = ""
        if i2m > 0xf0:
            fmt = ">H"
            padd = struct.pack("B", 0xff)
        return s + padd + struct.pack(fmt, i2m)

    def getfield(self, pkt, s):
        if struct.unpack("B", s[:1])[0] == 0xff:
            return s[3:], self.m2i(pkt, struct.unpack(">H", s[1:3])[0])
        else:
            return s[1:], self.m2i(pkt, struct.unpack("B", s[:1])[0])


class ASN1F_CHOICE_SAFE(ASN1F_CHOICE):
    def __init__(self, name, default, *args, **kwargs):
        if "implicit_tag" in kwargs:
            err_msg = "ASN1F_CHOICE has been called with an implicit_tag"
            raise ASN1_Error(err_msg)
        self.implicit_tag = None
        for kwarg in ["context", "explicit_tag"]:
            if kwarg in kwargs:
                setattr(self, kwarg, kwargs[kwarg])
            else:
                setattr(self, kwarg, None)
        ASN1F_field.__init__(self, name, None, context=self.context,
                             explicit_tag=self.explicit_tag)
        self.default = default
        self.current_choice = None
        self.choices = args

    def m2i(self, pkt, s):
        """Try to safely extract an ASN1_Packet from the choices list.

        :raise ASN1_Error: if unable to parse the packet using any of the given choices
        """
        if len(s) == 0:
            raise ASN1_Error("ASN1F_CHOICE: got empty string")

        for choice in self.choices:
            try:
                return self.extract_packet(choice, s)
            except (ASN1_Error, ASN1F_badsequence, BER_Decoding_Error):
                pass
        raise ASN1_Error


class TimestampField(LongField):
    """Timestamp field"""

    def i2h(self, pkt, x):
        dt = datetime.utcfromtimestamp(x)
        return dt.strftime("%Y-%m-%d %H:%M:%S UTC")


class LESignedByteField(Field):
    def __init__(self, name, default):
        Field.__init__(self, name, default, "<b")


class LESignedShortField(Field):
    def __init__(self, name, default):
        Field.__init__(self, name, default, "<h")


class LESignedLongField(Field):
    def __init__(self, name, default):
        Field.__init__(self, name, default, "<q")
