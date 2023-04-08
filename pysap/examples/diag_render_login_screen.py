#!/usr/bin/env python2
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
import logging
from collections import defaultdict
from argparse import ArgumentParser
# External imports
from scapy.config import conf
from scapy.packet import bind_layers
# Custom imports
import pysap
from pysap.SAPNI import SAPNI
from pysap.SAPDiagItems import *
from pysap.SAPDiag import SAPDiag, SAPDiagDP
from pysap.SAPDiagClient import SAPDiagConnection

# Try to import wx for failing gracefully if not found
try:
    import wx  # TODO: Change wx to Tkinter
    has_wx = True
except ImportError:
    has_wx = False


# Bind the SAPDiag layer
bind_layers(SAPNI, SAPDiag,)
bind_layers(SAPNI, SAPDiagDP,)
bind_layers(SAPDiagDP, SAPDiag,)
bind_layers(SAPDiag, SAPDiagItem,)
bind_layers(SAPDiagItem, SAPDiagItem,)


# Set the verbosity to 0
conf.verb = 0


# Command line options parser
def parse_options():

    description = "This example script renders the login screen provided by an SAP Netweaver Application Server using "\
                  "wxPython."

    usage = "%(prog)s [options] -d <remote host>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    target = parser.add_argument_group("Target")
    target.add_argument("-d", "--remote-host", dest="remote_host",
                        help="Remote host")
    target.add_argument("-p", "--remote-port", dest="remote_port", type=int, default=3200,
                        help="Remote port [%(default)d]")
    target.add_argument("--route-string", dest="route_string",
                        help="Route string for connecting through a SAP Router")

    misc = parser.add_argument_group("Misc options")
    misc.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Verbose output")
    misc.add_argument("--terminal", dest="terminal", default=None,
                      help="Terminal name")

    options = parser.parse_args()

    if not (options.remote_host or options.route_string):
        parser.error("Remote host or route string is required")

    return options


class DiagScreen(wx.Frame):
    def __init__(self, parent, windows_title, height, width, session_title, dbname, cpuname):
        wx.Frame.__init__(self, parent, title=windows_title)

        self.maincontainer = wx.BoxSizer(wx.VERTICAL)

        self.session_title = wx.StaticBox(self, label=session_title)

        self.container = wx.StaticBoxSizer(self.session_title, wx.VERTICAL)
        self.maincontainer.Add(self.container, flag=wx.EXPAND | wx.ALL, border=10)

        self.buttonbar = wx.ToolBar(self)
        self.container.Add(self.buttonbar, flag=wx.EXPAND | wx.ALL, border=10)

        self.content = wx.GridBagSizer()
        self.container.Add(self.content)
        self.SetSizer(self.container)

        self.menubar = wx.MenuBar()
        self.SetMenuBar(self.menubar)

        self.toolbar = self.CreateToolBar()
        self.toolbar.Realize()

        self.statusbar = self.CreateStatusBar()
        self.statusbar.SetFields(["", dbname, cpuname])

        self.menus = defaultdict(defaultdict)

    def add_text(self, x, y, maxlength, text, tooltip=None):
        text_control = wx.StaticText(self, label=text)
        if tooltip:
            text_control.SetTooltip(tooltip)
        self.content.Add(text_control, pos=(y, x), flag=wx.TOP | wx.LEFT | wx.BOTTOM, border=5)

    def add_text_box(self, x, y, maxlength, text, invisible=0):
        if invisible:
            textbox_control = wx.TextCtrl(self, style=wx.TE_PASSWORD)
        else:
            textbox_control = wx.TextCtrl(self)
        textbox_control.SetMaxLength(maxlength)
        textbox_control.SetValue(text)
        self.content.Add(textbox_control, pos=(y, x), flag=wx.TOP | wx.LEFT | wx.BOTTOM, border=5)

    def add_button(self, text):
        button = wx.Button(self.buttonbar, wx.ID_ANY, text)
        self.buttonbar.AddControl(button)

    def add_toolbar(self, text):
        toolbar = wx.Button(self.toolbar, wx.ID_ANY, text)
        self.toolbar.AddControl(toolbar)

    def add_menu(self, pos1, text):
        self.menus[pos1][0] = wx.Menu()
        self.menubar.Append(self.menus[pos1][0], text)

    def add_child_menu(self, text, pos1, pos2=0, pos3=0, pos4=0, sel=0, men=0, sep=0):
        # XXX: Support menus of level 4, need to use another structure for storing the menus and their handles
        if pos4 > 0:
            return
        if sep:
            self.menus[pos1][0].AppendSeparator()
        else:
            if men:
                self.menus[pos1][pos2] = wx.Menu()
                item = self.menus[pos1][0].AppendMenu(wx.ID_ANY, text, self.menus[pos1][pos2])
            else:
                if pos3 > 0:
                    item = self.menus[pos1][pos2].Append(wx.ID_ANY, text)
                else:
                    item = self.menus[pos1][0].Append(wx.ID_ANY, text)
            item.Enable(sel == 1)


