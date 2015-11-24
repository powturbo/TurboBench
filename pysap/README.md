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

[SAP Netweaver](http://www.sap.com/platform/netweaver/index.epx) [1] is a 
technology platform for building and integrating SAP business applications.
Communication between components uses different network protocols. While
some of them are standard and well-known protocols, others are proprietaries
and public information is not available.

This Python library provides modules for crafting and sending packets using
SAP's NI, Message Server, Router, RFC, SNC, Enqueue and Diag protocols. The
modules are based on [Scapy](http://www.secdev.org/projects/scapy/) [2] and
are based on information acquired at researching the different protocols and
services. Detailed information about the research can be found at [3], [4], 
[5], [6] and [7].


Features
--------

This tool counts with the following components:

- `SAPNI` module

    Scapy class for the SAP NI (Network Interface protocol). It also includes a
    Stream Socket implementation for the SAP NI protocol, as well as a base 
    proxy and server implementations.

- `SAPDiag` module

    Contain Scapy classes for craft and dissect Diag DP headers, Diag packets 
    and items. The main class is `SAPDiag`, that is in charge of handling 
    compression/decompression of payload items and serve as a container for 
    them.

- `SAPDiagItems` module

    Some classes for craft and dissect common Diag items.

- `SAPDiagClient` module

    Basic class for establishing a connection with an application server.

- `SAPEnqueue` module

    Scapy classes for the Enqueue protocol.

- `SAPRouter` module

    Scapy classes for the different SAP Router packets (route, control, error
    and admin messages).

- `SAPMS` module

    Scapy classes for the Message Server protocol.
    
- `SAPSNC` module

    Basic class to serve as container of SNC Frames found in SAP Router and
    SAP Diag packets. 

- Examples

    Example and proof of concept scripts to illustrate the use of the different
    modules and protocols: login brute force, gather information on the
    application server, intercept communications, a rogue Diag server
    implementation, test of Denial of Server issues [4], a Message Server
    monitor implementation, listener/messenger for Message Server, SAP Router
    internal networks scanner, port forwarder, fingerprinting, etc.
	

Installation & Build
--------------------

### Installation with pip ###

Installing pysap is simple with [pip](https://pip.pypa.io/), just run the
following command on a terminal:

    $ pip install pysap

Some example scripts has additional required libraries:

- tabulate
- netaddr
- [wxPython](http://www.wxpython.org/)
- [fau_timer](https://github.com/martingalloar/mona-timing-lib)

Some of those extra libraries can be installed with `pip` running the following
command:

    $ pip install pysap[examples] 

### Manual installation ###

The tool relays on the [Scapy](http://www.secdev.org/projects/scapy/) [2]
library for crafting packets. [Epydoc](http://epydoc.sourceforge.net/) is
also required for building the API documentation. To install the required
libraries use:

    $ pip install -r requirements.txt

Once you have downloaded pysap's sources, you can install it easily using
the Python's `setuptools` script provided:

1) `$ python setup.py test`

2) `$ python setup.py install`

### Scapy installation ###

- For installing Scapy on Windows, see some guidance [here](https://bitbucket.org/secdev/scapy/src/0bde7b23266e7cf1ba1ff3e3693fb3789c0ac751/doc/scapy/installation.rst?at=default).

- Some scapy installations also requires the following steps:

    Edit the file `supersocket.py` (located for example on 
    	`/usr/local/lib/python2.7/dist-packages/scapy/supersocket.py`)

    Add the line `from scapy.packet import Padding`

- Additionally, runnnig Scapy on Windows might require some patching:

    Edit the file `supersocket.py` 

    Add the line `import arch` at the end of the file

  See this [scapy issue](https://bitbucket.org/secdev/scapy/pull-request/56)
  for more details.


Documentation
-------------

Documentation of the API can be build using:

    $ python setup.py doc

A build is also available at [http://pythonhosted.org/pysap/](http://pythonhosted.org/pysap/).


Example uses
------------

Examples can be found in the `examples` directory.


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


References
----------

[1] http://www.sap.com/platform/netweaver/index.epx

[2] http://www.secdev.org/projects/scapy/

[3] http://www.coresecurity.com/corelabs-research/open-source-tools/pysap

[4] http://www.coresecurity.com/content/sap-netweaver-dispatcher-multiple-vulnerabilities

[5] http://www.coresecurity.com/content/SAP-netweaver-msg-srv-multiple-vulnerabilities

[6] http://www.coresecurity.com/corelabs-research/publications/uncovering-sap-vulnerabilities-reversing-and-breaking-diag-protocol-brucon2012

[7] http://www.coresecurity.com/corelabs-research/publications/sap-network-protocols-revisited


Contact
-------

Whether you want to report a bug or give some suggestions on this package, drop
us a few lines at `oss@coresecurity.com` or contact the author email 
`mgallo@coresecurity.com`.
