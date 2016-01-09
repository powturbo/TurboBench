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
import socket
import unittest
from struct import unpack
from threading import Thread
# External imports

# Custom imports
from pysap.SAPNI import SAPNIServerHandler, SAPNIServerThreaded, SAPNI
from pysap.SAPRouter import (SAPRouter, SAPRouterRouteHop, router_is_route,
                             SAPRoutedStreamSocket, SAPRouteException)


class PySAPRouterTest(unittest.TestCase):

    def check_route(self, route_string, route_hops):
        """Check from string to hops and back again"""
        hops = SAPRouterRouteHop.from_string(route_string)
        self.assertListEqual(hops, route_hops)

        string = SAPRouterRouteHop.from_hops(hops)
        self.assertEqual(string, route_string)

    def test_saprouter_route_string(self):
        """Test construction of SAPRouterRouteHop items"""

        # Two hops with full details
        self.check_route("/H/host1/S/service1/W/pass1/H/host2/S/service2/W/pass2",
                         [SAPRouterRouteHop(hostname="host1",
                                            port="service1",
                                            password="pass1"),
                          SAPRouterRouteHop(hostname="host2",
                                            port="service2",
                                            password="pass2")])

        # One intermediate hop with service/password
        self.check_route("/H/host1/H/host2/S/service2/W/pass2/H/host3",
                         [SAPRouterRouteHop(hostname="host1"),
                          SAPRouterRouteHop(hostname="host2",
                                            port="service2",
                                            password="pass2"),
                          SAPRouterRouteHop(hostname="host3")])

        # Example in SAP Help
        self.check_route("/H/sap_rout/H/your_rout/W/pass_to_app/H/yourapp/S/sapsrv",
                         [SAPRouterRouteHop(hostname="sap_rout"),
                          SAPRouterRouteHop(hostname="your_rout",
                                            password="pass_to_app"),
                          SAPRouterRouteHop(hostname="yourapp",
                                            port="sapsrv")])

        # Hostname with FQDN
        self.check_route("/H/some.valid.domain.com/S/3299",
                         [SAPRouterRouteHop(hostname="some.valid.domain.com",
                                            port="3299")])

        # Hostname with IP addresses
        self.check_route("/H/127.0.0.1/S/3299",
                         [SAPRouterRouteHop(hostname="127.0.0.1",
                                            port="3299")])

        # Invalid route strings
        self.assertListEqual(SAPRouterRouteHop.from_string("/S/service"), [])
        self.assertListEqual(SAPRouterRouteHop.from_string("/P/password"), [])


class SAPRouterServerTestHandler(SAPNIServerHandler):
    """Basic SAP Router server that accepts a specific route and then
    acts as an echo server."""

    def __init__(self, request, client_address, server):
        self.routed = False
        SAPNIServerHandler.__init__(self, request, client_address, server)

    def handle_data(self):
        if self.routed:
            self.request.send(self.packet)
            return

        self.packet.decode_payload_as(SAPRouter)
        route_request = self.packet[SAPRouter]

        if router_is_route(route_request):
            if route_request.route_string[1].hostname == "10.0.0.1" and \
               route_request.route_string[1].port == "3200":
                self.routed = True
                self.request.send(SAPRouter(type=SAPRouter.SAPROUTER_PONG))
            else:
                self.request.send(SAPRouter(type=SAPRouter.SAPROUTER_ERROR,
                                            return_code=-94))
        else:
            self.request.send(SAPRouter(type=SAPRouter.SAPROUTER_ERROR))


