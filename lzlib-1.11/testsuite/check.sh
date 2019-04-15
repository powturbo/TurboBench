#! /bin/sh
# check script for Lzlib - Compression library for the lzip format
# Copyright (C) 2009-2019 Antonio Diaz Diaz.
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

[ -e "${LZIP}" ] 2> /dev/null ||
	{
	echo "$0: a POSIX shell is required to run the tests"
	echo "Try bash -c \"$0 $1 $2\""
	exit 1
	}

if [ -d tmp ] ; then rm -rf tmp ; fi
mkdir tmp
cd "${objdir}"/tmp || framework_failure

cat "${testdir}"/test.txt > in || framework_failure
in_lz="${testdir}"/test.txt.lz
test2="${testdir}"/test2.txt
fail=0
test_failed() { fail=1 ; printf " $1" ; [ -z "$2" ] || printf "($2)" ; }

printf "testing lzlib-%s..." "$2"

"${LZIP}" -fkqm4 in
[ $? = 1 ] || test_failed $LINENO
[ ! -e in.lz ] || test_failed $LINENO
"${LZIP}" -fkqm274 in
[ $? = 1 ] || test_failed $LINENO
[ ! -e in.lz ] || test_failed $LINENO
for i in bad_size -1 0 4095 513MiB 1G 1T 1P 1E 1Z 1Y 10KB ; do
	"${LZIP}" -fkqs $i in
	[ $? = 1 ] || test_failed $LINENO $i
	[ ! -e in.lz ] || test_failed $LINENO $i
done
"${LZIP}" -tq in
[ $? = 2 ] || test_failed $LINENO
"${LZIP}" -tq < in
[ $? = 2 ] || test_failed $LINENO
"${LZIP}" -cdq in
[ $? = 2 ] || test_failed $LINENO
"${LZIP}" -cdq < in
[ $? = 2 ] || test_failed $LINENO
"${LZIP}" -dq -o in < "${in_lz}"
[ $? = 1 ] || test_failed $LINENO
# these are for code coverage
"${LZIP}" -cdt "${in_lz}" > out 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" -t -- nx_file 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" --help > /dev/null || test_failed $LINENO
"${LZIP}" -n1 -V > /dev/null || test_failed $LINENO
"${LZIP}" -m 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" -z 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" --bad_option 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" --t 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" --test=2 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" --output= 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" --output 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
printf "LZIP\001-.............................." | "${LZIP}" -t 2> /dev/null
printf "LZIP\002-.............................." | "${LZIP}" -t 2> /dev/null
printf "LZIP\001+.............................." | "${LZIP}" -t 2> /dev/null

printf "\ntesting decompression..."

"${LZIP}" -t "${in_lz}" || test_failed $LINENO
"${LZIP}" -cd "${in_lz}" > copy || test_failed $LINENO
cmp in copy || test_failed $LINENO

"${LZIP}" -t "${testdir}"/test_sync.lz || test_failed $LINENO
"${LZIP}" -cd "${testdir}"/test_sync.lz > copy || test_failed $LINENO
cmp in copy || test_failed $LINENO

rm -f copy || framework_failure
cat "${in_lz}" > copy.lz || framework_failure
"${LZIP}" -dk copy.lz || test_failed $LINENO
cmp in copy || test_failed $LINENO
printf "to be overwritten" > copy || framework_failure
"${LZIP}" -d copy.lz 2> /dev/null
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" -df copy.lz || test_failed $LINENO
[ ! -e copy.lz ] || test_failed $LINENO
cmp in copy || test_failed $LINENO

rm -f copy || framework_failure
cat "${in_lz}" > copy.lz || framework_failure
"${LZIP}" -d -S100k copy.lz || test_failed $LINENO	# ignore -S
[ ! -e copy.lz ] || test_failed $LINENO
cmp in copy || test_failed $LINENO

printf "to be overwritten" > copy || framework_failure
"${LZIP}" -df -o copy < "${in_lz}" || test_failed $LINENO
cmp in copy || test_failed $LINENO

rm -f copy || framework_failure
"${LZIP}" -s16 < in > anyothername || test_failed $LINENO
"${LZIP}" -dv --output copy - anyothername - < "${in_lz}" 2> /dev/null ||
	test_failed $LINENO
cmp in copy || test_failed $LINENO
cmp in anyothername.out || test_failed $LINENO
rm -f copy anyothername.out || framework_failure

"${LZIP}" -tq in "${in_lz}"
[ $? = 2 ] || test_failed $LINENO
"${LZIP}" -tq nx_file.lz "${in_lz}"
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" -cdq in "${in_lz}" > copy
[ $? = 2 ] || test_failed $LINENO
cat copy in | cmp in - || test_failed $LINENO
"${LZIP}" -cdq nx_file.lz "${in_lz}" > copy
[ $? = 1 ] || test_failed $LINENO
cmp in copy || test_failed $LINENO
rm -f copy || framework_failure
cat "${in_lz}" > copy.lz || framework_failure
for i in 1 2 3 4 5 6 7 ; do
	printf "g" >> copy.lz || framework_failure
	"${LZIP}" -atvvvv copy.lz "${in_lz}" 2> /dev/null
	[ $? = 2 ] || test_failed $LINENO $i
