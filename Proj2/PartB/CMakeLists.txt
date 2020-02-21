cmake_minimum_required(VERSION 3.15)
project(PartB C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -I/usr/local/include")

add_executable(lab2_list lab2_list.c SortedList.h SortedList.c)