def render_diag_screen(screen, verbose):
    """
    Renders the Dynt Atom items of a message

    """

    def get_item_value(screen, item_type, item_id, item_sid, i=0):
        item = screen.get_item(item_type, item_id, item_sid)
        if item:
            return item[i].item_value
        else:
            return []

    areasize = get_item_value(screen, "APPL", "VARINFO", "AREASIZE")
    dbname = get_item_value(screen, "APPL", "ST_R3INFO", "DBNAME")
    cpuname = get_item_value(screen, "APPL", "ST_R3INFO", "CPUNAME")
    client = get_item_value(screen, "APPL", "ST_R3INFO", "CLIENT")
    session_icon = get_item_value(screen, "APPL", "VARINFO", "SESSION_ICON")
    session_title = get_item_value(screen, "APPL", "VARINFO", "SESSION_TITLE")
    menus = get_item_value(screen, "APPL4", "MNUENTRY", "MENU_ACT")
    menudetails = get_item_value(screen, "APPL4", "MNUENTRY", "MENU_MNU")
    buttonbars = get_item_value(screen, "APPL4", "MNUENTRY", "MENU_PFK")
    toolbars = get_item_value(screen, "APPL4", "MNUENTRY", "MENU_KYB")

    if verbose:
        print("[*] DB Name: " + dbname)
        print("[*] CPU Name: " + cpuname)
        print("[*] Client: " + client)
        print("[*] Session Icon: " + session_icon)
        print("[*] Session Title: " + session_title)
        print("[*] Window Size: " + areasize.window_height + " x " + areasize.window_width)

    app = wx.App(False)
    login_frame = DiagScreen(None, "%s (%s)" % (session_icon, client), areasize.window_height, areasize.window_width, session_title, dbname, cpuname)

    # Render the atoms (control boxes and labels)
    atoms = screen.get_item(["APPL", "APPL4"], "DYNT", "DYNT_ATOM")
    if atoms:
        for atom_item in [atom for atom_item in atoms for atom in atom_item.item_value.items]:
            if atom_item.etype in [121, 123]:
                text = atom_item.field1_text
                maxnrchars = atom_item.field1_maxnrchars
            elif atom_item.etype in [130, 132]:
                text = atom_item.field2_text
                maxnrchars = atom_item.field2_maxnrchars
            else:
                text = None
                maxnrchars = 0

            if text is not None:
                if atom_item.etype in [123, 132]:  # DIAG_DGOTYP_KEYWORD_1 or DIAG_DGOTYP_KEYWORD_2
                    if text.find("@\Q") >= 0:
                        tooltip = text.split("@")[1][2:]
                        text = text.split("@")[2]
                    else:
                        tooltip = None
                    if verbose:
                        print("[*] Found text label at %d,%d: \"%s\" (maxlength=%d) (tooltip=\"%s\")" % (atom_item.col, atom_item.row, text.strip(), maxnrchars, tooltip))
                    login_frame.add_text(atom_item.col, atom_item.row, maxnrchars, text)
                elif atom_item.etype in [121, 130]:  # DIAG_DGOTYP_EFIELD_1 or DIAG_DGOTYP_EFIELD_2
                    if verbose:
                        print("[*] Found text box at %d,%d: \"%s\" (maxlength=%d)" % (atom_item.col, atom_item.row, text.strip(), maxnrchars))
                    login_frame.add_text_box(atom_item.col, atom_item.row, maxnrchars, text.strip(), atom_item.attr_DIAG_BSD_INVISIBLE == 1)
            else:
                print("[*] Found label without text")

    # Render the menus
    if menus:
        for menu in menus.entries:
            if verbose:
                print("[*] Found menu item: \"%s\"" % menu.text)
            login_frame.add_menu(menu.position_1, menu.text)

        # Render the submenus
        if menudetails:
            for menu in menudetails.entries:
                if verbose:
                    print("[*] Found child menu item: \"%s\", pos %d, %d, %d, %d" % (menu.text, menu.position_1, menu.position_2, menu.position_3, menu.position_4))
                login_frame.add_child_menu(menu.text, menu.position_1, menu.position_2, menu.position_3, menu.position_4, menu.flag_TERM_SEL, menu.flag_TERM_MEN, menu.flag_TERM_SEP)

    # Render the buttonbar
    if buttonbars:
        for button in buttonbars.entries:
            if verbose:
                print("[*] Found button item: \"%s\"" % button.text)
            login_frame.add_button(button.text)

    # Render the toolbar
    if toolbars:
        for toolbar in toolbars.entries:
            if verbose:
                print("[*] Found toolbar item: \"%s\"" % toolbar.text)
            login_frame.add_toolbar(toolbar.text)

    login_frame.Show(True)
    app.MainLoop()


# Main function
def main():
    options = parse_options()

    if not has_wx:
        print("[-] Required library not found. Please install it from https://wxpython.org/")
        return

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Create the connection to the SAP Netweaver server
    print("[*] Connecting to %s port %d" % (options.remote_host, options.remote_port))
    connection = SAPDiagConnection(options.remote_host,
                                   options.remote_port,
                                   terminal=options.terminal,
                                   route=options.route_string)

    # Send the initialization packet and store the response (login screen)
    login_screen = connection.init()

    print("[*] Login screen grabbed, rendering it")
    render_diag_screen(login_screen[SAPDiag], options.verbose)

    # Close the connection
    connection.close()


if __name__ == "__main__":
    main()
