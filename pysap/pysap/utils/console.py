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
from cmd import Cmd
# Optional imports
try:
    from tabulate import tabulate
except ImportError:
    tabulate = None


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
            self._print("\n".join("\t| ".join([str(col).strip() for col in line]).expandtabs(20) for line in table))

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
