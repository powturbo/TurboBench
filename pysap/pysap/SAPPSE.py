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
from scapy.asn1packet import ASN1_Packet
from scapy.asn1.ber import BERcodec_SEQUENCE
from scapy.asn1.asn1 import ASN1_Codecs, ASN1_SEQUENCE
from scapy.asn1fields import (ASN1F_SEQUENCE, ASN1F_PACKET, ASN1F_INTEGER, ASN1F_STRING,
                              ASN1F_CHOICE, ASN1F_OID, ASN1F_optional, ASN1F_enum_INTEGER,
                              ASN1F_BIT_STRING, ASN1F_SET_OF, ASN1F_PRINTABLE_STRING,
                              ASN1F_GENERALIZED_TIME, ASN1_Class_UNIVERSAL)
from scapy.layers.x509 import (X509_SubjectPublicKeyInfo, X509_Cert, X509_DirectoryName, X509_Validity,
                               X509_AlgorithmIdentifier)
# Import needed to initialize conf.mib
from scapy.asn1.mib import conf  # noqa: F401

# Custom imports
from pysap.SAPLPS import SAPLPSCipher
from pysap.utils.crypto import PKCS12_PBES1
from pysap.utils.fields import ASN1F_CHOICE_SAFE
# External imports
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.hashes import SHA1
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes


# Create a logger for the PSE layer
log_pse = logging.getLogger("pysap.pse")


class ASN1_Class_PSE(ASN1_Class_UNIVERSAL):
    name = "PSE"
    v2_ENC_CONT = 0xa0
    v4_ENC_CONT = 0xa3


class ASN1_PSE_v2_ENC_CONT_SEQUENCE(ASN1_SEQUENCE):
    tag = ASN1_Class_PSE.v2_ENC_CONT


class ASN1_PSE_v4_ENC_CONT_SEQUENCE(ASN1_SEQUENCE):
    tag = ASN1_Class_PSE.v4_ENC_CONT


class BERcodec_PSE_v2_ENC_CONT_SEQUENCE(BERcodec_SEQUENCE):
    tag = ASN1_Class_PSE.v2_ENC_CONT


class BERcodec_PSE_v4_ENC_CONT_SEQUENCE(BERcodec_SEQUENCE):
    tag = ASN1_Class_PSE.v4_ENC_CONT


class ASN1F_PSE_v2_ENC_CONT_SEQUENCE(ASN1F_SEQUENCE):
    ASN1_tag = ASN1_Class_PSE.v2_ENC_CONT


class ASN1F_PSE_v4_ENC_CONT_SEQUENCE(ASN1F_SEQUENCE):
    ASN1_tag = ASN1_Class_PSE.v4_ENC_CONT


class PKCS12_PBE1_Parameters(ASN1_Packet):
    """PKCS12 PBE1 Parameters"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_STRING("salt", ""),
        ASN1F_INTEGER("iterations", 2048),
    )


PKCS12_ALGORITHM_PBE1_SHA_3DES_CBC = "1.2.840.113549.1.12.1.3"

PKCS5_ALGORITHM_PBES2 = "1.2.840.113549.1.5.13"

NIST_ALGORITHM_AES_256_CBC = "2.16.840.1.101.3.4.1.42"


class PKCS5_Salt_Parameter(ASN1_Packet):
    """PKCS5 Salt Parameter"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_STRING("salt", "")


class PKCS5_Algorithm_Identifier(ASN1_Packet):
    """PKCS5 Algorithm Identifier"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_OID("alg_id", PKCS12_ALGORITHM_PBE1_SHA_3DES_CBC),
        ASN1F_optional(
            ASN1F_CHOICE(
                "parameters", PKCS12_PBE1_Parameters(),
                PKCS12_PBE1_Parameters,
                PKCS5_Salt_Parameter,
            )
        )
    )


sappse_obj_oid = {
    "SignCert": "1.3.36.2.1.1",       # certificate of public verification key
    "EncCert": "1.3.36.2.1.2",        # certificate of public encryption key
    "Cert": "1.3.36.2.1.3",           # certificate of public key
    "SignCSet": "1.3.36.2.2.1",       # cross-certificates of public verification key
    "EncCSet": "1.3.36.2.2.2",        # cross-certificates of public encryption key
    "CSet": "1.3.36.2.2.3",           # cross-certificates of public key
    "CrossCSet": "1.3.36.2.8.1",      # cross certificate pairs
    "SignSK": "1.3.36.2.3.1",         # secret signature key
    "DecSKnew": "1.3.36.2.3.2",       # secret decryption key (current)
    "DecSKold": "1.3.36.2.3.3",       # secret decryption key (old)
    "SKnew": "1.3.36.2.3.4",          # secret key (current)
    "SKold": "1.3.36.2.3.5",          # secret key (old)
    "FCPath": "1.3.36.2.4.1",         # forward certification path
    "PKRoot": "1.3.36.2.5.1",         # top level public verification key
    "PKList": "1.3.36.2.6.1",         # list of trusted public verification keys
    "EKList": "1.3.36.2.6.2",         # list of trusted public encryption keys
    "PCAList": "1.3.36.2.6.3",        # list of recognized PCAs
    "CrlSet": "1.3.36.2.9.1",         # list of revocation lists
    "SerialNumber": "1.3.36.2.10.1",  # current serial number (CA only)
    "EDBKey": "1.3.36.2.11.1",        # DSA database encryption key
    "AliasList": "1.3.36.2.12.1",     # user's alias list
    "QuipuPWD": "1.3.36.2.13.1",      # password for X.500 directory access
}


class SAPPSE_Root_Key(ASN1_Packet):
    """SAP PSEv2 Root Key definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_enum_INTEGER("version", 0x0, ["v0"], explicit_tag=0xa0),
        ASN1F_INTEGER("serial_number", 0),
        ASN1F_PACKET("public_key", X509_SubjectPublicKeyInfo(), X509_SubjectPublicKeyInfo),
        ASN1F_PACKET("validity", X509_Validity(), X509_Validity, explicit_tag=0xa1),
        ASN1F_PACKET("sign_alg_id", X509_AlgorithmIdentifier(), X509_AlgorithmIdentifier, explicit_tag=0xa2),
        ASN1F_BIT_STRING("sign_bit_string", ""),
    )


