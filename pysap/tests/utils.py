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
from binascii import unhexlify
from os.path import join as join, dirname


def data_filename(filename):
    return join(dirname(__file__), 'data', filename)


def read_data_file(filename, unhex=True):
    filename = data_filename(filename)
    with open(filename, 'r') as f:
        data = f.read()

    data = data.replace('\n', ' ').replace(' ', '')
    if unhex:
        data = unhexlify(data)

    return data
