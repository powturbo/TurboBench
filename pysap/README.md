pysap - Python library for crafting SAP's network protocols packets
===================================================================

[![Build Status](https://travis-ci.org/CoreSecurity/pysap.svg?branch=master)](https://travis-ci.org/CoreSecurity/pysap)
[![Code Health](https://landscape.io/github/CoreSecurity/pysap/master/landscape.svg)](https://landscape.io/github/CoreSecurity/pysap/master)
[![Code Issues](http://www.quantifiedcode.com/api/v1/project/53b997e2c9054c5ab44782614c572358/badge.svg)](http://www.quantifiedcode.com/app/project/53b997e2c9054c5ab44782614c572358)
[![Latest Version](https://img.shields.io/pypi/v/pysap.svg)](https://pypi.python.org/pypi/pysap/)

Copyright (C) 2015 by Martin Gallo, Core Security

Version 0.1.9.dev0 (XXX 2015)


Overview
--------

[SAP Netweaver](http://www.sap.com/platform/netweaver/index.epx) is a 
technology platform for building and integrating SAP business applications.
Communication between components uses different network protocols. While
some of them are standard and well-known protocols, others are proprietaries
and public information is not available.

[pysap](http://www.coresecurity.com/corelabs-research/open-source-tools/pysap)
is a Python library that provides modules for crafting and sending packets
using SAP's NI, Message Server, Router, RFC, SNC, Enqueue and Diag protocols.
The modules are build on top of [Scapy](http://www.secdev.org/projects/scapy/)
and are based on information acquired at researching the different protocols
and services.


Features
--------

* Dissection and crafting of the following network protocols:

    * SAP Network Interface (NI)
    * SAP Diag
    * SAP Enqueue
    * SAP Router
    * SAP Message Server (MS)
    * SAP SNC

* Library implementing SAP's LZH and LZC compression algorithms.

* Automatic compression/decompression of payloads with SAP's algorithms.

* Client, proxy and server classes implemented for some of the protocols.

* Example scripts to illustrate the use of the different modules and protocols.
	

Installation
------------

To install pysap simply run:

    $ pip install pysap


Documentation
-------------

Documentation is available at [http://pythonhosted.org/pysap/](http://pythonhosted.org/pysap/).


License
-------

This library is distributed under the GPLv2 license. Check the `COPYING` file for
more details.


Authors
-------

The library was designed and developed by Martin Gallo from the Security
Consulting Services team of Core Security.

### Contributors ###

Contributions made by:

  * Florian Grunow
  * Scott Walsh
  * Joris van de Vis
  * Victor Portal Gonzalez 


Contact
-------

Whether you want to report a bug or give some suggestions on this package, drop
us a few lines at `oss@coresecurity.com` or contact the author email 
`mgallo@coresecurity.com`.
