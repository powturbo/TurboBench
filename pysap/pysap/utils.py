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
import struct
from cmd import Cmd
from threading import Thread, Event
# External imports
from scapy.packet import Packet
from scapy.volatile import (RandEnum, RandomEnumeration, RandNum, RandTermString,
                            RandBin)
from scapy.fields import (ByteEnumField, ShortEnumField, MultiEnumField,
                          StrLenField, Field, StrFixedLenField, StrField)
# Optional imports
try:
    from tabulate import tabulate
except ImportError:
    tabulate = None


class PacketNoPadded(Packet):
    """Regular scapy packet with no padding.
    """
    def extract_padding(self, s):
        return '', s


class RandEnumKeys(RandEnum):
    """Picks a random value from dict keys list. Used for fuzzing enum fields.

    """
    def __init__(self, enum):
        self.enum = []
        for key in list(enum.keys()):
            self.enum.append(key)
        self.seq = RandomEnumeration(0, len(list(enum.keys())) - 1)

    def _fix(self):
        return self.enum[self.seq.next()]


class ByteEnumKeysField(ByteEnumField):
    """ByteEnumField that picks valid values when fuzzed.
    """
    def randval(self):
        return RandEnumKeys(self.i2s)


class ShortEnumKeysField(ShortEnumField):
    """ShortEnumField that picks valid values when fuzzed.
    """
    def randval(self):
        return RandEnumKeys(self.i2s)


class RandByteReduced(RandNum):
    """RandByte that only returns random values between 0 and x2a. Used while
    performing some fuzz to reduce the test cases space.

    """
    def __init__(self):
        RandNum.__init__(self, 0, 0x2a)


class ByteMultiEnumKeysField(MultiEnumField):
    """MultiEnumField that picks a reduced number of values. Used for fuzzing
    Byte fields with reduced number of values.

    """
    def randval(self):
        return RandByteReduced()


class MutablePacketField(StrLenField):
    """Packet field that mutates the class according to a list of evaluators.
    The evaluators are run against the packet and given to a class getter.

    If the class can't be found, the field is treated as a StrLenField.
    """
    def __init__(self, name, default, length_from, get_class, evaluators=None):
        """
        @param length_from: function to obtain the field length
        @type length_from: C{callable}

        @param get_class: function to obtain the class
        @type get_class: C{callable}

        @param evaluators: evaluators
        @type evaluators: C{list} of C{callable}
        """
        StrLenField.__init__(self, name, default, length_from=length_from)
        self.evaluators = evaluators or []
        self._get_class = get_class

    def get_class(self, pkt):
        # Run the evaluators on the actual packet
        values = [evaluator(pkt) for evaluator in self.evaluators]
        # Return the class using the function provided
        return self._get_class(pkt, *values)

    def i2m(self, pkt, i):
        cls = self.get_class(pkt)
        if cls is not None:
            return str(i)
        else:
            return StrLenField.i2m(self, pkt, i)

    def m2i(self, pkt, m):
        cls = self.get_class(pkt)
        if cls is not None:
            return cls(m)
        else:
            return StrLenField.m2i(self, pkt, m)


class StrNullFixedLenField(StrFixedLenField):
    """Packet field that has a fixed length and is null-terminated.
    """
    def __init__(self, name, default, length=None, length_from=None, max_length=None):
        self.max_length = max_length or 200
        StrFixedLenField.__init__(self, name, default, length=length, length_from=length_from)

    def i2repr(self, pkt, v):
        if type(v) is str:
            v = v.rstrip("\0")
        return repr(v)

    def getfield(self, pkt, s):
        l = self.length_from(pkt)
        return s[l + 1:], self.m2i(pkt, s[:l])

    def addfield(self, pkt, s, val):
        l = self.length_from(pkt)
        return s + struct.pack("%is" % l, self.i2m(pkt, val)) + "\x00"

    def randval(self):
        try:
            l = self.length_from(None)
        except:
            l = RandTermString(RandNum(0, self.max_length), "\x00")
        return RandBin(l)


class StrFixedLenPaddedField(StrFixedLenField):
    """Packet field that has a fixed length and is padded with a
    given character.
    """

    def __init__(self, name, default, length=None, length_from=None, padd=" "):
        StrFixedLenField.__init__(self, name, default, length, length_from)
        self.padd = padd

    def getfield(self, pkt, s):
        l = self.length_from(pkt)
        return s[l:], self.m2i(pkt, s[:l])

    def addfield(self, pkt, s, val):
        l = self.length_from(pkt)
        val += self.padd * l
        return StrFixedLenField.addfield(self, pkt, s, val)


class StrNullFixedLenPaddedField(StrFixedLenField):
    """Packet field that has a fixed length and is padded with a
    given character and null terminated.
    """

    def __init__(self, name, default, length=None, length_from=None, padd=" "):
        StrFixedLenField.__init__(self, name, default, length, length_from)
        self.padd = padd

    def getfield(self, pkt, s):
        l = self.length_from(pkt)
        lz = s.find("\x00")
        if lz < l:
            return s[l + 1:], self.m2i(pkt, s[:lz])
        return s[l + 1:], self.m2i(pkt, s[:l])

    def addfield(self, pkt, s, val):
        l = self.length_from(pkt)
        val += self.padd * l
        return StrFixedLenField.addfield(self, pkt, s, val)


