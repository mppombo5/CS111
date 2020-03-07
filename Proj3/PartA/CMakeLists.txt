cmake_minimum_required(VERSION 3.15)
project(PartA)

set(CMAKE_CXX_STANDARD 17)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_executable(lab3a lab3a.cpp ext2_fs.h)