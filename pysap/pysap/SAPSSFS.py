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
# External imports
from scapy.packet import Packet
from scapy.fields import (ByteField, YesNoByteField, LenField, StrFixedLenField, StrField, PacketListField)
from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.hmac import HMAC
from cryptography.hazmat.primitives.hashes import Hash, SHA1
from cryptography.hazmat.backends import default_backend
# Custom imports
from pysap.utils.crypto import rsec_decrypt
from pysap.utils.fields import PacketNoPadded, StrFixedLenPaddedField, TimestampField


# Create a logger for the SSFS layer
log_ssfs = logging.getLogger("pysap.ssfs")


ssfs_hmac_key_unobscured = "\xe3\xa0\x61\x11\x85\x41\x68\x99\xf3\x0e\xda\x87\x7a\x80\xcc\x69"
"""Fixed key embedded in rsecssfx binaries for validating integrity of records"""


class SAPSSFSLKY(Packet):
    """SAP SSFS LKY file format packet.

    """
    name = "SAP SSFS LKY"
    fields_desc = [
        StrFixedLenField("preamble", "RSecSSFsLKY", 11),
    ]


class SAPSSFSLock(Packet):
    """SAP SSFS Lock file format packet.

    """
    name = "SAP SSFS Lock"
    fields_desc = [
        StrFixedLenField("preamble", "RSecSSFsLock", 12),
        ByteField("file_type", 0),
        ByteField("type", 0),
        TimestampField("timestamp", None),
        StrFixedLenPaddedField("user", None, 24, padd=" "),
        StrFixedLenPaddedField("host", None, 24, padd=" "),
    ]


class SAPSSFSKey(Packet):
    """SAP SSFS Key file format packet.

    Key file length is 0x5c
    """
    name = "SAP SSFS Key"
    fields_desc = [
        StrFixedLenField("preamble", "RSecSSFsKey", 11),
        ByteField("type", 1),
        StrFixedLenField("key", None, 24),
        TimestampField("timestamp", None),
        StrFixedLenPaddedField("user", None, 24, padd=" "),
        StrFixedLenPaddedField("host", None, 24, padd=" "),
    ]


class SAPSSFSDecryptedPayload(PacketNoPadded):
    """SAP SSFS Decrypted Payload.

    This represents a payload after decryption.
    """
    name = "SAP SSFS Decrypted Payload"

    fields_desc = [
        # Record Header
        StrFixedLenField("preamble", "\x00"*8, 8),
        LenField("length", 0, fmt="I"),  # Max record length supported is 0x18150
        StrFixedLenField("hash", None, 20),
        # Data Header
        StrFixedLenField("data", None, length_from=lambda pkt: pkt.length),
        StrField("padd", None),
    ]

    @property
    def valid(self):
        """Returns whether the SHA1 value is valid for the given payload"""
        blob = str(self)

        digest = Hash(SHA1(), backend=default_backend())
        digest.update(blob[:8])
        digest.update(blob[8:8+4])
        if self.length:
            digest.update(blob[0x20:0x20 + self.length])
        if len(blob) > self.length + 0x20:
            digest.update(blob[0x20 + self.length:])
        blob_hash = digest.finalize()
        return blob_hash == self.hash


class SAPSSFSDataRecord(PacketNoPadded):
    """SAP SSFS Data Record.

    The Data Record is comprised of a record header of 24 bytes and a data header of 152 bytes followed by the
    actual data.
    """
    name = "SAP SSFS Data Record"

    fields_desc = [
        # Record Header
        StrFixedLenField("preamble", "RSecSSFsData", 12),
        LenField("length", 0, fmt="I"),  # Max record length supported is 0x18150
        ByteField("type", 1),   # Record type "1" supported
        StrFixedLenField("filler1", None, 7),
        # Data Header
        StrFixedLenPaddedField("key_name", None, 64, padd=" "),
        TimestampField("timestamp", None),
        StrFixedLenPaddedField("user", None, 24, padd=" "),
        StrFixedLenPaddedField("host", None, 24, padd=" "),
        YesNoByteField("is_deleted", 0),
        YesNoByteField("is_stored_as_plaintext", 0),
        YesNoByteField("is_binary_data", 0),
        StrFixedLenField("filler2", None, 9),
        StrFixedLenField("hmac", None, 20),  # HMAC-SHA1 of the data header and payload
        # Data
        StrFixedLenField("data", None, length_from=lambda pkt: pkt.length - 176),
    ]

    def get_plain_data(self, key=None):
        if self.is_stored_as_plaintext:
            return self.data
        if not key:
            raise KeyError("Key need to be provided to get the plain data")
        return self.decrypt_data(key)

    def decrypt_data(self, key):
        log_ssfs.debug("Decrypting record {}".format(self.key_name))
        decrypted_data = rsec_decrypt(self.data, key.key)
        decrypted_payload = SAPSSFSDecryptedPayload(decrypted_data)
        log_ssfs.warn("Decrypted payload integrity is {}".format(decrypted_payload.valid))
        return decrypted_payload.data

    @property
    def valid(self):
        """Returns whether the HMAC-SHA1 value is valid for the given payload"""

        # Calculate the HMAC-SHA1
        h = HMAC(ssfs_hmac_key_unobscured, SHA1(), backend=default_backend())
        h.update(str(self)[24:156])  # Entire Data header without the HMAC field
        h.update(self.data)

        # Validate the signature
        try:
            h.verify(self.hmac)
            return True
        except InvalidSignature:
            return False

    @property
    def deleted(self):
        """Returns whether the HMAC-SHA1 value has been deleted"""
        return self.is_deleted == 1


class SAPSSFSData(Packet):
    """SAP SSFS Data file format packet.

    """
    name = "SAP SSFS Data File"

    fields_desc = [
        PacketListField("records", None, SAPSSFSDataRecord),
    ]

    def has_record(self, key_name):
        """Returns if the data file contains a record with a given key name.

        :param key_name: the name of the key to look for
        :type key_name: string

        :return: if the data file contains the record with key_name
        :rtype: bool
        """
        for record in self.records:
            if record.key_name.rstrip(" ") == key_name:
                return True
        return False

    def get_records(self, key_name):
        """Generator to retrieve records with the given key name.

        :param key_name: the name of the key to look for
        :type key_name: string

        :return: the record with key_name
        :rtype: SAPSSFSDataRecord
        """
        for record in self.records:
            if record.key_name.rstrip(" ") == key_name:
                yield record

    def get_record(self, key_name):
        """Returns the first record with the given key name.

        :param key_name: the name of the key to look for
        :type key_name: string

        :return: the record with key_name
        :rtype: SAPSSFSDataRecord
        """
        return next(self.get_records(key_name), None)

    def get_value(self, key_name, key=None):
        """Returns the value with the given key name.

        :param key_name: the name of the key to look for
        :type key_name: string

        :param key: the encryption key
        :type key: SAPSSFSKey

        :return: the value with key_name
        :rtype: bytes
        """
        try:
            return self.get_record(key_name).get_plain_data(key)
        except AttributeError:
            return None
