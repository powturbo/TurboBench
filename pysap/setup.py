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
from sys import exit
from glob import glob
from subprocess import call
from setuptools import setup, Extension, Command
# Custom imports
import pysap


class DocumentationCommand(Command):
    """Custom command for building the documentation with Sphinx.
    """

    description = "Builds the documentation using Sphinx"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        """Runs Sphinx
        """
        exit(call("cd docs && make html", shell=True))


class PreExecuteNotebooksCommand(Command):
    """Custom command for pre-executing Jupyther notebooks included in the documentation.
    """

    description = "Pre-executes Jupyther notebooks included in the documentation"
    user_options = [
        ('notebooks=', 'n', "patterns to match (i.e. 'protocols/SAPDiag*')"),
    ]

    def initialize_options(self):
        """Initialize options with default values."""
        self.notebooks = None

    def finalize_options(self):
        """Check and expand provided values."""
        base_path = "docs/"
        if self.notebooks:
            self.notebooks = glob(base_path + self.notebooks)
        else:
            self.notebooks = glob(base_path + "protocols/*.ipynb")
            self.notebooks.extend(glob(base_path + "fileformats/*.ipynb"))

    def run(self):
        """Pre executes notebooks."""
        status = 0
        for notebook in self.notebooks:
            status |= call("jupyter nbconvert --inplace --to notebook --execute {}".format(notebook), shell=True)
        exit(status)


sapcompress_macros = [
    # Enable this macro if you want some debugging information on the (de)compression functions
    # ('DEBUG', None),
    # Enable this macro if you want detailed debugging information (hexdumps) on the (de)compression functions
    # ('DEBUG_TRACE', None),
]


sapcompress = Extension('pysapcompress',
                        ['pysapcompress/pysapcompress.cpp',
                         'pysapcompress/vpa105CsObjInt.cpp',
                         'pysapcompress/vpa106cslzc.cpp',
                         'pysapcompress/vpa107cslzh.cpp',
                         'pysapcompress/vpa108csulzh.cpp'],
                        define_macros=sapcompress_macros)


with open("README.md", "r") as fh:
    long_description = fh.read()


setup(name=pysap.__title__,  # Package information
      version=pysap.__version__,
      author='Martin Gallo, OWASP CBAS Project',
      author_email='martin.gallo@gmail.com',
      description='Python library for crafting SAP\'s network protocols packets',
      long_description=long_description,
      long_description_content_type="text/markdown",
      url=pysap.__url__,
      download_url=pysap.__url__,
      license=pysap.__license__,
      classifiers=['Development Status :: 3 - Alpha',
                   'Intended Audience :: Developers',
                   'Intended Audience :: Information Technology',
                   'Intended Audience :: System Administrators',
                   'License :: OSI Approved :: GNU General Public License v2 or later (GPLv2+)',
                   'Programming Language :: Python',
                   'Programming Language :: C++',
                   'Topic :: Security'],
      # Packages list
      packages=['pysap', 'pysap.utils', 'pysap.utils.crypto'],
      provides=['pysapcompress', 'pysap'],

      # Extension module compilation
      ext_modules=[sapcompress],

      # Script files
      scripts=['bin/pysapcar', 'bin/pysapgenpse'],

      # Tests command
      test_suite='tests.test_suite',

      # Documentation commands
      cmdclass={'doc': DocumentationCommand,
                'notebooks': PreExecuteNotebooksCommand},

      # Requirements
      install_requires=open('requirements.txt').read().splitlines(),

      # Optional requirements for docs and some examples
      extras_require={"docs": open('requirements-docs.txt').read().splitlines(),
                      "examples": open('requirements-examples.txt').read().splitlines()},
      )
