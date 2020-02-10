cmake_minimum_required(VERSION 3.15)
project(PartA C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(lab2a_add lab2_add.c)