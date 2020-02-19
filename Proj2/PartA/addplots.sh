#!/usr/local/bin/bash

addcsv=lab2_add.csv

threads1=(2 4 8 12)
iters1=(10 20 40 80 100 1000 10000 100000)

echo "Starting round 1, yield with no sync"
for t in ${threads1[@]}; do
    for i in ${iters1[@]}; do
        echo "Starting iteration with $t threads, $i iters..."
        ./gencsvy 3 $t $i >> $addcsv
    done
done
echo "Finished round 1"

threads2=(2 8)
iters2=(100 1000 10000 100000)

echo "Starting round 2, yield and no yield w/ no sync"
for t in ${threads2[@]}; do
    for i in ${iters2[@]}; do
        echo "Starting iteration with $t threads, $i iters..."
        ./gencsv  3 $t $i >> $addcsv
        ./gencsvy 3 $t $i >> $addcsv
    done
done
echo "Finished round 2"

iters3=(100 1000 10000 100000)

echo "Starting round 3, single-thread no sync"
for i in ${iters3[@]}; do
    echo "Starting iteration with $i iters..."
    ./gencsv 3 1 $i >> $addcsv
done
echo "Finished round 3"

threads4=(2 4 8 12)

echo "Starting round 4, yield with all sync options"
for t in ${threads4[@]}; do
    echo "Starting iteration with $t threads..."
    ./gencsvy 3 $t 10000   >> $addcsv
    ./gencsvy 3 $t 10000 m >> $addcsv
    ./gencsvy 3 $t 10000 c >> $addcsv
    ./gencsvy 3 $t 1000  s >> $addcsv
done
echo "Finished round 4"

threads5=(1 2 4 8 12)

echo "Starting round 5, no yield w/ all sync options"
for t in ${threads5[@]}; do
    echo "Starting iteration with $t threads..."
    ./gencsv 3 $t 10000   >> $addcsv
    ./gencsv 3 $t 10000 m >> $addcsv
    ./gencsv 3 $t 10000 s >> $addcsv
    ./gencsv 3 $t 10000 c >> $addcsv
done
echo "Finished round 5"
