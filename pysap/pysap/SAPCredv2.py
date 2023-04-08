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
from binascii import unhexlify
# External imports
from scapy.packet import Packet
from scapy.compat import plain_str
from scapy.asn1packet import ASN1_Packet
from scapy.fields import (ByteField, ByteEnumField, ShortField, StrField, StrFixedLenField)
from scapy.layers.x509 import (X509_RDN, X509_AttributeTypeAndValue,
                               _attrName_mapping, _attrName_specials)
from scapy.asn1.asn1 import (ASN1_IA5_STRING, ASN1_Codecs, ASN1_PRINTABLE_STRING,
                             ASN1_OID)
from scapy.asn1fields import (ASN1F_SEQUENCE, ASN1F_SEQUENCE_OF, ASN1F_BIT_STRING,
                              ASN1F_IA5_STRING, ASN1F_INTEGER, ASN1F_UTF8_STRING,
                              ASN1F_optional)
# Import needed to initialize conf.mib
from scapy.asn1.mib import conf  # noqa: F401

# Custom imports
from pysap.SAPLPS import SAPLPSCipher
from pysap.utils.fields import ASN1F_CHOICE_SAFE
from pysap.utils.crypto import dpapi_decrypt_blob
# External imports
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.hashes import Hash, SHA256
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes


# Create a logger for the Credv2 layer
log_cred = logging.getLogger("pysap.cred")


cred_key_fmt = "240657rsga&/%srwthgrtawe45hhtrtrsr35467b2dx3456j67mv67f89656f75"
"""Fixed key embedded in CommonCryptoLib for encrypted credentials"""


class SAPCredv2_Decryption_Error(Exception):
    pass


class SAPCredv2_Cred_Plain(ASN1_Packet):
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_IA5_STRING("pin", None),
        ASN1F_optional(ASN1F_IA5_STRING("option1", None)),
        ASN1F_optional(ASN1F_IA5_STRING("option2", None)),
        ASN1F_optional(ASN1F_IA5_STRING("option3", None)),
    )

    def decrypt_provider(self, cred):
        """Decrypts a credential file already decrypted using the specified
        provider. This is platform dependent.

        :param cred: credential from where the blob was extracted
        :type cred: SAPCredv2_Cred

        :return: the content in the blob decrypted using the provider
        :rtype: string

        :raise Exception: if the provider is invalid or unsupported
        """
        if self.option1 and self.option1 in self.providers:
            return self.providers[self.option1](self, cred)
        else:
            raise Exception("Invalid or unsupported provider")

    @staticmethod
    def decrypt_MSCryptProtect(plain, cred):
        """Decrypts a credential using the Windows DP API. Requires the current
        logged-in user to have permissions to decrypt the blob stored in the
        credentials file.

        :param plain: plain credential extracted
        :type plain: SAPCredv2_Cred_Plain

        :param cred: credential from where the blob was extracted
        :type cred: SAPCredv2_Cred

        :return: the content in the blob decrypted using the provider
        :rtype: string
        """
        entropy = cred.pse_path
        return dpapi_decrypt_blob(unhexlify(plain.blob.val), entropy)

    PROVIDER_MSCryptProtect = "MSCryptProtect"
    """Provider for Windows hosts using DPAPI"""

    providers = {
        PROVIDER_MSCryptProtect: decrypt_MSCryptProtect,
    }
    """Definition of implemented providers"""


CIPHER_ALGORITHM_3DES = 0
"""Constant for 3DES encryption algorithm"""

CIPHER_ALGORITHM_AES256 = 1
"""Constant for AES256 encryption algorithm"""

cipher_algorithms = {
    CIPHER_ALGORITHM_3DES: "3DES",
    CIPHER_ALGORITHM_AES256: "AES256",
}
"""Dict with encryption algorithms supported"""


class SAPCredv2_Cred_Cipher(Packet):
    """SAP Cred cipher packet. This is the header of an encrypted
    credential format 1. It contains all the required data to decrypt the stored
    credential.

    """
    name = "SAP Cred Cipher Header"

    fields_desc = [
        ByteField("version", 2),
        ByteEnumField("algorithm", 0, cipher_algorithms),
        ShortField("unknown", 0),
        StrFixedLenField("salt", None, 16),
        StrFixedLenField("iv", None, 16),
        StrField("cipher_text", None),
    ]


