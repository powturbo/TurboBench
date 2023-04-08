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
import sys
import unittest
# External imports
from scapy.asn1.asn1 import ASN1_PRINTABLE_STRING, ASN1_OID
from scapy.layers.x509 import X509_RDN, X509_AttributeTypeAndValue
# Custom imports
from tests.utils import data_filename
from pysap.SAPCredv2 import (SAPCredv2, SAPCredv2_Cred_Plain,
                             CIPHER_ALGORITHM_3DES, CIPHER_ALGORITHM_AES256)


class PySAPCredV2Test(unittest.TestCase):

    decrypt_username = "username"
    decrypt_pin = "1234567890"
    cert_name = "CN=PSEOwner"
    common_name = "PSEOwner"
    subject_str = "/CN=PSEOwner"
    subject = [
        X509_RDN(rdn=[
            X509_AttributeTypeAndValue(type=ASN1_OID("2.5.4.3"),
                                       value=ASN1_PRINTABLE_STRING(common_name))
        ])
    ]
    pse_path = "/secudir/pse-v2-noreq-DSA-1024-SHA1.pse"
    pse_path_win = "C:\\secudir\\pse-v2-noreq-DSA-1024-SHA1.pse"

    def validate_credv2_lps_off_fields(self, creds, number, lps_type, cipher_format_version,
                                       cipher_algorithm, cert_name=None, pse_path=None):
        self.assertEqual(len(creds), number)
        cred = creds[0].cred

        self.assertEqual(cred.common_name, cert_name or self.cert_name)
        self.assertEqual(cred.pse_file_path, pse_path or self.pse_path)
        self.assertEqual(cred.lps_type, lps_type)
        self.assertEqual(cred.cipher_format_version, cipher_format_version)
        self.assertEqual(cred.cipher_algorithm, cipher_algorithm)

        self.assertEqual(cred.cert_name, cert_name or self.cert_name)
        self.assertEqual(cred.unknown1, "")
        self.assertEqual(cred.pse_path, pse_path or self.pse_path)
        self.assertEqual(cred.unknown2, "")

    def validate_credv2_plain(self, cred, decrypt_username=None, decrypt_pin=None):
        plain = cred.decrypt(decrypt_username or self.decrypt_username)
        self.assertEqual(plain.pin.val, decrypt_pin or self.decrypt_pin)

    def test_credv2_lps_off_v0_3des(self):
        """Test parsing of a version 0 3DES encrypted credential with LPS off"""
        with open(data_filename("credv2_lps_off_v0_3des"), "rb") as fd:
            s = fd.read()
        creds = SAPCredv2(s).creds
        self.validate_credv2_lps_off_fields(creds, 1, None, 0, CIPHER_ALGORITHM_3DES)

    def test_credv2_lps_off_v0_3des_decrypt(self):
        """Test decryption of a version 0 3DES encrypted credential with LPS off"""

        with open(data_filename("credv2_lps_off_v0_3des"), "rb") as fd:
            s = fd.read()
        cred = SAPCredv2(s).creds[0].cred
        self.validate_credv2_plain(cred)

    def test_credv2_lps_off_v0_dp_3des(self):
        """Test parsing of a version 0 3DES encrypted credential with LPS off using DP (Windows)"""

        with open(data_filename("credv2_lps_off_v0_dp_3des"), "rb") as fd:
            s = fd.read()
        creds = SAPCredv2(s).creds
        self.validate_credv2_lps_off_fields(creds, 1, None, 0, CIPHER_ALGORITHM_3DES,
                                            pse_path=self.pse_path_win)

    def test_credv2_lps_off_v0_dp_3des_decrypt(self):
        """Test decryption of a version 0 3DES encrypted credential with LPS off using DP (Windows)"""

        with open(data_filename("credv2_lps_off_v0_dp_3des"), "rb") as fd:
            s = fd.read()

        cred = SAPCredv2(s).creds[0].cred
        plain = cred.decrypt(self.decrypt_username)
        self.assertEqual(plain.option1, SAPCredv2_Cred_Plain.PROVIDER_MSCryptProtect)

    def test_credv2_lps_off_v1_3des(self):
        """Test parsing of a version 1 3DES encrypted credential with LPS off"""

        with open(data_filename("credv2_lps_off_v1_3des"), "rb") as fd:
            s = fd.read()
        creds = SAPCredv2(s).creds
        self.validate_credv2_lps_off_fields(creds, 1, None, 1, CIPHER_ALGORITHM_3DES)

    def test_credv2_lps_off_v1_3des_decrypt(self):
        """Test decryption of a version 1 3DES encrypted credential with LPS off"""

        with open(data_filename("credv2_lps_off_v1_3des"), "rb") as fd:
            s = fd.read()

        cred = SAPCredv2(s).creds[0].cred
        self.validate_credv2_plain(cred)

    def test_credv2_lps_off_v1_aes256(self):
        """Test parsing of a version 1 AES256 encrypted credential with LPS off"""

        with open(data_filename("credv2_lps_off_v1_aes256"), "rb") as fd:
            s = fd.read()

        creds = SAPCredv2(s).creds
        self.validate_credv2_lps_off_fields(creds, 1, None, 1, CIPHER_ALGORITHM_AES256)

    def test_credv2_lps_off_v1_aes256_decrypt(self):
        """Test decryption of a version 1 AES256 encrypted credential with LPS off"""

        with open(data_filename("credv2_lps_off_v1_aes256"), "rb") as fd:
            s = fd.read()

        cred = SAPCredv2(s).creds[0].cred
        self.validate_credv2_plain(cred)

    def test_credv2_lps_on_v2_int_aes256(self):
        """Test parsing of a version 2 AES256 encrypted credential with LPS on, INT type"""

        with open(data_filename("credv2_lps_on_v2_int_aes256"), "rb") as fd:
            s = fd.read()

        creds = SAPCredv2(s).creds
        self.assertEqual(len(creds), 1)

        cred = creds[0].cred
        self.assertEqual(cred.common_name, self.subject_str)
        self.assertEqual(cred.subject, self.subject)
        self.assertEqual(cred.subject[0].rdn[0].type.val, "2.5.4.3")
        self.assertEqual(cred.subject[0].rdn[0].value.val, self.common_name)

        self.assertEqual(cred.pse_file_path, self.pse_path)
        self.assertEqual(cred.lps_type, 0)
        self.assertEqual(cred.cipher_format_version, 2)
        self.assertEqual(cred.version.val, 2)
        self.assertEqual(cred.pse_path, self.pse_path)

    def test_credv2_lps_on_v2_int_aes256_decrypt(self):
        """Test decryption of a version 2 AES256 encrypted credential with LPS on, INT type"""

        with open(data_filename("credv2_lps_on_v2_int_aes256"), "rb") as fd:
            s = fd.read()

        cred = SAPCredv2(s).creds[0].cred
        self.validate_credv2_plain(cred)

    def test_credv2_lps_on_v2_dp_aes256(self):
        """Test parsing of a version 2 AES256 encrypted credential with LPS on, DP type"""

        with open(data_filename("credv2_lps_on_v2_dp_aes256"), "rb") as fd:
            s = fd.read()

        creds = SAPCredv2(s).creds
        self.assertEqual(len(creds), 1)

        cred = creds[0].cred
        self.assertEqual(cred.common_name, self.subject_str)
        self.assertEqual(cred.subject, self.subject)
        self.assertEqual(cred.subject[0].rdn[0].type.val, "2.5.4.3")
        self.assertEqual(cred.subject[0].rdn[0].value.val, self.common_name)

        self.assertEqual(cred.pse_file_path, self.pse_path_win)
        self.assertEqual(cred.lps_type, 1)
        self.assertEqual(cred.cipher_format_version, 2)
        self.assertEqual(cred.version.val, 2)
        self.assertEqual(cred.pse_path, self.pse_path_win)

    def test_credv2_lps_on_v2_int_aes256_composed_subject(self):
        """Test parsing of a version 2 AES256 encrypted credential with LPS on, INT type,
        and pointing to a PSE with a composed subject
        """

        with open(data_filename("credv2_lps_on_v2_int_aes256_composed_subject"), "rb") as fd:
            s = fd.read()

        c = SAPCredv2(s)
        creds = SAPCredv2(s).creds
        self.assertEqual(len(creds), 1)

        subject_str = "/C=AR/CN=PSEOwner"
        subject = [
            X509_RDN(rdn=[
                X509_AttributeTypeAndValue(type=ASN1_OID("2.5.4.6"),
                                           value=ASN1_PRINTABLE_STRING("AR"))]),
            X509_RDN(rdn=[
                X509_AttributeTypeAndValue(type=ASN1_OID("2.5.4.3"),
                                           value=ASN1_PRINTABLE_STRING(self.common_name))]),
        ]
        cred = creds[0].cred
        self.assertEqual(cred.common_name, subject_str)
        self.assertEqual(cred.subject, subject)
        self.assertEqual(cred.subject[0].rdn[0].type.val, "2.5.4.6")
        self.assertEqual(cred.subject[0].rdn[0].value.val, "AR")
        self.assertEqual(cred.subject[1].rdn[0].type.val, "2.5.4.3")
        self.assertEqual(cred.subject[1].rdn[0].value.val, self.common_name)

        self.assertEqual(cred.lps_type, 0)
        self.assertEqual(cred.cipher_format_version, 2)
        self.assertEqual(cred.version.val, 2)
        self.assertEqual(cred.pse_file_path, self.pse_path)
        self.assertEqual(cred.pse_path, self.pse_path)

        self.validate_credv2_plain(cred)


def test_suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPCredV2Test))
    return suite


if __name__ == "__main__":
    test_runner = unittest.TextTestRunner(verbosity=2, resultclass=unittest.TextTestResult)
    result = test_runner.run(test_suite())
    sys.exit(not result.wasSuccessful())
