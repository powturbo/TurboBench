@echo off

del 1 2
c_fil c .\book1 1
c_fil d 1 2
md5sum 2 ./book1

del 1 2
c_mem c .\book1 1
c_mem d 1 2
md5sum 2 ./book1
