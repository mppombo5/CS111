#!/usr/local/bin/bash

addcsv=lab2_add.csv

threads1=(2 4 8 12)
iters1=(10 20 40 80 100 1000 10000 100000)

for t in ${threads1[@]}; do
    for i in ${iters1[@]}; do
        ./gencsvy 10 $t $i >> $addcsv
    done
done
echo "Finished round 1, yield tests no sync"

threads2=(2 8)
iters2=(100 1000 10000 100000)

for t in ${threads2[@]}; do
    for i in ${iters2[@]}; do
        ./gencsv  10 $t $i >> $addcsv
        ./gencsvy 10 $t $i >> $addcsv
    done
done
echo "Finished round 2, yield and non-yield no sync"

iters3=(100 1000 10000 100000)

for i in ${iters3[@]}; do
    ./gencsv 10 1 $i >> $addcsv
done
echo "Finished round 3, single-thread no sync"

threads4=(2 4 8 12)

for t in ${threads4[@]}; do
    ./gencsvy 10 $t 10000   >> $addcsv
    ./gencsvy 10 $t 10000 m >> $addcsv
    ./gencsvy 10 $t 10000 c >> $addcsv
    ./gencsvy 10 $t 1000  s >> $addcsv
done
echo "Finished round 4, yield with all sync options"

threads5=(1 2 4 8 12)

for t in ${threads5[@]}; do
    ./gencsv 10 $t 10000   >> $addcsv
    ./gencsv 10 $t 10000 m >> $addcsv
    ./gencsv 10 $t 10000 s >> $addcsv
    ./gencsv 10 $t 10000 c >> $addcsv
done
echo "Finished round 5, no yield with all sync options"
