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

# External imports
from scapy.packet import Packet
from scapy.fields import StrFixedLenField
# Custom imports


class SAPSNCFrame(Packet):
    """SAP SNC Frame packet

    This packet is used just to contain SNC Frames.
    """
    name = "SAP SNC Frame"
    fields_desc = [
        StrFixedLenField("eye_catcher", "SNCFRAME")
    ]
