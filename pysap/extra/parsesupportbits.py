#!/usr/bin/env python
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
from optparse import OptionParser
# Custom imports
import pysap


# Command line options parser
def parse_options():

    description = \
    """This script can be used to parse support bits info  and generate
    required info for pysap/wireshark plugin. Input file can be obtained
    from SAP Gui traces (for example file 'sapguidll_01_0001.trc').
    """

    epilog = "pysap %(version)s - %(url)s - %(repo)s" % {"version": pysap.__version__,
                                                         "url": pysap.__url__,
                                                         "repo": pysap.__repo__}

    usage = "Usage: %prog -i <input_file>"

    parser = OptionParser(usage=usage, description=description, epilog=epilog)
    parser.add_option("-i", "--input", dest="input_file", help="Input file")

    (options, _) = parser.parse_args()

    if not options.input_file:
        parser.error("Input file required")

    return options


# Main function
def main():
    options = parse_options()
    print("[*] Parsing input file", options.input_file)

    # Read and parse the data
    with open(options.input_file, 'r') as f:
        input_data = f.readlines()

    data = {}
    for line in input_data:
        support_bit = line.split(' ', 1)[1].split('(', 1)
        name, bit = support_bit[0], int(support_bit[1].split(')', 1)[0])
        data[bit] = name

    # Fill missing bits
    unknown_no = unused_no = 1
    for bit in range(0, max(data.keys()) + 1):
        if bit in data:
            name = data[bit].replace(' ', '_')
            if name == "UNUSED":
                name = "UNUSED_%d" % unused_no
                unused_no += 1

            unknown = False
        else:
            name = "UNKNOWN_%d" % unknown_no
            unknown_no += 1
            unknown = True

        data[bit] = name, unknown

    pysap = wireshark_define = wireshark_hf = wireshark_parse = wireshark_module = ''
    for bit in list(data.keys()):
        name, unknown = data[bit]
        notice = " (Unknown support bit)" if unknown else ''

        if (bit % 8) == 0 and bit != 0:
            pysap += '\n'
            wireshark_define += '\n'
            wireshark_parse += 'offset+=1;\n'
            wireshark_module += '\n'

        pysap += '        BitField("%s", 0, 1),  # %d%s\n' % (name, bit, notice)

        bitt = 1 << bit % 8
        wireshark_define += '#define SAPDIAG_SUPPORT_BIT_%s\t0x%02x  /* %d%s */\n' % (name, bitt, bit, notice)

        wireshark_hf += 'static int hf_SAPDIAG_SUPPORT_BIT_%s = -1;\n' % name

        wireshark_parse += 'proto_tree_add_item(tree, hf_SAPDIAG_SUPPORT_BIT_%s, tvb, offset, 1, FALSE);  /* %d%s */\n' % (name, bit, notice)

        wireshark_module += '{ &hf_SAPDIAG_SUPPORT_BIT_%s,\n\t{ "Support Bit %s", ' \
                            '"sapdiag.diag.supportbits.%s", FT_BOOLEAN, 8, NULL, ' \
                            'SAPDIAG_SUPPORT_BIT_%s, "SAP Diag Support Bit %s",\n\tHFILL }},\n' % (name, name, name, name, name)

    print("[*] pysap SAPDiagItems definition:")
    print(pysap)
    print("[*] wireshark plugin define:")
    print(wireshark_define)
    print("[*] wireshark plugin hf definitions:")
    print(wireshark_hf)
    print("[*] wireshark plugin parsing:")
    print(wireshark_parse)
    print("[*] wireshark plugin module:")
    print(wireshark_module)


if __name__ == "__main__":
    main()