class PySAPRoutedStreamSocketTest(unittest.TestCase):

    test_port = 8005
    test_address = "127.0.0.1"
    test_string = "TEST" * 10

    def start_server(self, handler_cls):
        self.server = SAPNIServerThreaded((self.test_address, self.test_port),
                                          handler_cls,
                                          bind_and_activate=False)
        self.server.allow_reuse_address = True
        self.server.server_bind()
        self.server.server_activate()
        self.server_thread = Thread(target=self.server.serve_forever)
        self.server_thread.start()

    def stop_server(self):
        self.server.shutdown()
        self.server.server_close()
        self.server_thread.join()

    def test_saproutedstreamsocket(self):
        """Test SAPRoutedStreamSocket"""
        self.start_server(SAPRouterServerTestHandler)

        sock = socket.socket()
        sock.connect((self.test_address, self.test_port))

        route = [SAPRouterRouteHop(hostname=self.test_address,
                                   port=self.test_port),
                 SAPRouterRouteHop(hostname="10.0.0.1",
                                   port="3200")]

        self.client = SAPRoutedStreamSocket(sock, route=route,
                                            router_version=40)
        packet = self.client.sr(self.test_string)

        self.assertIn(SAPNI, packet)
        self.assertEqual(packet[SAPNI].length, len(self.test_string) + 4)
        self.assertEqual(unpack("!I", packet[SAPNI].payload.load[:4]), (len(self.test_string), ))
        self.assertEqual(packet[SAPNI].payload.load[4:], self.test_string)

        self.client.close()
        self.stop_server()

    def test_saproutedstreamsocket_route_error(self):
        """Test SAPRoutedStreamSocket throwing of SAPRouteException if
        a route denied return error is received"""
        self.start_server(SAPRouterServerTestHandler)

        sock = socket.socket()
        sock.connect((self.test_address, self.test_port))

        route = [SAPRouterRouteHop(hostname=self.test_address,
                                   port=self.test_port),
                 SAPRouterRouteHop(hostname="10.0.0.2",
                                   port="3200")]

        with self.assertRaises(SAPRouteException):
            self.client = SAPRoutedStreamSocket(sock, route=route,
                                                router_version=40)

        self.stop_server()

    def test_saproutedstreamsocket_error(self):
        """Test SAPRoutedStreamSocket throwing of Exception if an invalid
        or unexpected packet is received"""
        self.start_server(SAPRouterServerTestHandler)

        sock = socket.socket()
        sock.connect((self.test_address, self.test_port))

        with self.assertRaises(Exception):
            self.client = SAPRoutedStreamSocket(sock, route=None,
                                                router_version=40)

        self.stop_server()

    def test_saproutedstreamsocket_getnisocket(self):
        """Test SAPRoutedStreamSocket get nisocket class method"""
        self.start_server(SAPRouterServerTestHandler)

        # Test using a complete route
        route = [SAPRouterRouteHop(hostname=self.test_address,
                                   port=self.test_port),
                 SAPRouterRouteHop(hostname="10.0.0.1",
                                   port="3200")]
        self.client = SAPRoutedStreamSocket.get_nisocket(route=route,
                                                         router_version=40)

        packet = self.client.sr(self.test_string)
        self.assertIn(SAPNI, packet)
        self.assertEqual(packet[SAPNI].length, len(self.test_string) + 4)
        self.assertEqual(unpack("!I", packet[SAPNI].payload.load[:4]), (len(self.test_string), ))
        self.assertEqual(packet[SAPNI].payload.load[4:], self.test_string)

        # Test using a route and a target host/port
        route = [SAPRouterRouteHop(hostname=self.test_address,
                                   port=self.test_port)]
        self.client = SAPRoutedStreamSocket.get_nisocket("10.0.0.1",
                                                         "3200",
                                                         route=route,
                                                         router_version=40)

        packet = self.client.sr(self.test_string)
        self.assertIn(SAPNI, packet)
        self.assertEqual(packet[SAPNI].length, len(self.test_string) + 4)
        self.assertEqual(unpack("!I", packet[SAPNI].payload.load[:4]), (len(self.test_string), ))
        self.assertEqual(packet[SAPNI].payload.load[4:], self.test_string)

        # Test using a route string
        route = "/H/%s/S/%s/H/10.0.0.1/S/3200" % (self.test_address,
                                                  self.test_port)
        self.client = SAPRoutedStreamSocket.get_nisocket(route=route,
                                                         router_version=40)

        packet = self.client.sr(self.test_string)
        self.assertIn(SAPNI, packet)
        self.assertEqual(packet[SAPNI].length, len(self.test_string) + 4)
        self.assertEqual(unpack("!I", packet[SAPNI].payload.load[:4]), (len(self.test_string), ))
        self.assertEqual(packet[SAPNI].payload.load[4:], self.test_string)

        self.client.close()
        self.stop_server()


def suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PySAPRouterTest))
    suite.addTest(loader.loadTestsFromTestCase(PySAPRoutedStreamSocketTest))
    return suite


if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite())