class SAPPSE_Obj_PKRoot(ASN1_Packet):
    """SAP PSEv2 PKRoot Object definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_PACKET("ca", X509_DirectoryName(), X509_DirectoryName),
        ASN1F_PACKET("new_key", SAPPSE_Root_Key(), SAPPSE_Root_Key),
        ASN1F_PACKET("old_key", SAPPSE_Root_Key(), SAPPSE_Root_Key, explicit_tag=0xa0),
    )


class SAPPSE_Obj_PKList(ASN1_Packet):
    """SAP PSEv2 PKList, EKList, PCAList Object definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_enum_INTEGER("version", 0x0, ["v0"], explicit_tag=0xa0),
        ASN1F_INTEGER("serial_number", 0),
        ASN1F_PACKET("signature", X509_AlgorithmIdentifier(), X509_AlgorithmIdentifier),
        ASN1F_PACKET("issuer", X509_DirectoryName(), X509_DirectoryName),
        ASN1F_PACKET("validity", X509_Validity(), X509_Validity),
        ASN1F_PACKET("partner", X509_DirectoryName(), X509_DirectoryName),
        ASN1F_PACKET("verification_key", X509_SubjectPublicKeyInfo(), X509_SubjectPublicKeyInfo),
    )


class SAPPSE_Obj_CertList(ASN1_Packet):
    """SAP PSEv2 CertList, CSet, SignCSet, EncCSet Object definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SET_OF("certs", None, X509_Cert)


class SAPPSE_Obj(ASN1_Packet):
    """SAP PSEv2 Object definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_PRINTABLE_STRING("object_name", "PKRoot"),
        ASN1F_GENERALIZED_TIME("created", None),
        ASN1F_OID("object_type", sappse_obj_oid["PKRoot"]),
        ASN1F_CHOICE_SAFE("object_value", None,
                          X509_SubjectPublicKeyInfo,               # SKnew, SKold, DECSKnew, DECSKold, SignSK
                          X509_Cert,                               # Cert, SignCert, EncCert
                          SAPPSE_Obj_PKRoot,                       # PKRoot
                          SAPPSE_Obj_CertList,                     # CertList, CSet, SignCSet, EncCSet
                          # ASN1F_SET_OF("cert_pairs", None, X509_CertPair),       # CrossCSet
                          # ASN1F_SEQUENCE_OF("forward_certification_path", None,  # FCPath
                          #                   ASN1F_SET_OF("cross_certs", None,
                          #                                X509_Cert)),
                          # ASN1F_SET_OF("pklist", SAPPSE_Obj_PKList(), SAPPSE_Obj_PKList),  # PKList, EKList, PCAList
                          # ASN1F_SET_OF("crlset", SAPPSE_Obj_CRLSet(), SAPPSE_Obj_CRLSet),  # CRLSet
                          # ASN1F_STRING("serial_number"),           # SerialNumber
                          # ASN1F_STRING("quipu_password"),          # QuipuPWD
                          # SAPPSE_Obj_EDBKey,                       # EDBKey
                          )
    )


class SAPPSE_Cont(ASN1_Packet):
    """SAP PSEv2 Content definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_PACKET("algorithm_identifier", PKCS5_Algorithm_Identifier(),
                     PKCS5_Algorithm_Identifier),
        ASN1F_GENERALIZED_TIME("timestamp", None),
        ASN1F_INTEGER("unknown1", 1),
        ASN1F_SET_OF("pse_obj", SAPPSE_Obj(), SAPPSE_Obj),
    )


class SAPPSEv2_Enc_Cont(ASN1_Packet):
    """SAP PSEv2 Encrypted content definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_PSE_v2_ENC_CONT_SEQUENCE(
        ASN1F_STRING("encrypted_pin", ""),
        ASN1F_PACKET("algorithm_identifier", PKCS5_Algorithm_Identifier(),
                     PKCS5_Algorithm_Identifier),
        ASN1F_STRING("cipher_text", ""),
    )


