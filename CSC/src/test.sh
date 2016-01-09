#!/bin/sh


md5sum /disk2/D/tar6
echo "encoding..."
./test c /disk2/D/tar6 /tmp/_t.csc
echo "encode done"
echo "decoding..."
./test d /tmp/_t.csc /tmp/out
echo "decode done"
md5sum /tmp/out

