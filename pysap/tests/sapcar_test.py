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
from os import unlink, rmdir
from os.path import basename, exists
# External imports
# Custom imports
from tests.utils import data_filename
from pysap.SAPCAR import (SAPCARArchive, SAPCARArchiveFile, SAPCARArchiveFilev200Format, SAPCARArchiveFilev201Format,
                          SAPCAR_VERSION_200, SAPCAR_VERSION_201, SIZE_FOUR_GB)


class PySAPCARTest(unittest.TestCase):

    test_archive_file = "somefile"
    test_filename = "test_string.txt"
    test_timestamp_raw = 1449010128
    test_timestamp = "01 Dec 2015 22:48"
    test_perm_mode = 33204
    test_permissions = "-rw-rw-r--"
    test_string = "The quick brown fox jumps over the lazy dog"

    def setUp(self):
        with open(self.test_filename, "wb") as fd:
            fd.write(self.test_string)

    def tearDown(self):
        for filename in [self.test_filename, self.test_archive_file]:
            if exists(filename):
                unlink(filename)
        if exists("test"):
            rmdir("test")

    def check_sapcar_archive(self, filename, version):
        """Test SAP CAR archive file"""

        with open(data_filename(filename), "rb") as fd:
            sapcar_archive = SAPCARArchive(fd, mode="r")

            self.assertEqual(filename, basename(sapcar_archive.filename))
            self.assertEqual(version, sapcar_archive.version)
            self.assertEqual(1, len(sapcar_archive.files))
            self.assertEqual(1, len(sapcar_archive.files_names))
            self.assertListEqual([self.test_filename], sapcar_archive.files_names)
            self.assertListEqual([self.test_filename], sapcar_archive.files.keys())

            af = sapcar_archive.open(self.test_filename)
            self.assertEqual(self.test_string, af.read())
            af.close()

            ff = sapcar_archive.files[self.test_filename]
            self.assertEqual(len(self.test_string), ff.size)
            self.assertEqual(self.test_filename, ff.filename)
            self.assertEqual(self.test_timestamp, ff.timestamp)
            self.assertEqual(self.test_timestamp_raw, ff.timestamp_raw)
            self.assertEqual(self.test_permissions, ff.permissions)
            self.assertEqual(self.test_perm_mode, ff.perm_mode)
            self.assertEqual(version, ff.version)
            self.assertTrue(ff.is_file())
            self.assertFalse(ff.is_directory())

            self.assertTrue(ff.check_checksum())
            self.assertEqual(ff.calculate_checksum(self.test_string), ff.checksum)

            af = ff.open()
            self.assertEqual(self.test_string, af.read())
            af.close()

    def test_sapcar_archive(self):
        """Test some basic construction of a SAP CAR archive"""

        self.assertRaises(ValueError, SAPCARArchive, self.test_archive_file, "w", version="2.02")
        self.assertRaises(ValueError, SAPCARArchive, self.test_archive_file, "x")

    def test_sapcar_archive_200(self):
        """Test SAP CAR archive file version 2.00"""

        self.check_sapcar_archive("car200_test_string.sar", SAPCAR_VERSION_200)

    def test_sapcar_archive_201(self):
        """Test SAP CAR archive file version 2.01"""

        self.check_sapcar_archive("car201_test_string.sar", SAPCAR_VERSION_201)

    def test_sapcar_archive_add_file(self):
        """Test some basic construction of a SAP CAR archive adding from an existent file"""

        ar = SAPCARArchive(self.test_archive_file, "w")
        ar.add_file(self.test_filename)
        ar.add_file(self.test_filename, archive_filename=self.test_filename+"two")

        self.assertEqual("2.01", ar.version)
        self.assertEqual(2, len(ar.files))
        self.assertEqual(2, len(ar.files_names))
        self.assertListEqual([self.test_filename, self.test_filename+"two"], ar.files_names)
        self.assertListEqual([self.test_filename, self.test_filename+"two"], ar.files.keys())

        for filename in [self.test_filename, self.test_filename+"two"]:
            af = ar.open(filename)
            self.assertEqual(self.test_string, af.read())
            af.close()

            ff = ar.files[filename]
            self.assertEqual(len(self.test_string), ff.size)
            self.assertEqual(filename, ff.filename)

            self.assertTrue(ff.check_checksum())
            self.assertEqual(ff.calculate_checksum(self.test_string), ff.checksum)

            af = ff.open()
            self.assertEqual(self.test_string, af.read())
            af.close()

        ar.write()
        ar.close()

    def test_sapcar_archive_file_from_file(self):
        """Test SAP CAR archive file object construction from file using the original name
        and a different one"""
        ff = SAPCARArchiveFile.from_file(self.test_filename)
        self.assertEqual(len(self.test_string), ff.size)
        self.assertEqual(self.test_filename, ff.filename)
        self.assertTrue(ff.check_checksum())

        af = ff.open()
        self.assertEqual(self.test_string, af.read())
        af.close()

        test_filename_new = "some_other_filename.txt"
        ff = SAPCARArchiveFile.from_file(self.test_filename, archive_filename=test_filename_new)
        self.assertEqual(len(self.test_string), ff.size)
        self.assertEqual(test_filename_new, ff.filename)
        self.assertTrue(ff.check_checksum())

        af = ff.open()
        self.assertEqual(self.test_string, af.read())
        af.close()

    def test_sapcar_archive_file_200_to_201(self):
        """Test SAP CAR archive file object conversion from 2.00 to 2.01"""

        with open(data_filename("car200_test_string.sar"), "rb") as fd200:
            ar200 = SAPCARArchive(fd200, mode="r")
            ff200 = ar200.files[self.test_filename]
            ff201 = SAPCARArchiveFile.from_archive_file(ff200, SAPCAR_VERSION_201)

            self.assertEqual(ff200.size, ff201.size)
            self.assertEqual(ff200.filename, ff201.filename)
            self.assertEqual(ff200.timestamp, ff201.timestamp)
            self.assertEqual(ff200.timestamp_raw, ff201.timestamp_raw)
            self.assertEqual(ff200.permissions, ff201.permissions)
            self.assertEqual(ff200.perm_mode, ff201.perm_mode)
            self.assertEqual(ff200.checksum, ff201.checksum)
            self.assertIs(ff200.is_file(), ff201.is_file())
            self.assertIs(ff200.is_directory(), ff201.is_directory())

            af = ff201.open()
            self.assertEqual(self.test_string, af.read())
            af.close()

    def test_sapcar_archive_200_to_201(self):
        """Test SAP CAR archive object conversion from 2.00 to 2.01"""

        with open(data_filename("car200_test_string.sar"), "rb") as fd:
            ar = SAPCARArchive(fd, mode="r")
            ff200 = ar.files[self.test_filename]

            ar.version = SAPCAR_VERSION_201
            ff201 = ar.files[self.test_filename]

            self.assertEqual(ff200.size, ff201.size)
            self.assertEqual(ff200.filename, ff201.filename)
            self.assertEqual(ff200.timestamp, ff201.timestamp)
            self.assertEqual(ff200.timestamp_raw, ff201.timestamp_raw)
            self.assertEqual(ff200.permissions, ff201.permissions)
            self.assertEqual(ff200.perm_mode, ff201.perm_mode)
            self.assertEqual(ff200.checksum, ff201.checksum)
            self.assertIs(ff200.is_file(), ff201.is_file())
            self.assertIs(ff200.is_directory(), ff201.is_directory())

            af = ff201.open()
            self.assertEqual(self.test_string, af.read())
            af.close()

    def test_sapcar_archive_201_to_200(self):
        """Test SAP CAR archive object conversion from 2.01 to 2.00"""

        with open(data_filename("car201_test_string.sar"), "rb") as fd:
            ar = SAPCARArchive(fd, mode="r")
            ff201 = ar.files[self.test_filename]

            ar.version = SAPCAR_VERSION_200
            ff200 = ar.files[self.test_filename]

            self.assertEqual(ff200.size, ff201.size)
            self.assertEqual(ff200.filename, ff201.filename)
            self.assertEqual(ff200.timestamp, ff201.timestamp)
            self.assertEqual(ff200.timestamp_raw, ff201.timestamp_raw)
            self.assertEqual(ff200.permissions, ff201.permissions)
            self.assertEqual(ff200.perm_mode, ff201.perm_mode)
            self.assertEqual(ff200.checksum, ff201.checksum)
            self.assertIs(ff200.is_file(), ff201.is_file())
            self.assertIs(ff200.is_directory(), ff201.is_directory())

            af = ff200.open()
            self.assertEqual(self.test_string, af.read())
            af.close()

    def test_sapcar_archive_file_length(self):

        for cls in [SAPCARArchiveFilev200Format, SAPCARArchiveFilev201Format]:
            ff = cls()

            # Test several get file length combinations
            ff.file_length_low = 0
            ff.file_length_high = 0
            self.assertEqual(ff.file_length, 0)

            ff.file_length_low = 99999
            ff.file_length_high = 0
            self.assertEqual(ff.file_length, 99999)

            ff.file_length_low = 0
            ff.file_length_high = 99999
            self.assertEqual(ff.file_length, SIZE_FOUR_GB * 99999)

            ff.file_length_low = 99999
            ff.file_length_high = 99999
            self.assertEqual(ff.file_length, (SIZE_FOUR_GB * 99999) + 99999)

            # Test several set file length combinations
            ff.file_length = 0
            self.assertEqual(ff.file_length_low, 0)
            self.assertEqual(ff.file_length_high, 0)

            ff.file_length = 99999
            self.assertEqual(ff.file_length_low, 99999)
            self.assertEqual(ff.file_length_high, 0)

            ff.file_length = SIZE_FOUR_GB * 99999
            self.assertEqual(ff.file_length_low, 0)
            self.assertEqual(ff.file_length_high, 99999)

            ff.file_length = (SIZE_FOUR_GB * 99999) + 99999
            self.assertEqual(ff.file_length_low, 99999)
            self.assertEqual(ff.file_length_high, 99999)


def test_suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPCARTest))
    return suite


if __name__ == "__main__":
    test_runner = unittest.TextTestRunner(verbosity=2, resultclass=unittest.TextTestResult)
    result = test_runner.run(test_suite())
    sys.exit(not result.wasSuccessful())
