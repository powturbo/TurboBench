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
from tests import sapni_test
from tests import sapdiag_test
from tests import saprouter_test
from tests import pysapcompress_test


def suite():
    suite = unittest.TestSuite()
    suite.addTests(sapni_test.suite())
    suite.addTests(sapdiag_test.suite())
    suite.addTests(saprouter_test.suite())
    suite.addTests(pysapcompress_test.suite())
    return suite


if __name__ == '__main__':
    unittest.TextTestRunner(verbosity=2).run(suite())