class SAPPSEv4_Enc_Cont(ASN1_Packet):
    """SAP PSEv4 Encrypted content definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_PSE_v4_ENC_CONT_SEQUENCE(
        ASN1F_INTEGER("unknown", 1),
        ASN1F_PACKET("algorithm_identifier", PKCS5_Algorithm_Identifier(),
                     PKCS5_Algorithm_Identifier),
        ASN1F_STRING("cipher_text", ""),
        ASN1F_STRING("encrypted_pin", ""),
    )


class SAPPSEFile(ASN1_Packet):
    """SAP PSE definition"""
    ASN1_codec = ASN1_Codecs.BER
    ASN1_root = ASN1F_SEQUENCE(
        ASN1F_INTEGER("version", 2),
        ASN1F_CHOICE("enc_cont", SAPPSEv2_Enc_Cont(),
                     SAPPSEv2_Enc_Cont,
                     SAPPSEv4_Enc_Cont,
                     )
    )

    def decrypt(self, pin):
        """Decrypts a PSE file given a provided PIN. Calls the respective decryption function
        based on the PSE version.
        """

        if self.version == 2:
            return self.decrypt_non_lps(pin)
        elif self.version == 256:
            return self.decrypt_lps(pin)
        else:
            raise ValueError("Unsupported or invalid PSE version")

    def decrypt_lps(self, pin):
        """Decrypts an LPS encrypted PSE file given a provided PIN.

        :param pin:
        :type pin: string

        :return: decrypted object
        :rtype: SAPPSE_Cont

        :raise ValueError: if the provided PIN doesn't match with the one used for encryption
        :raise NotImplementedError: if the algorithm specified is not supported
        :raise Exception: if the algorithm specified is not valid
        """

        # Decrypt the encryption key using the LPS method
        cipher = SAPLPSCipher(self.enc_cont.encrypted_pin.val)
        log_pse.debug("Obtained LPS cipher object (version={}, lps={})".format(cipher.version,
                                                                               cipher.lps_type))
        key = cipher.decrypt()

        # Choose the proper algorithms and values according to the algorithm ID
        if self.enc_cont.algorithm_identifier.alg_id == NIST_ALGORITHM_AES_256_CBC:
            algorithm = algorithms.AES
            mode = modes.CBC
            key, iv = key[:32], key[32:]
        else:
            raise Exception("Invalid PBE algorithm")

        # Decrypt the cipher text with the derived key and IV
        decryptor = Cipher(algorithm(key), mode(iv), backend=default_backend()).decryptor()
        plain = decryptor.update(self.enc_cont.cipher_text.val) + decryptor.finalize()

        return plain

    def decrypt_non_lps(self, pin):
        """Decrypts a non-LPS encrypted PSE file given a provided PIN. Implements PKCS12 PBE1
        based encryption for v2 PSE files.

        :param pin:
        :type pin: string

        :return: decrypted object
        :rtype: SAPPSE_Cont

        :raise ValueError: if the provided PIN doesn't match with the one used for encryption
        :raise NotImplementedError: if the algorithm specified is not supported
        :raise Exception: if the algorithm specified is not valid
        """

        cipher_text = self.enc_cont.cipher_text.val

        # Choose the proper algorithms and values according to the algorithm ID
        if self.enc_cont.algorithm_identifier.alg_id == PKCS12_ALGORITHM_PBE1_SHA_3DES_CBC:
            salt = self.enc_cont.algorithm_identifier.parameters.salt.val
            iterations = self.enc_cont.algorithm_identifier.parameters.iterations.val
            hash_algorithm = SHA1
            enc_algorithm = algorithms.TripleDES
            enc_mode = modes.CBC
            iv = None
            pbes_cls = PKCS12_PBES1
        elif self.enc_cont.algorithm_identifier.alg_id == PKCS5_ALGORITHM_PBES2:
            raise NotImplementedError("PBE algorithm not implemented")
        else:
            raise Exception("Invalid PBE algorithm")

        # Build the PBE class
        pbes = pbes_cls(salt, iterations, iv, pin, hash_algorithm, enc_algorithm, enc_mode, default_backend())

        # On version 2, we can check that the PIN was valid before decrypting the whole
        # cipher text
        if self.version == 2:
            encrypted_pin = pbes.encrypt(pin)
            if encrypted_pin != self.enc_cont.encrypted_pin.val:
                raise ValueError("Invalid PIN supplied")

        # Decrypt and parse the cipher text
        plain_text = pbes.decrypt(cipher_text)

        return plain_text
