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
from scapy.fields import (ByteField, ByteEnumField, ShortField, StrFixedLenField,
                          StrLenField, LEIntField)
# Custom imports
from pysap.utils.crypto import dpapi_decrypt_blob
# Optional imports
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.hashes import Hash, SHA1
from cryptography.hazmat.primitives.hmac import HMAC


# Create a logger for the LPS layer
log_lps = logging.getLogger("pysap.lps")


cred_key_lps_fallback = "\xe7\x6a\xd2\xce\x4b\xa7\xc7\x9e\xf9\x79\x5f\xa8\x2e\x6e\xaa\x1d\x76\x02\x2e\xcd\xd7\x74\x38\x51"
"""Fixed key embedded in CommonCryptoLib for encrypted credentials using LPS in fallback mode"""


class SAPLPSDecryptionError(Exception):
    pass


class SAPLPSCipher(Packet):
    """SAP LPS cipher packet. This is the data stored inside an LPS encrypted blob
    (credential or PSE file). It contains all the required data to decrypt and validate
    the stored context.

    Context length should be <28h
    Restriction length should be <400h
    """
    name = "SAP LPS Cipher"

    LPS_FALLBACK = 0
    LPS_DPAPI = 1
    LPS_TPM = 2

    lps_types = {
        LPS_FALLBACK: "FALLBACK",
        LPS_DPAPI: "DPAPI",
        LPS_TPM: "TPM",
    }
    """LPS types"""

    fields_desc = [
        ByteField("version", 2),
        ByteEnumField("lps_type", 0, lps_types),
        ShortField("context_length", 0),
        StrLenField("context", None, length_from=lambda x: x.context_length),
        ShortField("restriction_length", 0),
        StrLenField("restriction", None, length_from=lambda x: x.restriction_length),
        ShortField("encrypted_key_length", 0),
        StrLenField("encrypted_key", None, length_from=lambda x: x.encrypted_key_length),
        ShortField("unknown_length", 0),
        StrLenField("unknown", None, length_from=lambda x: x.unknown_length),
        ShortField("encrypted_data_length", 0),
        StrLenField("encrypted_data", None, length_from=lambda x: x.encrypted_data_length),
        StrFixedLenField("hmac", None, 0x14),
        LEIntField("checksum", None),
    ]

    def decrypt(self):
        """Decrypt a blob using LPS. Implements FALLBACK and DP methods to derive
        or obtain the encryption key from the one stored in the PSE/credential file and then uses
        that encryption key to decrypt the credential using the AES cipher.

        LPS-protected PSEs/credentials are verified with both a CRC32 checksum and an HMAC.
        Validation of the checksum and HMAC is not implemented.

        :return: decrypted object
        :rtype: string

        :raise NotImplementedError: if the LPS method is not implemented
        :raise SAP_LPS_Decryption_Error: if there's an error decrypting the object
        """

        # Validate supported version
        if self.version != 2:
            log_lps.error("Version not supported")
            raise SAPLPSDecryptionError("Version not supported")

        # TODO: Calculate and validate CRC32

        # Decrypt the encryption key using the LPS method
        if self.lps_type in lps_encryption_key_decryptor:
            encryption_key = lps_encryption_key_decryptor[self.lps_type](self)
        else:
            log_lps.error("Invalid LPS decryption method")
            raise SAPLPSDecryptionError("Invalid LPS decryption method")

        # Decrypt the cipher text with the encryption key
        iv = "\x00" * 16
        decryptor = Cipher(algorithms.AES(encryption_key), modes.CBC(iv), backend=default_backend()).decryptor()
        plain = decryptor.update(self.encrypted_data) + decryptor.finalize()

        # TODO: Calculate and validate HMAC

        return plain

    def decrypt_encryption_key_dpapi(self):
        """Decrypts the encryption key using the DP API. The key is encrypted using
        the DP API without any additional entropy.

        :return: Encryption key decrypted
        :rtype: string
        """
        log_lps.debug("Obtaining encryption key with DPAPI LPS mode")

        return dpapi_decrypt_blob(self.encrypted_key)

    def decrypt_encryption_key_fallback(self):
        """Decrypts the encryption key using the FALLBACK method. In this method, the
        context string, usually "CredEncryption" or "PSEEncryption", is encrypted using
        a derivation of a fixed key hardcoded in CommonCryptoLib, and used as key to
        encrypt the actual encryption key used in the file with the AES cipher.

        :return: Encryption key decrypted
        :rtype: string
        """
        log_lps.debug("Obtaining encryption key with FALLBACK LPS mode")

        digest = Hash(SHA1(), backend=default_backend())
        digest.update(cred_key_lps_fallback)
        hashed_key = digest.finalize()

        hmac = HMAC(hashed_key, SHA1(), backend=default_backend())
        hmac.update(self.context)
        default_key = hmac.finalize()[:16]

        iv = "\x00" * 16
        decryptor = Cipher(algorithms.AES(default_key), modes.CBC(iv), backend=default_backend()).decryptor()
        encryption_key = decryptor.update(self.encrypted_key) + decryptor.finalize()

        return encryption_key

    def decrypt_encryption_key_tpm(self):
        """Decrypts the encryption key using the TPM method.

        :return: Encryption key decrypted
        :rtype: string
        """
        log_lps.error("LPS TPM decryption not implemented")
        raise NotImplementedError("LPS TPM decryption not implemented")


lps_encryption_key_decryptor = {
    SAPLPSCipher.LPS_FALLBACK: SAPLPSCipher.decrypt_encryption_key_fallback,
    SAPLPSCipher.LPS_DPAPI: SAPLPSCipher.decrypt_encryption_key_dpapi,
    SAPLPSCipher.LPS_TPM: SAPLPSCipher.decrypt_encryption_key_tpm,
}
"""LPS encryption key decryptor functions"""
