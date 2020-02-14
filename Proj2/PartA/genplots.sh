#!/usr/local/bin/bash

addcsv=lab2_add.csv

threads=2
iters=100
while [[ threads -le 8 ]]; do
    while [[ iters -le 100000 ]]; do
        ./gencsv 10 $threads $iters >> $addcsv
        ./gencsvy 10 $threads $iters >> $addcsv

        let "iters *= 10"
    done
    iters=100
    let "threads += 2"
done
