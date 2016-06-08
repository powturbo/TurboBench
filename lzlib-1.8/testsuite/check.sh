#! /bin/sh
# check script for Lzlib - A compression library for lzip files
# Copyright (C) 2009-2016 Antonio Diaz Diaz.
#
# This script is free software: you have unlimited permission
# to copy, distribute and modify it.

LC_ALL=C
export LC_ALL
objdir=`pwd`
testdir=`cd "$1" ; pwd`
LZIP="${objdir}"/minilzip
BBEXAMPLE="${objdir}"/bbexample
LZCHECK="${objdir}"/lzcheck
framework_failure() { echo "failure in testing framework" ; exit 1 ; }

if [ ! -f "${LZIP}" ] || [ ! -x "${LZIP}" ] ; then
	echo "${LZIP}: cannot execute"
	exit 1
fi

if [ -e "${LZIP}" ] 2> /dev/null ; then true
else
	echo "$0: a POSIX shell is required to run the tests"
	echo "Try bash -c \"$0 $1 $2\""
	exit 1
fi

if [ -d tmp ] ; then rm -rf tmp ; fi
mkdir tmp
cd "${objdir}"/tmp || framework_failure

cat "${testdir}"/test.txt > in || framework_failure
in_lz="${testdir}"/test.txt.lz
test2="${testdir}"/test2.txt
fail=0

printf "testing lzlib-%s..." "$2"

"${LZIP}" -fkqm4 in
if [ $? = 1 ] && [ ! -e in.lz ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -fkqm274 in
if [ $? = 1 ] && [ ! -e in.lz ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -fkqs-1 in
if [ $? = 1 ] && [ ! -e in.lz ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -fkqs0 in
if [ $? = 1 ] && [ ! -e in.lz ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -fkqs4095 in
if [ $? = 1 ] && [ ! -e in.lz ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -fkqs513MiB in
if [ $? = 1 ] && [ ! -e in.lz ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -tq in
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -tq < in
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -cdq in
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -cdq < in
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
dd if="${in_lz}" bs=1 count=6 2> /dev/null | "${LZIP}" -tq
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
dd if="${in_lz}" bs=1 count=20 2> /dev/null | "${LZIP}" -tq
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi

printf "\ntesting decompression..."

"${LZIP}" -t "${in_lz}"
if [ $? = 0 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -cd "${in_lz}" > copy || fail=1
cmp in copy || fail=1
printf .

"${LZIP}" -t "${testdir}"/test_sync.lz
if [ $? = 0 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -cd "${testdir}"/test_sync.lz > copy || fail=1
cmp in copy || fail=1
printf .

rm -f copy
cat "${in_lz}" > copy.lz || framework_failure
"${LZIP}" -dk copy.lz || fail=1
cmp in copy || fail=1
printf "to be overwritten" > copy || framework_failure
"${LZIP}" -dq copy.lz
if [ $? = 1 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -df copy.lz
if [ $? = 0 ] && [ ! -e copy.lz ] && cmp in copy ; then
	printf . ; else printf - ; fail=1 ; fi

printf "to be overwritten" > copy || framework_failure
"${LZIP}" -df -o copy < "${in_lz}" || fail=1
cmp in copy || fail=1
printf .

rm -f copy
"${LZIP}" -s16 < in > anyothername || fail=1
"${LZIP}" -d -o copy - anyothername - < "${in_lz}"
if [ $? = 0 ] && cmp in copy && cmp in anyothername.out ; then
	printf . ; else printf - ; fail=1 ; fi
rm -f copy anyothername.out

"${LZIP}" -tq in "${in_lz}"
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -tq foo.lz "${in_lz}"
if [ $? = 1 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -cdq in "${in_lz}" > copy
if [ $? = 2 ] && cat copy in | cmp in - ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -cdq foo.lz "${in_lz}" > copy
if [ $? = 1 ] && cmp in copy ; then printf . ; else printf - ; fail=1 ; fi
rm -f copy
cat "${in_lz}" > copy.lz || framework_failure
"${LZIP}" -dq in copy.lz
if [ $? = 2 ] && [ -e copy.lz ] && [ ! -e copy ] && [ ! -e in.out ] ; then
	printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -dq foo.lz copy.lz
if [ $? = 1 ] && [ ! -e copy.lz ] && [ ! -e foo ] && cmp in copy ; then
	printf . ; else printf - ; fail=1 ; fi

cat in in > in2 || framework_failure
"${LZIP}" -s16 -o copy2 < in2 || fail=1
"${LZIP}" -t copy2.lz || fail=1
"${LZIP}" -cd copy2.lz > copy2 || fail=1
cmp in2 copy2 || fail=1
printf .

printf "garbage" >> copy2.lz || framework_failure
rm -f copy2
"${LZIP}" -atq copy2.lz
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -atq < copy2.lz
if [ $? = 2 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -adkq copy2.lz
if [ $? = 2 ] && [ ! -e copy2 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -adkq -o copy2 < copy2.lz
if [ $? = 2 ] && [ ! -e copy2 ] ; then printf . ; else printf - ; fail=1 ; fi
printf "to be overwritten" > copy2 || framework_failure
"${LZIP}" -df copy2.lz || fail=1
cmp in2 copy2 || fail=1
printf .

printf "\ntesting   compression..."

"${LZIP}" -cfq "${in_lz}" > out			# /dev/null is a tty on OS/2
if [ $? = 1 ] ; then printf . ; else printf - ; fail=1 ; fi
"${LZIP}" -cF -s16 "${in_lz}" > out || fail=1
"${LZIP}" -cd out | "${LZIP}" -d > copy || fail=1
cmp in copy || fail=1
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -k -$i -s16 in || fail=1
	mv -f in.lz copy.lz || fail=1
	printf "garbage" >> copy.lz || fail=1
	"${LZIP}" -df copy.lz || fail=1
	cmp in copy || fail=1
done
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -c -$i -s16 in > out || fail=1
	printf "g" >> out || fail=1
	"${LZIP}" -cd out > copy || fail=1
	cmp in copy || fail=1
done
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -$i -s16 < in > out || fail=1
	"${LZIP}" -d < out > copy || fail=1
	cmp in copy || fail=1
done
printf .

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -f -$i -s16 -o out < in || fail=1
	"${LZIP}" -df -o copy < out.lz || fail=1
	cmp in copy || fail=1
done
printf .

"${BBEXAMPLE}" in || fail=1
printf .
"${BBEXAMPLE}" out || fail=1
printf .
"${BBEXAMPLE}" ${test2} || fail=1
printf .

"${LZCHECK}" in || fail=1
printf .
"${LZCHECK}" out || fail=1
printf .
"${LZCHECK}" ${test2} || fail=1
printf .

echo
if [ ${fail} = 0 ] ; then
	echo "tests completed successfully."
	cd "${objdir}" && rm -r tmp
else
	echo "tests failed."
fi
exit ${fail}
