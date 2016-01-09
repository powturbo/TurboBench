.. User guide frontend

User guide
==========

The following parts of the documentation contains some background information
about pysap, as well as some step-by-step instructions for installing,
configuring and using pysap.


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


References
----------

Additional information about the protocols and the research can be found at
different publications:

* `Uncovering SAP vulnerabilities: reversing and breaking the Diag protocol <https://www.coresecurity.com/corelabs-research/publications/uncovering-sap-vulnerabilities-reversing-and-breaking-diag-protocol-brucon2012>`_

* `SAP’s Network Protocols Revisited <https://www.coresecurity.com/corelabs-research/publications/sap-network-protocols-revisited>`_

* `HoneySAP: Who really wants your money <https://www.coresecurity.com/corelabs-research/publications/honeysap-who-really-wants-your-money>`_

And advisories:

* `SAP Netweaver Dispatcher Multiple Vulnerabilities <https://www.coresecurity.com/content/sap-netweaver-dispatcher-multiple-vulnerabilities>`_

* `SAP Netweaver Message Server Multiple Vulnerabilities <https://www.coresecurity.com/content/SAP-netweaver-msg-srv-multiple-vulnerabilities>`_

* `SAP Router Password Timing Attack <https://www.coresecurity.com/advisories/sap-router-password-timing-attack>`_

* `SAP Netweaver Enqueue Server Trace Pattern Denial of Service Vulnerability <https://www.coresecurity.com/advisories/sap-netweaver-enqueue-server-trace-pattern-denial-service-vulnerability>`_

* `SAP LZC LZH Compression Multiple Vulnerabilities <https://www.coresecurity.com/advisories/sap-lzc-lzh-compression-multiple-vulnerabilities>`_


.. _installation:

Installation
------------

This section of the documentation covers the installation process of pysap.


Installation with pip
~~~~~~~~~~~~~~~~~~~~~

Installing pysap is simple with `pip <https://pip.pypa.io/>`_, just run the
following command on a terminal::

    $ pip install pysap

Some example scripts has additional required libraries:

- ``tabulate``
- ``netaddr``
- `wxPython <http://www.wxpython.org/>`_
- `fau_timer <https://github.com/martingalloar/mona-timing-lib>`_

Some of those extra libraries can be installed with `pip`_ running the following
command::

    $ pip install pysap[examples]

`Sphinx <https://sphinx-doc.org/>`_ is also required for building the API
documentation. For installing it use::

    $ pip install pysap[docs]


Manual installation
~~~~~~~~~~~~~~~~~~~

The tool relays on the `Scapy <http://www.secdev.org/projects/scapy/>`_
library for crafting packets. To install the required libraries use::

    $ pip install -r requirements.txt

Once you have downloaded pysap's sources, you can install it easily using
the Python's ``setuptools`` script provided:

1) ``python setup.py test``

2) ``python setup.py install``


Scapy installation
~~~~~~~~~~~~~~~~~~

- For installing `Scapy`_ on Windows, see some guidance `here <https://bitbucket.org/secdev/scapy/src/0bde7b23266e7cf1ba1ff3e3693fb3789c0ac751/doc/scapy/installation.rst?at=default>`_.

- Some scapy installations also requires the following steps:

    Edit the file ``supersocket.py`` (located for example on
    	``/usr/local/lib/python2.7/dist-packages/scapy/supersocket.py``)

    Add the line ``from scapy.packet import Padding``

- Additionally, running `Scapy`_ on Windows might require some patching:

    Edit the file ``supersocket.py``

    Add the line ``import arch`` at the end of the file

  See this `scapy issue <https://bitbucket.org/secdev/scapy/pull-request/56>`_
  for more details.
