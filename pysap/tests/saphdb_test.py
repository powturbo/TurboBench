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
from threading import Thread
from SocketServer import BaseRequestHandler, ThreadingTCPServer
# Custom imports
from pysap.SAPHDB import SAPHDBConnection


class SAPHDBServerTestHandler(BaseRequestHandler):
    """Basic SAP HDB server that performs initialization."""

    def handle_data(self):
        self.request.recv(14)
        self.request.send("\x00" * 8)


class PySAPHDBConnectionTest(unittest.TestCase):

    test_port = 30017
    test_address = "127.0.0.1"

    def start_server(self, address, port, handler_cls):
        self.server = ThreadingTCPServer((address, port),
                                         handler_cls,
                                         bind_and_activate=False)
        self.server.allow_reuse_address = True
        self.server.server_bind()
        self.server.server_activate()
        self.server_thread = Thread(target=self.server.serve_forever)
        self.server_thread.daemon = True
        self.server_thread.start()

    def stop_server(self):
        self.server.shutdown()
        self.server.server_close()
        self.server_thread.join(1)

    def test_saphdbconnection_initialize(self):
        """Test HDB Connection initialize"""
        self.start_server(self.test_address, self.test_port, SAPHDBServerTestHandler)

        client = SAPHDBConnection(self.test_address, self.test_port)
        client.connect()
        client.initialize()

        self.stop_server()


def test_suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPHDBConnectionTest))
    return suite


if __name__ == "__main__":
    test_runner = unittest.TextTestRunner(verbosity=2, resultclass=unittest.TextTestResult)
    result = test_runner.run(test_suite())
    sys.exit(not result.wasSuccessful())
