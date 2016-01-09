
BriefLZ - small fast Lempel-Ziv
===============================

Version 1.1.0

Copyright (c) 2002-2015 Joergen Ibsen

<http://www.ibsensoftware.com/>

[![Build Status](https://travis-ci.org/jibsen/brieflz.svg?branch=master)](https://travis-ci.org/jibsen/brieflz) [![Build Status](https://ci.appveyor.com/api/projects/status/l9vhammx8p8hkrqb?svg=true)](https://ci.appveyor.com/project/jibsen/brieflz) [![codecov.io](http://codecov.io/github/jibsen/brieflz/coverage.svg?branch=master)](http://codecov.io/github/jibsen/brieflz?branch=master)


About
-----

BriefLZ is a small and fast open source implementation of a Lempel-Ziv
style compression algorithm. The main focus is on speed, but the ratios
achieved are quite good compared to similar algorithms.


Usage
-----

The include file `brieflz.h` contains documentation in the form of [doxygen][]
comments. A configuration file is included, so you can simply run `doxygen`
to generate documentation in HTML format.

If you wish to compile BriefLZ on 16-bit systems, make sure to adjust the
constants `BLZ_HASH_BITS` and `BLOCK_SIZE`.

When using BriefLZ as a shared library (dll on Windows), define `BLZ_DLL`.
When building BriefLZ as a shared library, define both `BLZ_DLL`and
`BLZ_DLL_EXPORTS`.

The `example` folder contains a simple command-line program, `blzpack`, that
can compress and decompress a file using BriefLZ. For convenience, the example
comes with makefiles for GCC and MSVC.

BriefLZ uses [CMake][] to generate build systems. To create one for the tools
on your platform, and build BriefLZ, use something along the lines of:

~~~sh
mkdir build
cd build
cmake ..
cmake --build .
~~~

You can also simply compile the source files and link them into your project.
CMake just provides an easy way to build and test across various platforms and
toolsets.

[doxygen]: http://www.doxygen.org/
[CMake]: http://www.cmake.org/


License
-------

BriefLZ - small fast Lempel-Ziv

Copyright (c) 2002-2015 Joergen Ibsen

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must
     not claim that you wrote the original software. If you use this
     software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  3. This notice may not be removed or altered from any source
     distribution.
