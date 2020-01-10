#!/bin/bash

progName='lab0'
testStr1='Hello there!'
testStr2='General Kenobi!'

echo $testStr1 | ./$progName | grep -Eq "^${testStr1}$"
if [[ $? -ne 0 ]]; then
    echo 'Error: stdin not correctly redirected to stdout.' >&2
    exit 1
fi

echo $testStr2 | ./$progName | grep -Eq "^${testStr2}$"
if [[ $? -ne 0 ]]; then
    echo 'Error: stdin not correctly redirected to stdout.' >&2
fi

infile=$(mktemp)
echo $testStr1 > $infile
./$progName --input=$infile | grep -Eq "^${testStr1}$"
if [[ $? -ne 0 ]]; then
    echo 'Error: file contents not correctly copied to stdout.' >&2
    rm -rf $infile
    exit 1
fi

echo $testStr2 >> $infile
./$progName --input=$infile | grep -Eq "^${testStr2}$"
if [[ $? -ne 0 ]]; then
    echo 'Error: file contents not correctly copied to stdout. (2nd case)' >&2
    rm -rf $infile
    exit 1
fi

outfile=$(mktemp)
echo $testStr1 | ./$progName --output=$outfile
cat $outfile | grep -Eq "^${testStr1}$"
if [[ $? -ne 0 ]]; then
    echo 'Error: input not correctly copied to file.' >&2
    rm -rf $infile $outfile
    exit 1
fi

echo $testStr2 | ./$progName --output=$outfile
cat $outfile | grep -Eq "^${testStr2}$"
if [[ $? -ne 0 ]]; then
    echo 'Error: input not correctly copied to file.' >&2
    rm -rf $infile $outfile
    exit 1
fi

# now put it all together!
echo $testStr1 > $infile
./$progName --input=$infile --output=$outfile
if ! diff -u $infile $outfile; then
    echo 'Error: outfile is not the same as infile.' >&2
    rm -rf $infile $outfile
    exit 1
fi

# segfault and catch tests
echo '--start segfaults--'
./$progName --segfault
./$progName -i $infile -o $outfile -s
./$progName -o $outfile -s

echo '--end segfaults--'
printf '\n'

./$progName -s --catch 2> /dev/null
if [[ $? -ne 4 ]]; then
    echo 'Signal handler not registered correctly.' >&2
    rm -rf $infile $outfile
    exit 1
fi
./$progName --input=$infile -sc 2> /dev/null
if [[ $? -ne 4 ]]; then
    echo 'Signal handler not registered correctly.' >&2
    rm -rf $infile $outfile
    exit 1
fi
./$progName --input=$infile --output=$outfile -sc 2> /dev/null
if [[ $? -ne 4 ]]; then
    echo 'Signal handler not registered correctly.' >&2
    rm -rf $infile $outfile
    exit 1
fi



rm -rf $infile
rm -rf $outfile

echo 'All smoke tests passed!'
