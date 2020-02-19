@echo off

del *.exe

set incs=-DSTRICT -DNDEBUG -DWIN32

set opts=-fwhole-program -fpermissive -fstrict-aliasing -fomit-frame-pointer -I../Lib3 -I../Lib ^
-fno-stack-protector -fno-stack-check -fno-check-new ^
-fno-exceptions -fno-rtti -fno-operator-names ^
-flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections
                                                                                                                  
rem -fprofile-use -fprofile-correction  -fwhole-program  -Ofast 

set gcc=C:\MinGW820x\bin\g++.exe -march=skylake -mavx2 -m64
set path=%gcc%\..\

del *.exe *.o

%gcc% -std=gnu++1z -O9 -s %incs% %opts% -static c_fil.cpp coder/model.cpp -o c_fil.exe

%gcc% -std=gnu++1z -O9 -s %incs% %opts% -static c_mem.cpp coder/model.cpp -o c_mem.exe

del *.o
