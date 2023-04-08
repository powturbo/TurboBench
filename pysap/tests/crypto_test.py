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
import unittest
# External imports
from cryptography.hazmat.backends import default_backend
# Custom imports
from pysap.utils.crypto import SCRAM_SHA256, SCRAM_PBKDF2SHA256


class PySAPCryptoUtilsTest(unittest.TestCase):

    def test_scram_sha256_scramble_salt(self):
        """Test SCRAM SHA256 scramble salt calculation.

        Values are taken from https://github.com/SAP/PyHDB/blob/master/tests/test_auth.py
        """

        password = "secret"
        salt = b"\x80\x96\x4f\xa8\x54\x28\xae\x3a\x81\xac" \
               b"\xd3\xe6\x86\xa2\x79\x33"
        server_key = b"\x41\x06\x51\x50\x11\x7e\x45\x5f\xec\x2f\x03\xf6" \
                     b"\xf4\x7c\x19\xd4\x05\xad\xe5\x0d\xd6\x57\x31\xdc" \
                     b"\x0f\xb3\xf7\x95\x4d\xb6\x2c\x8a\xa6\x7a\x7e\x82" \
                     b"\x5e\x13\x00\xbe\xe9\x75\xe7\x45\x18\x23\x8c\x9a"
        client_key = b"\xed\xbd\x7c\xc8\xb2\xf2\x64\x89\xd6\x5a\x7c\xd5" \
                     b"\x1e\x27\xf2\xe7\x3f\xca\x22\x7d\x1a\xb6\xaa\xfc" \
                     b"\xac\x0f\x42\x8c\xa4\xd8\xe1\x0c\x19\xe3\xe3\x8f" \
                     b"\x3a\xac\x51\x07\x5e\x67\xbb\xe5\x2f\xdb\x61\x03" \
                     b"\xa7\xc3\x4c\x8a\x70\x90\x8e\xd5\xbe\x0b\x35\x42" \
                     b"\x70\x5f\x73\x8c"
        expected_scrambled_salt = b"\xe4\x7d\x8f\x24\x48\x55\xb9\x2d\xc9\x66\x39\x5d" \
                       b"\x0d\x28\x25\x47\xb5\x4d\xfd\x09\x61\x4d\x44\x37\x4d\xf9\x4f" \
                       b"\x29\x3c\x1a\x02\x0e"

        scram = SCRAM_SHA256(default_backend())
        scrambled_salt = scram.scramble_salt(password, salt, server_key, client_key)
        self.assertEqual(len(expected_scrambled_salt), len(scrambled_salt))
        self.assertEqual(expected_scrambled_salt, scrambled_salt)

    def test_scram_pbkdf2sha256_scramble_salt(self):
        """Test SCRAM-PBKDF2-SHA256 scramble salt calculation.

        Values are taken from https://github.com/SAP/go-hdb/blob/master/internal/protocol/authentication_test.go
        """

        password = "Toor1234"
        rounds = 15000
        salt = b"3\xb2\xd5\xd5\\R\xc2(Px\xc5[\xa6C\x17?"
        server_key = b" [\xa5\x12\x9eM\x86E\x80\x9dE\xd1/!\xab\xa48\xac\xe5\x00\x99\x03A\x1d\xef\xd2\xba\x86Q \x1d\x89\xef\xa7'\x01\xabuU\x8am&*M+*RF"
        client_key = b'\x89\x9c\xb6<\x9e\x8a]gP\xca6\xbf\xd2N\x8e\xcf\xd2\xb0\x9d\x81\x80\x13\x87\x00\x7f\x1a:\xc5\xbc\xd8y\x1ax\xc4"\x8a\x05\x08: $\xf0\xc7~\xa4p@#.f\xff\xf9~\xfa\x18g\xc6\x98!K\x06\xb3\xbb\xe6'
        expected_scrambled_salt = b'\xfd\xb5e\x00\xd6\xde\x19cb\xfd\x8dj&\xff\x10\x99"J\xd3F\x15[G\xdf\xaa$\xf9|\x01\x87\xb0%'

        scram = SCRAM_PBKDF2SHA256(default_backend())
        scrambled_salt = scram.scramble_salt(password, salt, server_key, client_key, rounds)
        self.assertEqual(len(expected_scrambled_salt), len(scrambled_salt))
        self.assertEqual(expected_scrambled_salt, scrambled_salt)


def test_suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPCryptoUtilsTest))
    return suite


if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(test_suite())
