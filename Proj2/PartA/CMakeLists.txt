cmake_minimum_required(VERSION 3.15)
project(PartA C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(lab2_add lab2_add.c)
add_executable(lab2_list lab2_list.c SortedList.c SortedList.h)