done
"${LZIP}" -dq in copy.lz
[ $? = 2 ] || test_failed $LINENO
[ -e copy.lz ] || test_failed $LINENO
[ ! -e copy ] || test_failed $LINENO
[ ! -e in.out ] || test_failed $LINENO
"${LZIP}" -dq nx_file.lz copy.lz
[ $? = 1 ] || test_failed $LINENO
[ ! -e copy.lz ] || test_failed $LINENO
[ ! -e nx_file ] || test_failed $LINENO
cmp in copy || test_failed $LINENO

cat in in > in2 || framework_failure
cat "${in_lz}" "${in_lz}" > in2.lz || framework_failure
"${LZIP}" -t in2.lz || test_failed $LINENO
"${LZIP}" -cd in2.lz > copy2 || test_failed $LINENO
cmp in2 copy2 || test_failed $LINENO

"${LZIP}" -s16 --output=copy2.lz < in2 || test_failed $LINENO
"${LZIP}" -t copy2.lz || test_failed $LINENO
"${LZIP}" -cd copy2.lz > copy2 || test_failed $LINENO
cmp in2 copy2 || test_failed $LINENO

printf "\ngarbage" >> copy2.lz || framework_failure
"${LZIP}" -tvvvv copy2.lz 2> /dev/null || test_failed $LINENO
rm -f copy2 || framework_failure
"${LZIP}" -atq copy2.lz
[ $? = 2 ] || test_failed $LINENO
"${LZIP}" -atq < copy2.lz
[ $? = 2 ] || test_failed $LINENO
"${LZIP}" -adkq copy2.lz
[ $? = 2 ] || test_failed $LINENO
[ ! -e copy2 ] || test_failed $LINENO
"${LZIP}" -adkq -o copy2 < copy2.lz
[ $? = 2 ] || test_failed $LINENO
[ ! -e copy2 ] || test_failed $LINENO
printf "to be overwritten" > copy2 || framework_failure
"${LZIP}" -df copy2.lz || test_failed $LINENO
cmp in2 copy2 || test_failed $LINENO
rm -f in2 copy2 || framework_failure

printf "\ntesting   compression..."

"${LZIP}" -cf "${in_lz}" > out 2> /dev/null	# /dev/null is a tty on OS/2
[ $? = 1 ] || test_failed $LINENO
"${LZIP}" -cFvvm36 -s16 "${in_lz}" > out 2> /dev/null || test_failed $LINENO
"${LZIP}" -cd out | "${LZIP}" -d > copy || test_failed $LINENO
cmp in copy || test_failed $LINENO

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -k -$i -s16 in || test_failed $LINENO $i
	mv -f in.lz copy.lz || test_failed $LINENO $i
	printf "garbage" >> copy.lz || framework_failure
	"${LZIP}" -df copy.lz || test_failed $LINENO $i
	cmp in copy || test_failed $LINENO $i
done

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -c -$i -s16 in > out || test_failed $LINENO $i
	printf "g" >> out || framework_failure
	"${LZIP}" -cd out > copy || test_failed $LINENO $i
	cmp in copy || test_failed $LINENO $i
done

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -$i -s16 < in > out || test_failed $LINENO $i
	"${LZIP}" -d < out > copy || test_failed $LINENO $i
	cmp in copy || test_failed $LINENO $i
done

for i in s4Ki 0 1 2 3 4 5 6 7 8 9 ; do
	"${LZIP}" -f -$i -s16 -o out < in || test_failed $LINENO $i
	"${LZIP}" -df -o copy < out.lz || test_failed $LINENO $i
	cmp in copy || test_failed $LINENO $i
done
rm -f out.lz || framework_failure

cat in in in in in in in in > in8 || framework_failure
"${LZIP}" -1s12 -S100k in8 || test_failed $LINENO
"${LZIP}" -t in800001.lz in800002.lz || test_failed $LINENO
"${LZIP}" -cd in800001.lz in800002.lz | cmp in8 - || test_failed $LINENO
rm -f in800001.lz in800002.lz || framework_failure
"${LZIP}" -1s12 -S100k -o out.lz < in8 || test_failed $LINENO
"${LZIP}" -t out.lz00001.lz out.lz00002.lz || test_failed $LINENO
"${LZIP}" -cd out.lz00001.lz out.lz00002.lz | cmp in8 - || test_failed $LINENO
rm -f out.lz00001.lz out.lz00002.lz || framework_failure
"${LZIP}" -1ks4Ki -b100000 in8 || test_failed $LINENO
"${LZIP}" -t in8.lz || test_failed $LINENO
"${LZIP}" -cd in8.lz | cmp in8 - || test_failed $LINENO
rm -f in8 || framework_failure
"${LZIP}" -0 -S100k -o out < in8.lz || test_failed $LINENO
"${LZIP}" -t out00001.lz out00002.lz || test_failed $LINENO
"${LZIP}" -cd out00001.lz out00002.lz | cmp in8.lz - || test_failed $LINENO
rm -f out00001.lz || framework_failure
"${LZIP}" -1 -S100k -o out < in8.lz || test_failed $LINENO
"${LZIP}" -t out00001.lz out00002.lz || test_failed $LINENO
"${LZIP}" -cd out00001.lz out00002.lz | cmp in8.lz - || test_failed $LINENO
rm -f out00001.lz out00002.lz || framework_failure
"${LZIP}" -0 -F -S100k in8.lz || test_failed $LINENO
"${LZIP}" -t in8.lz00001.lz in8.lz00002.lz || test_failed $LINENO
"${LZIP}" -cd in8.lz00001.lz in8.lz00002.lz | cmp in8.lz - || test_failed $LINENO
rm -f in8.lz00001.lz in8.lz00002.lz || framework_failure
"${LZIP}" -0kF -b100k in8.lz || test_failed $LINENO
"${LZIP}" -t in8.lz.lz || test_failed $LINENO
"${LZIP}" -cd in8.lz.lz | cmp in8.lz - || test_failed $LINENO
rm -f in8.lz in8.lz.lz || framework_failure

