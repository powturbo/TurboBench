from distutils.core import setup, Extension

quicklz = Extension('quicklz', sources = ['quicklzpy.c'])

setup (name = 'quicklz',
        version = '1.0',
        description = 'This is a python module to access the QuickLZ compression algorithm.',
        ext_modules = [quicklz])
