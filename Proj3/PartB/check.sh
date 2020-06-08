#!/bin/bash

test="tests/P3B-test_"
for (( i = 1; i <= 22; i++ )); do
    ./lab3b ${test}$i.csv > test.err
    diff -u test.err ${test}$i.err
done
