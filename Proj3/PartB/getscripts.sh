#!/bin/bash

mkdir tests
cd tests

for (( i = 1; i <= 22; i++ )); do
    wget http://web.cs.ucla.edu/classes/cs111/Samples/P3B-test_$i.csv
    echo "Downloaded P3B-test_$i.csv"
    wget http://web.cs.ucla.edu/classes/cs111/Samples/P3B-test_$i.err
    echo "Downloaded P3B-test_$i.err"
done