"${BBEXAMPLE}" in || test_failed $LINENO
"${BBEXAMPLE}" out || test_failed $LINENO
"${BBEXAMPLE}" ${test2} || test_failed $LINENO

"${LZCHECK}" in || test_failed $LINENO
"${LZCHECK}" out || test_failed $LINENO
"${LZCHECK}" ${test2} || test_failed $LINENO

printf "\ntesting bad input..."

headers='LZIp LZiP LZip LzIP LzIp LziP lZIP lZIp lZiP lzIP'
body='\001\014\000\203\377\373\377\377\300\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000$\000\000\000\000\000\000\000'
cat "${in_lz}" > int.lz
printf "LZIP${body}" >> int.lz
if "${LZIP}" -tq int.lz ; then
	for header in ${headers} ; do
		printf "${header}${body}" > int.lz	# first member
		"${LZIP}" -tq int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -tq < int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -cdq int.lz > /dev/null
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -tq --loose-trailing int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -tq --loose-trailing < int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -cdq --loose-trailing int.lz > /dev/null
		[ $? = 2 ] || test_failed $LINENO ${header}
		cat "${in_lz}" > int.lz
		printf "${header}${body}" >> int.lz	# trailing data
		"${LZIP}" -tq int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -tq < int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -cdq int.lz > /dev/null
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -t --loose-trailing int.lz ||
			test_failed $LINENO ${header}
		"${LZIP}" -t --loose-trailing < int.lz ||
			test_failed $LINENO ${header}
		"${LZIP}" -cd --loose-trailing int.lz > /dev/null ||
			test_failed $LINENO ${header}
		"${LZIP}" -tq --loose-trailing --trailing-error int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -tq --loose-trailing --trailing-error < int.lz
		[ $? = 2 ] || test_failed $LINENO ${header}
		"${LZIP}" -cdq --loose-trailing --trailing-error int.lz > /dev/null
		[ $? = 2 ] || test_failed $LINENO ${header}
	done
else
	printf "\nwarning: skipping header test: 'printf' does not work on your system."
fi
rm -f int.lz || framework_failure

cat "${in_lz}" "${in_lz}" "${in_lz}" > in3.lz || framework_failure
if dd if=in3.lz of=trunc.lz bs=14752 count=1 2> /dev/null &&
   [ -e trunc.lz ] && cmp in2.lz trunc.lz > /dev/null 2>&1 ; then
	for i in 6 20 14734 14753 14754 14755 14756 14757 14758 ; do
		dd if=in3.lz of=trunc.lz bs=$i count=1 2> /dev/null
		"${LZIP}" -tq trunc.lz
		[ $? = 2 ] || test_failed $LINENO $i
		"${LZIP}" -tq < trunc.lz
		[ $? = 2 ] || test_failed $LINENO $i
		"${LZIP}" -cdq trunc.lz > out
		[ $? = 2 ] || test_failed $LINENO $i
		"${LZIP}" -dq < trunc.lz > out
		[ $? = 2 ] || test_failed $LINENO $i
	done
else
	printf "\nwarning: skipping truncation test: 'dd' does not work on your system."
fi
rm -f in2.lz in3.lz trunc.lz out || framework_failure

cat "${in_lz}" > ingin.lz || framework_failure
printf "g" >> ingin.lz || framework_failure
cat "${in_lz}" >> ingin.lz || framework_failure
"${LZIP}" -t ingin.lz || test_failed $LINENO
"${LZIP}" -cd ingin.lz > copy || test_failed $LINENO
cmp in copy || test_failed $LINENO
"${LZIP}" -t < ingin.lz || test_failed $LINENO
"${LZIP}" -d < ingin.lz > copy || test_failed $LINENO
cmp in copy || test_failed $LINENO
rm -f copy ingin.lz || framework_failure

echo
if [ ${fail} = 0 ] ; then
	echo "tests completed successfully."
	cd "${objdir}" && rm -r tmp
else
	echo "tests failed."
fi
exit ${fail}
