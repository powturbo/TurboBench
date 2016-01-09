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
from struct import pack
# External imports
from scapy.fields import StrField
from scapy.packet import Packet, Raw
# Custom imports
from tests.utils import read_data_file
from pysap.SAPDiag import (SAPDiagItems, SAPDiagItem, SAPDiag, bind_diagitem,
                           diag_item_get_class)
from pysap.SAPDiagItems import SAPDiagDyntAtomItem


class PySAPDiagTest(unittest.TestCase):

    def test_sapdiag_header_build(self):
        """Test SAPDiag headers building"""
        diag_item = SAPDiagItem(item_value="TEST")

        diag_header_plain = SAPDiag(compress=0)
        diag_header_plain.message.append(diag_item)
        diag_plain_message = str(diag_header_plain.message)

        diag_header_compr = SAPDiag(compress=1)
        diag_header_compr.message.append(diag_item)
        diag_compr_message = str(diag_header_compr.message)

        self.assertEqual(diag_plain_message, diag_compr_message)

        diag_header_compr.compress = 0
        self.assertEqual(str(diag_header_plain), str(diag_header_compr))

    def test_sapdiag_header_dissection(self):
        """Test SAPDiag headers dissection"""
        diag_item = SAPDiagItem(item_value="TEST")

        diag_header_plain = SAPDiag(compress=0)
        diag_header_plain.message.append(diag_item)
        new_diag_header_plain = SAPDiag(str(diag_header_plain))

        self.assertEqual(str(diag_header_plain),
                         str(new_diag_header_plain))

        diag_header_compr = SAPDiag(compress=1)
        diag_header_compr.message.append(diag_item)
        new_diag_header_compr = SAPDiag(str(diag_header_compr))

        self.assertEqual(str(diag_header_compr),
                         str(new_diag_header_compr))

    def test_sapdiag_item(self):
        """Test construction of SAPDiag Items"""
        with self.assertRaises(KeyError):
            SAPDiagItem(item_type="LALA")

    def test_sapdiag_items(self):
        """Test dissection of Diag Items"""
        diag_items = SAPDiagItems(read_data_file('nw_703_login_screen_decompressed.data'))

        self.assertEqual(len(diag_items.message), 131)

        for item in diag_items.message:
            self.assertIsInstance(item, SAPDiagItem)

    def test_sapdiag_atoms(self):
        """Test dissection of Diag Items and Dynt Atom Items"""
        diag_items = SAPDiagItems(read_data_file('nw_703_login_screen_decompressed.data'))
        diag_packet = SAPDiag(message=diag_items.message)
        diag_atoms = diag_packet.get_item(0x12, 0x09, 0x02)

        for atom in diag_atoms:
            for atom_item in atom.item_value.items:
                self.assertIsInstance(atom_item, SAPDiagDyntAtomItem)

    def test_sapdiag_items_lookup(self):
        """Test lookup and filtering of SAPDiagItems inside a SAPDiag
        packet"""
        sapdiag = SAPDiag()

        sapdiag_ses_item = SAPDiagItem(item_type="SES")
        sapdiag.message.append(sapdiag_ses_item)

        sapdiag_appl_item = SAPDiagItem(item_type="APPL",
                                        item_id="ST_USER",
                                        item_sid="RFC_PARENT_UUID")
        sapdiag.message.append(sapdiag_appl_item)

        self.assertIn(sapdiag_ses_item, sapdiag.get_item(0x1))
        self.assertIn(sapdiag_ses_item, sapdiag.get_item("SES"))
        self.assertNotIn(sapdiag_ses_item, sapdiag.get_item(0x10))
        self.assertNotIn(sapdiag_ses_item, sapdiag.get_item("APPL"))

        self.assertIn(sapdiag_appl_item, sapdiag.get_item(0x10))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item("APPL"))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item("APPL", 0x04))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item("APPL", "ST_USER"))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item("APPL", 0x04, 0x10))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item("APPL", "ST_USER", 0x10))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item("APPL", "ST_USER", "RFC_PARENT_UUID"))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item(0x1))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item("APPL4"))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item("APPL", 0x06))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item("APPL", "ST_R3INFO"))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item("APPL", 0x04, 0x02))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item("APPL", "ST_USER", 0x02))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item("APPL", "ST_USER", "CONNECT"))

        self.assertListEqual([sapdiag_ses_item, sapdiag_appl_item], sapdiag.get_item([0x01, 0x10]))
        self.assertListEqual([sapdiag_ses_item, sapdiag_appl_item], sapdiag.get_item(["SES", "APPL"]))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item(["APPL"], [0x04, 0x06]))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item(["APPL"], ["ST_USER", "ST_R3INFO"]))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item(["APPL"], 0x04, [0x02, 0x10]))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item(["APPL"], "ST_USER", ["RFC_PARENT_UUID", "CONNECT"]))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item(["SES", "APPL4"]))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item(["APPL"], ["ST_R3INFO"]))
        self.assertNotIn(sapdiag_appl_item, sapdiag.get_item(["APPL"], ["ST_USER"], ["CONNECT"]))

        # Insert a wrong item and observe that the lookup still works
        sapdiag.message.append(Raw("\x00" * 10))
        self.assertIn(sapdiag_ses_item, sapdiag.get_item("SES"))
        self.assertIn(sapdiag_appl_item, sapdiag.get_item(["APPL"], "ST_USER", ["RFC_PARENT_UUID", "CONNECT"]))

    def test_sapdiag_items_bind(self):
        """Test binding of SAPDiagItem classes"""
        class SAPDiagItemTest(Packet):
            fields_desc = [StrField("strfield", None)]
        bind_diagitem(SAPDiagItemTest, "APPL", 0x99, 0xff)

        item_string = "strfield"
        item_value = SAPDiagItemTest(strfield=item_string)
        item = SAPDiagItem("\x10\x99\xff" + pack("!H", len(item_string)) + item_string)

        self.assertEqual(item.item_value, item_value)
        self.assertEqual(item.item_length, len(item_string))
        self.assertEqual(item.item_value.strfield, item_string)
        self.assertEqual(str(item.item_value), str(item_value))
        self.assertIs(diag_item_get_class(item, "APPL", 0x99, 0xff), SAPDiagItemTest)


def suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPDiagTest))
    return suite


if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite())
