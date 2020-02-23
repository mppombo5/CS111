#!/bin/bash

threads1=(1 2 4 8 12 16 24)
echo 'Starting round 1: fixed, synced iterations w/ varying threads...' >&2
for t in ${threads1[@]}; do
    ./listcsv 1 $t 1000 m 0 0
    ./listcsv 1 $t 1000 s 0 0
done
echo 'Finished round 1' >&2

threads2=(1 4 8 12 16)
iters2=(1 2 4 8 16)
echo 'Starting round 2: partitioned list w/ no sync...' >&2
for t in ${threads2[@]}; do
    for i in ${iters2[@]}; do
        ./listcsv 1 $t $i 0 id 4
    done
done
echo 'Finished round 2' >&2

threads3=(1 4 8 12 16)
iters3=(10 20 40 80)
echo 'Starting round 3: partitioned list w/ syncs...' >&2
for t in ${threads3[@]}; do
    for i in ${iters3[@]}; do
        ./listcsv 1 $t $i m id 4
        ./listcsv 1 $t $i s id 4
    done
done
echo 'Finished round 3' >&2

# Only start at 4 lists, because 1 list was already done in the first round
threads4=(1 2 4 8 12)
lists4=(4 8 16)
echo 'Starting round 4: variable partitions, no sync w/ varying threads...' >&2
for t in ${threads4[@]}; do
    for l in ${lists4[@]}; do
        ./listcsv 1 $t 1000 m 0 $l
        ./listcsv 1 $t 1000 s 0 $l
    done
done
echo 'Finished round 4' >&2