class SAPCredv2_Cred(ASN1_Packet):
    """SAP Credv2 Credential without LPS definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_IA5_STRING("cert_name", None),
        ASN1F_IA5_STRING("unknown1", None),
        ASN1F_IA5_STRING("pse_path", None),
        ASN1F_IA5_STRING("unknown2", None),
        ASN1F_BIT_STRING("cipher", None),
    )

    @property
    def common_name(self):
        return self.cert_name.val

    @property
    def pse_file_path(self):
        return self.pse_path.val

    @property
    def lps_type(self):
        return None

    @property
    def lps_type_str(self):
        return "OFF"

    @property
    def cipher_format_version(self):
        cipher = self.cipher.val_readable
        if len(cipher) >= 36 and ord(cipher[0]) in [0, 1]:
            return ord(cipher[0])
        return 0

    @property
    def cipher_algorithm(self):
        if self.cipher_format_version == 1:
            return ord(self.cipher.val_readable[1])
        return 0

    def decrypt(self, username):
        """Decrypt a credential given a particular username. Tries to identify the credential
        format and choose the decryption method to use.

        :param username: Username to use when decrypting
        :type username: string

        :return: decrypted object
        :rtype: SAPCredv2_Cred_Plain
        """

        if self.cipher_format_version == 1:
            return self.decrypt_with_header(username)
        else:
            return self.decrypt_simple(username)

    def decrypt_simple(self, username):
        """Decrypt a credential using the simple approach. It only handles 3DES.
        Tries to parse the decrypted object into a plain credential object type. If it fails,
        probably due to an invalid username use to decrypt it, raises an exception.

        :param username: Username to use when decrypting
        :type username: string

        :return: decrypted object
        :rtype: SAPCredv2_Cred_Plain
        """

        blob = self.cipher.val_readable

        # Construct the key using the key format and the username
        key = (cred_key_fmt % username)[:24]
        # Set empty IV
        iv = "\x00" * 8

        # Decrypt the cipher text with the derived key and IV
        decryptor = Cipher(algorithms.TripleDES(key), modes.CBC(iv), backend=default_backend()).decryptor()
        plain = decryptor.update(blob) + decryptor.finalize()

        return SAPCredv2_Cred_Plain(plain)

    def xor(self, string, start):
        """XOR a given string using a fixed key and a starting number."""
        key = 0x15a4e35
        x = start
        y = ""
        for c in string:
            x *= key
            x += 1
            y += chr(ord(c) ^ (x & 0xff))
        return y

    def derive_key(self, key, blob, header, username):
        """Derive a key using SAP's algorithm. The key is derived using SHA256 and xor from an
        initial key, a header, salt and username.
        """
        digest = Hash(SHA256(), backend=default_backend())
        digest.update(key)
        digest.update(blob[0:4])
        digest.update(header.salt)
        digest.update(self.xor(username, ord(header.salt[0])))
        digest.update("" * 0x20)
        hashed = digest.finalize()
        derived_key = self.xor(hashed, ord(header.salt[1]))

        # Validate and select proper algorithm
        if header.algorithm == CIPHER_ALGORITHM_3DES:
            return algorithms.TripleDES, derived_key[:24], header.iv[:8], header.iv[8:] + header.cipher_text
        elif header.algorithm == CIPHER_ALGORITHM_AES256:
            return algorithms.AES, derived_key, header.iv, header.cipher_text
        else:
            raise SAPCredv2_Decryption_Error("Algorithm not supported")

    def decrypt_with_header(self, username):
        """Decrypt a credential file using the header. It handles 3DES and AES256 algorithms.
        Tries to parse the decrypted object into a plain credential object type. If it fails,
        probably due to an invalid username use to decrypt it, raises an exception.

        :param username: Username to use when decrypting
        :type username: string

        :return: decrypted object
        :rtype: SAPCredv2_Cred_Plain

        :raise SAPCredv2_Decryption_Error: if there's an error decrypting the object
        """

        blob = self.cipher.val_readable
        header = SAPCredv2_Cred_Cipher(blob)

        # Validate supported version
        if header.version != 1:
            raise SAPCredv2_Decryption_Error("Version not supported")

        # Derive the key according to SAP's algorithm
        algorithm, key, iv, cipher_text = self.derive_key(cred_key_fmt, blob, header, username)

        # Decrypt the cipher text with the derived key and IV
        decryptor = Cipher(algorithm(key), modes.CBC(iv), backend=default_backend()).decryptor()
        plain = decryptor.update(cipher_text) + decryptor.finalize()

        # Perform a final xor over the decrypted content with a fixed key
        plain = self.xor(plain, 0x64FB914E)

        return SAPCredv2_Cred_Plain(plain)


_default_subject = [
    X509_RDN(),
    X509_RDN(
        rdn=[X509_AttributeTypeAndValue(
            type=ASN1_OID("2.5.4.10"),
            value=ASN1_PRINTABLE_STRING("pysap"))]),
    X509_RDN(
        rdn=[X509_AttributeTypeAndValue(
            type=ASN1_OID("2.5.4.3"),
            value=ASN1_PRINTABLE_STRING("pysap Default Subject"))])
]


class SAPCredv2_Cred_LPS(ASN1_Packet):
    """SAP Credv2 Credential with LPS definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_INTEGER("version", 2),
        ASN1F_SEQUENCE_OF("subject", _default_subject, X509_RDN),
        ASN1F_UTF8_STRING("pse_path", None),
        ASN1F_BIT_STRING("cipher", None),
    )

    def get_subject(self):
        attrs = self.subject
        attrsDict = {}
        for attr in attrs:
            # we assume there is only one name in each rdn ASN1_SET
            attrsDict[attr.rdn[0].type.oidname] = plain_str(attr.rdn[0].value.val)  # noqa: E501
        return attrsDict

    @property
    def common_name(self):
        """This reassembles the issuer construction from Scapy's X.509 Certificate class.
        """
        name_str = ""
        attrsDict = self.get_subject()
        for attrType, attrSymbol in _attrName_mapping:
            if attrType in attrsDict:
                name_str += "/" + attrSymbol + "="
                name_str += attrsDict[attrType]
        for attrType in sorted(attrsDict):
            if attrType not in _attrName_specials:
                name_str += "/" + attrType + "="
                name_str += attrsDict[attrType]
        return name_str

    @property
    def pse_file_path(self):
        return self.pse_path.val

    @property
    def lps_type(self):
        return ord(self.cipher.val_readable[1])

    @property
    def lps_type_str(self):
        if self.lps_type in SAPLPSCipher.lps_types:
            lps = SAPLPSCipher.lps_types[self.lps_type]
        else:
            lps = "OFF"
        return lps

    @property
    def cipher_format_version(self):
        return ord(self.cipher.val_readable[0])

    @property
    def cipher_algorithm(self):
        if self.version == 2:
            return CIPHER_ALGORITHM_AES256
        else:
            return CIPHER_ALGORITHM_3DES

    def decrypt(self, username=None):
        """Decrypt a credential file using LPS.

        :param username: Username to use when decrypting. Not used but kept to match signature
        :type username: string

        :return: decrypted object
        :rtype: SAPCredv2_Cred_Plain
        """

        cipher = SAPLPSCipher(self.cipher.val_readable)
        log_cred.debug("Obtained LPS cipher object (version={}, lps={})".format(cipher.version,
                                                                                cipher.lps_type))
        plain = cipher.decrypt()

        # Get the pin from the raw data
        plain_size = ord(plain[0])
        pin = plain[plain_size + 1:]

        # Create a plain credential container
        plain_cred = SAPCredv2_Cred_Plain()
        plain_cred.pin = ASN1_IA5_STRING(pin)
        return plain_cred


class SAPCredv2Cred(ASN1_Packet):
    """SAP Credv2 Credential definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_CHOICE_SAFE("cred", SAPCredv2_Cred(),
                                  SAPCredv2_Cred,
                                  SAPCredv2_Cred_LPS)


class SAPCredv2(ASN1_Packet):
    """SAP Credv2 Credential set definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE_OF("creds", None, SAPCredv2Cred)
