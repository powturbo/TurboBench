CPUTIME
=======
A simple multi-platform CPU user-mode chronometer

Branch | Linux | Windows
--- | --- | ---
Master | [![Build Status](https://travis-ci.org/centaurean/cputime.svg?branch=master)](https://travis-ci.org/centaurean/cputime) | [![Build status](https://ci.appveyor.com/api/projects/status/vfuxoenyd0y9iidx/branch/master?svg=true)](https://ci.appveyor.com/project/gpnuma/cputime/branch/master)

Build
-----
To build a static and dynamic library of CPUTime on Windows, Linux or Mac OSX,

1) Download [premake 5](http://premake.github.io/) and make it available in your path

2) Run the following from the command line

    cd build
    premake5 gmake
    make

or alternatively, on windows for example :

    premake5.exe vs2010

Quick start
-----------
```C
#include "cputime_api.h"

cputime_chronometer chrono;
cputime_chronometer_start(&chrono);
// Do something
double lap = cputime_chronometer_lap(&chrono);
// Do something else
double elapsed = cputime_chronometer_stop(&chrono);

// At any time, if needed, it's possible to get a timeval structure containing the total elapsed usermode time
struct timeval current_total_usertime = cputime_get_current_total_usertime();
