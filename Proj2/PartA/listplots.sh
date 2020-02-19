#!/usr/local/bin/bash

listcsv=lab2_list.csv
echo "" > $listcsv

iters1=(10 100 1000 10000 20000)

for i in ${iters1[@]}; do
    ./listcsv 3 1 $i >> $listcsv 2> /dev/null
done
echo "Round 1 done, single thread with varying iterations"

threads2_1=(2 4 8 12)
iters2_1=(1 10 100 1000)

for t in ${threads2_1[@]}; do
    for i in ${iters2_1[@]}; do
        ./listcsv 3 $t $i >> $listcsv 2> /dev/null
    done
done
echo "Round 2.1 done, multiple threads with no yield"

iters2_2=(1 2 4 8 16 32)

for t in ${threads2_1[@]}; do
    for i in ${iters2_2[@]}; do
        ./listcsv 3 $t $i 0 i >> $listcsv 2> /dev/null
        ./listcsv 3 $t $i 0 d >> $listcsv 2> /dev/null
        ./listcsv 3 $t $i 0 il >> $listcsv 2> /dev/null
        ./listcsv 3 $t $i 0 dl >> $listcsv 2> /dev/null
    done
done
echo "Round 2.2 done, multiple threads with yield"

yields3=(i d l id il dl idl)
syncs3=(s m)
for y in ${yields3[@]}; do
    for s in ${syncs3[@]}; do
        ./listcsv 3 12 32 $s $y >> $listcsv 2> /dev/null
    done
done
echo "Round 3 done, static threads/iters with variable yield and sync"

threads4=(1 2 3 8 12 16 24)
syncs4=(s m)
for t in ${threads4[@]}; do
    for s in ${syncs4[@]}; do
        ./listcsv 3 $t 1000 $s 0 >> $listcsv 2> /dev/null
    done
done
echo "Round 4 done, variable threads/syncs"
