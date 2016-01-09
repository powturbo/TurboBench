#!/bin/sh

# quick validation test

what="all"

make

if [[ $what = "all" ]]; then
  echo "simple test"
  for s in 2 5 10 30 100 200 256; do
    ./test 200001 -s $s -buck | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -rev  | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -mod  | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -pack | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -w    | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -w2   | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -w4   | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -a    | grep "errors" | grep -v "#0 "
    ./test 200001 -s $s -a2   | grep "errors" | grep -v "#0 "
  done
fi

echo "corner case test #1"
for n in `seq 0 33`; do
  ./test $n -w  | grep "errors" | grep -v "#0 "
  ./test $n -w2 | grep "errors" | grep -v "#0 "
  ./test $n -w4 | grep "errors" | grep -v "#0 "
  ./test $n -a  | grep "errors" | grep -v "#0 "
  ./test $n -a2 | grep "errors" | grep -v "#0 "
done

echo "corner case test #2"
for n in `seq 8189 8201`; do
#  echo "*** n=$n ***"
  ./test $n -w  | grep "errors" | grep -v "#0 "
  ./test $n -w2 | grep "errors" | grep -v "#0 "
  ./test $n -w4 | grep "errors" | grep -v "#0 "
  ./test $n -a  | grep "errors" | grep -v "#0 "
  ./test $n -a2 | grep "errors" | grep -v "#0 "
done