class IntToStrField(Field):
    """Custom field from int to str values, with a variable length
    """

    def __init__(self, name, default, length=11):
        """Initialize the field with a variable length. The 'machine'
        representation is a string field and the 'internal' repr.
        is a numeric value.
        """
        Field.__init__(self, name, default, "%ds" % length)
        # Stores the length of the field
        self.length = length
        # Stores the conversion format between representations
        self.format = "%" + "%d" % length + "d"

    def m2i(self, pkt, x):
        return str(x)

    def i2m(self, pkt, x):
        return self.format % int(x)

    def i2count(self, pkt, x):
        return x


class SignedShortField(Field):
    def __init__(self, name, default):
        Field.__init__(self, name, default, "h")


class StrEncodedPaddedField(StrField):
    def __init__(self, name, default, encoding="utf-16", padd="\x0c",
                 fmt="H", remain=0):
        StrField.__init__(self, name, default, fmt, remain)
        self.encoding = encoding
        self.padd = padd

    def h2i(self, pkt, x):
        if x:
            x = x.encode(self.encoding)
        return x

    def i2h(self, pkt, x):
        if x:
            x = x.decode(self.encoding)
        return x

    def addfield(self, pkt, s, val):
        return s + self.i2m(pkt, val) + self.padd

    def getfield(self, pkt, s):
        l = s.find(self.padd)
        if l < 0:
            return "", s
        return s[l + 1:], self.m2i(pkt, s[:l])


class Worker(Thread):
    """Thread Worker

    It runs a function into a new thread.
    """

    def __init__(self, decoder, function):
        Thread.__init__(self)
        self.decoder = decoder
        self.function = function
        self.stopped = Event()

    def run(self):
        while not self.stopped.is_set():
            self.function()

    def stop(self):
        self.stopped.set()


# Base Console
class BaseConsole (Cmd, object):

    # Initialization
    def __init__(self, options):
        super(BaseConsole, self).__init__()
        self.prompt = "pysap's console>"
        self._hist = []
        self.options = options
        self.runtimeoptions = {}

    # Console Command definitions
    def do_history(self, args):
        """Show commands history."""
        self._print(self._hist)

    def do_exit(self, args):
        """Exit console."""
        return -1

    def do_help(self, args):
        """Show help."""
        super(BaseConsole, self).do_help(args)

    def do_options(self, args):
        """Show/set options."""
        # If no args, print the current options
        if not args:
            options_dict = vars(self.options)
            self._print("Configuration options:")
            for option, value in list(options_dict.items()):
                self._print("[" + option + "] = " + str(value))

            self._print("Run-time options:")
            for option in list(self.runtimeoptions.keys()):
                self._print("[" + option + "] = " + str(self.runtimeoptions[option]))
        # Set a run-time option
        else:
            args = args.split(" ", 1)
            if len(args) == 2:
                option = args.pop(0)  # Extract the option
                value = args.pop(0)
                if option in list(self.runtimeoptions.keys()):
                    self.runtimeoptions[option] = value
                else:
                    self._error("Invalid or non run-time option.")
            else:
                self._error("Invalid number of parameters.")
                self.do_help("options")

    def complete_options(self, text, line, begidx, endidx):
        if not text:     # Complete list of run-time options
            return list(self.runtimeoptions.keys())
        else:        # Options starting with text
            return [option for option in list(self.runtimeoptions.keys()) if option.startswith(text)]

    def do_script(self, args):
        """Runs a script file."""
        if not args:
            self._error("Invalid number of parameters.")
            self.do_help("script")
        else:
            try:
                scriptfile = open(args, 'r')
                for line in scriptfile:
                    if not line[0] in ["\n", "#"]:
                        self.precmd(line)
                        self.onecmd(line)
            except IOError:
                self._error("Error reading script file.")

    # Console output methods

    def _tabulate(self, table, **args):
        if tabulate:
            tabular = tabulate(table, args)
            self._print(tabular)
        else:
            self._print("\n".join("\t|".join(line) for line in table))

    def _print(self, string=""):
        print(str(string))
        self._log(string)

    def _log(self, string=""):
        if self.options.consolelog:  # To console file if specified
            self.options.consolelog.write(str(string) + "\n")

    def _debug(self, string=""):
        if self.options.verbose:
            self._print(string)

    def _error(self, string):
        self._print("Error: " + string)  # To console if log file specified

    # Override of cmd.Cmd methods and hooks

    def preloop(self):
        super(BaseConsole, self).preloop()
        self._hist = []
        self._debug("Entering console " + self.intro)
        self._log(self.ruler * 24)
        self._log(self.intro)

    def postloop(self):
        super(BaseConsole, self).postloop()
        self._debug("Exiting console " + self.intro)
        self._log(self.ruler * 24)
        self._log()

    def precmd(self, line):
        self._hist += [line.strip()]
        self._debug("Executing console command: " + line)
        self._log(self.prompt + str(line))
        return line

    def postcmd(self, stop, line):
        return super(BaseConsole, self).postcmd(stop, line)

    def emptyline(self):
        pass
