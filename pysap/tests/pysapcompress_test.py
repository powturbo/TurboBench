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
import unittest
# Custom imports
from tests.utils import read_data_file


class PySAPCompressTest(unittest.TestCase):

    test_string_plain = "TEST" * 70
    test_string_compr_lzc = '\x18\x01\x00\x00\x11\x1f\x9d\x8dT\x8aL\xa1\x12p`A\x82\x02\x11\x1aLx\xb0!\xc3\x87\x0b#*\x9c\xe8PbE\x8a\x101Z\xccx\xb1#\xc7\x8f\x1bCj\x1c\xe9QdI\x92 Q\x9aLy\xf2 '
    test_string_compr_lzh = '\x18\x01\x00\x00\x12\x1f\x9d\x02]\x88kpH\xc8(\xc6\xc0\x00\x00'

    def test_import(self):
        """Test import of the pysapcompress library"""
        try:
            import pysapcompress  # @UnusedImport
        except ImportError as e:
            self.Fail(str(e))

    def test_compress_input(self):
        """Test compress function input"""
        from pysapcompress import compress, CompressError
        self.assertRaisesRegexp(CompressError, "invalid input length", compress, "")
        self.assertRaisesRegexp(CompressError, "unknown algorithm", compress, "TestString", algorithm=999)

    def test_decompress_input(self):
        """Test decompress function input"""
        from pysapcompress import decompress, DecompressError
        self.assertRaisesRegexp(DecompressError, "invalid input length", decompress, "", 1)
        self.assertRaisesRegexp(DecompressError, "input not compressed", decompress, "AAAAAAAA", 1)
        self.assertRaisesRegexp(DecompressError, "unknown algorithm", decompress, "\x0f\x00\x00\x00\xff\x1f\x9d\x00\x00\x00\x00", 1)

    def test_lzc(self):
        """Test compression and decompression using LZC algorithm"""
        from pysapcompress import compress, decompress, ALG_LZC
        status, out_length_compressed, out_compressed = compress(self.test_string_plain, ALG_LZC)

        self.assertTrue(status)
        self.assertEqual(out_length_compressed, len(out_compressed))
        self.assertEqual(out_length_compressed, len(self.test_string_compr_lzc))
        self.assertEqual(out_compressed, self.test_string_compr_lzc)

        status, out_length_decompressed, out_decompressed = decompress(out_compressed, len(self.test_string_plain))

        self.assertTrue(status)
        self.assertEqual(out_length_decompressed, len(out_decompressed))
        self.assertEqual(out_length_decompressed, len(self.test_string_plain))
        self.assertEqual(out_decompressed, self.test_string_plain)

    def test_lzh(self):
        """Test compression and decompression using LZH algorithm"""
        from pysapcompress import compress, decompress, ALG_LZH
        status, out_length_compressed, out_compressed = compress(self.test_string_plain, ALG_LZH)

        self.assertTrue(status)
        self.assertEqual(out_length_compressed, len(out_compressed))
        # LZH compression has a random component, so the only check here is
        # in regards to the length and to see if if decompress back to the
        # original string

        status, out_length_decompressed, out_decompressed = decompress(out_compressed, len(self.test_string_plain))

        self.assertTrue(status)
        self.assertEqual(out_length_decompressed, len(out_decompressed))
        self.assertEqual(out_length_decompressed, len(self.test_string_plain))
        self.assertEqual(out_decompressed, self.test_string_plain)

    def test_lzh_decompress(self):
        """Test decompression using LZH algorithm"""
        from pysapcompress import decompress
        status, out_length_decompressed, out_decompressed = decompress(self.test_string_compr_lzh, len(self.test_string_plain))

        self.assertTrue(status)
        self.assertEqual(out_length_decompressed, len(out_decompressed))
        self.assertEqual(out_length_decompressed, len(self.test_string_plain))
        self.assertEqual(out_decompressed, self.test_string_plain)

    def test_login_screen(self):
        """Test (de)compression of a login screen packet. The result is
        compared with data obtained from SAP GUI."""
        from pysapcompress import compress, decompress, ALG_LZH
        login_screen_compressed = read_data_file('nw_703_login_screen_compressed.data')
        login_screen_decompressed = read_data_file('nw_703_login_screen_decompressed.data')

        status, out_length, decompressed = decompress(login_screen_compressed, len(login_screen_decompressed))

        self.assertTrue(status)
        self.assertEqual(out_length, len(login_screen_decompressed))
        self.assertEqual(decompressed, login_screen_decompressed)

        status, out_length, compressed = compress(decompressed, ALG_LZH)

        self.assertTrue(status)
        self.assertEqual(out_length, len(login_screen_compressed))

        # As we can't compare with the compressed data (LZH randomness),
        # decompress again and check with the plain text

        status, out_length, decompressed = decompress(compressed, len(login_screen_decompressed))

        self.assertTrue(status)
        self.assertEqual(out_length, len(login_screen_decompressed))
        self.assertEqual(decompressed, login_screen_decompressed)

    def test_login(self):
        """Test (de)compression of a login packet. The result is
        compared with data obtained from SAP GUI."""
        from pysapcompress import compress, decompress, ALG_LZH
        login_compressed = read_data_file('sapgui_730_login_compressed.data')
        login_decompressed = read_data_file('sapgui_730_login_decompressed.data')

        status, out_length, decompressed = decompress(login_compressed, len(login_decompressed))

        self.assertTrue(status)
        self.assertEqual(out_length, len(login_decompressed))
        self.assertEqual(decompressed, login_decompressed)

        status, out_length, compressed = compress(decompressed, ALG_LZH)

        self.assertTrue(status)
        self.assertEqual(out_length, len(login_compressed))

        # As we can't compare with the compressed data (LZH randomness),
        # decompress again and check with the plain text

        status, out_length, decompressed = decompress(compressed, len(login_decompressed))

        self.assertTrue(status)
        self.assertEqual(out_length, len(login_decompressed))
        self.assertEqual(decompressed, login_decompressed)

    def test_invalid_write(self):
        """Test invalid write vulnerability in LZC code (CVE-2015-2282)"""
        from pysapcompress import decompress, DecompressError

        test_case = read_data_file('invalid_write_testcase.data', False)

        self.assertRaisesRegexp(DecompressError, "stack overflow in decomp", decompress, test_case, 6716)

    def test_invalid_read(self):
        "Test invalid read vulnerability in LZH code (CVE-2015-2278)"
        from pysapcompress import decompress, DecompressError

        test_case = read_data_file('invalid_read_testcase.data', False)

        try:
            decompress(test_case, 661)
        except Exception as e:
            self.assertIsInstance(e, DecompressError)
            self.assertEqual(str(e), "Decompression error (bad hufman tree)")


def suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPCompressTest))
    return suite


if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite())
