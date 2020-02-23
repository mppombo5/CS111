#!/usr/bin/gnuplot
# 
# input: lab2b_list.csv
#   1. test name
#   2. # threads
#   3. # iterations per thread
#   4. # lists
#   5. # operations performed
#   6. runtime (ns)
#   7. runtime per operation (ns)
#   8. average wait-for-lock time (ns)
#
# output a bunch of png's

# general plot parameters
set terminal png
set datafile separator ","

# First graph: total number of ops per second for each sync method
# Throughput = 1 billion/time per op
# x: # threads
# y: # operations
set title "Lab2b-1: Total Operations per Second"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:30]
set ylabel "Operations per Second"
set logscale y 10
set output 'lab2b_1.png'

# grep out no yield, spinlock and mutex tests
plot \
    "< egrep 'list-none-m,[1248][246]?,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'mutex' with linespoints lc rgb 'green',\
    "< egrep 'list-none-s,[1248][246]?,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title 'spinlock' with linespoints lc rgb 'blue'


# Second graph: plot wait-for-lock AND avg time per op
# x: threads
# y: time (ns)
set title "Lab2b-2: Time per Operation and Time per Lock (Mutex)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:30]
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'

# grep no yield, mutex tests with time-per-op and wait-for-lock
plot \
    "< egrep 'list-none-m,[1248][46]?,1000,1,' lab2b_list.csv" using ($2):($7)\
        title 'time-per-op' with linespoints lc rgb 'green',\
    "< egrep 'list-none-m,[1248][46]?,1000,1,' lab2b_list.csv" using ($2):($8)\
        title 'wait-for-lock' with linespoints lc rgb 'blue'

# Third graph: successful runs of non-synchronized runs w/ lists
# x: # threads
# y: # iters
set title "Lab2b-3: Unprotected Threads/Iterations w/ Lists, without failure"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:20]
set ylabel "Successful Iterations"
set logscale y 10
set yrange [0.75:]
set output 'lab2b_3.png'

# grep no-sync w/ lists
plot \
    "< grep 'list-id-none', lab2b_list.csv" using ($2):($3)\
        title 'no sync' with points lc rgb 'red',\
    "< grep 'list-id-m', lab2b_list.csv" using ($2):($3)\
        title 'mutex' with points lc rgb 'blue',\
    "< grep 'list-id-s', lab2b_list.csv" using ($2):($3)\
        title 'spinlock' with points lc rgb 'green'

# Fourth graph: mutex sync w/ partitions
# x: # threads
# y: # operations
set title "Lab2b-4: Aggregate Throughput for Partitioned Mutex"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:16]
set ylabel "Operations per Second"
set logscale y 10
set output 'lab2b_4.png'

# grep no yield mutex tests
plot \
    "< egrep 'list-none-m,[1248]2?,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '1 list' with linespoints lc rgb 'green',\
    "< egrep 'list-none-m,[1248]2?,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '4 lists' with linespoints lc rgb 'blue',\
    "< egrep 'list-none-m,[1248]2?,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '8 lists' with linespoints lc rgb 'red',\
    "< egrep 'list-none-m,[1248]2?,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '16 lists' with linespoints lc rgb 'brown'

# Fifth graph: spinlock sync w/ partitions
# x: # threads
# y: # operations
set title "Lab2b-5: Aggregate Throughput for Partitioned Spinlock"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:16]
set ylabel "Operations per Second"
set logscale y 10
set output 'lab2b_5.png'

# grep no yield spinlock tests
plot \
    "< egrep 'list-none-s,[1248]2?,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '1 list' with linespoints lc rgb 'green',\
    "< egrep 'list-none-s,[1248]2?,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '4 lists' with linespoints lc rgb 'blue',\
    "< egrep 'list-none-s,[1248]2?,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '8 lists' with linespoints lc rgb 'red',\
    "< egrep 'list-none-s,[1248]2?,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7))\
        title '16 lists' with linespoints lc rgb 'brown'
