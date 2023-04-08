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
# Custom imports
from tests.utils import data_filename
from pysap.SAPPSE import (SAPPSEFile, PKCS12_ALGORITHM_PBE1_SHA_3DES_CBC)


class PySAPPSEv2Test(unittest.TestCase):

    iterations = 10000
    decrypt_pin = "1234567980"
    cert_name = "CN=PSEOwner"
    common_name = "PSEOwner"

    def test_pse_v2_lps_off_pbes1_3des_sha1(self):
        """Test parsing of a v2 PBES1 encrypted PSE with LPS off"""

        with open(data_filename("pse_v2_lps_off_pbes1_3des_sha1.pse"), "rb") as fd:
            s = fd.read()

        pse = SAPPSEFile(s)
        self.assertEqual(pse.version, 2)
        self.assertEqual(pse.enc_cont.algorithm_identifier.alg_id.val, PKCS12_ALGORITHM_PBE1_SHA_3DES_CBC)
        self.assertEqual(pse.enc_cont.algorithm_identifier.parameters.iterations, self.iterations)
        self.assertEqual(len(pse.enc_cont.algorithm_identifier.parameters.salt.val), 8)

    def test_pse_v2_lps_off_pbes1_3des_sha1_decrypt(self):
        """Test decryption of a v2 PBES1 encrypted PSE with LPS off"""

        with open(data_filename("pse_v2_lps_off_pbes1_3des_sha1.pse"), "rb") as fd:
            s = fd.read()

        pse = SAPPSEFile(s)
        self.assertRaisesRegexp(ValueError, "Invalid PIN supplied", pse.decrypt, "Some Invalid PIN")
        pse.decrypt(self.decrypt_pin)

    def test_pse_v4_lps_off_pbes1_3des_sha1(self):
        """Test parsing of a v4 PBES1 encrypted PSE with LPS off"""

        with open(data_filename("pse_v4_lps_off_pbes1_3des_sha1.pse"), "rb") as fd:
            s = fd.read()

        pse = SAPPSEFile(s)
        self.assertEqual(pse.version, 4)
        self.assertEqual(pse.enc_cont.algorithm_identifier.alg_id.val, PKCS12_ALGORITHM_PBE1_SHA_3DES_CBC)
        self.assertEqual(pse.enc_cont.algorithm_identifier.parameters.iterations, self.iterations)
        self.assertEqual(len(pse.enc_cont.algorithm_identifier.parameters.salt.val), 8)


def test_suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPPSEv2Test))
    return suite


if __name__ == "__main__":
    test_runner = unittest.TextTestRunner(verbosity=2, resultclass=unittest.TextTestResult)
    result = test_runner.run(test_suite())
    sys.exit(not result.wasSuccessful())
