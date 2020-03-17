cmake_minimum_required(VERSION 3.15)
project(PartC C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(lab4c_tcp lab4c_tcp.c utilities.c utilities.h)