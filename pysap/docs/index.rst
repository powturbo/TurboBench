.. pysap documentation master file, created by
   sphinx-quickstart on Mon Nov 30 19:17:45 2015.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

pysap - Python library for crafting SAP's network protocols packets
===================================================================

Version v\ |release| (:ref:`installation`)

`SAP Netweaver <https://www.sap.com/platform/netweaver/index.epx>`_ is a
technology platform for building and integrating SAP business applications.
Communication between components uses different network protocols. While some
of them are standard and well-known protocols, others are proprietaries and
public information is not available.

`pysap <https://www.coresecurity.com/corelabs-research/open-source-tools/pysap>`_
is a Python library that provides modules for crafting and sending packets using
SAP's NI, Message Server, Router, RFC, SNC, Enqueue and Diag protocols. The
modules are based on `Scapy <http://www.secdev.org/projects/scapy/>`_ and are
based on information acquired at researching the different protocols and services.


Content
=======

.. toctree::
   :maxdepth: 3

   user/user
   api/api

.. examples/examples
.. dev/dev


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

