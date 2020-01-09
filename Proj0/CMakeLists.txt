cmake_minimum_required(VERSION 3.15)
project(Proj0 C)

set(CMAKE_C_STANDARD 11)

add_executable(lab0 lab0.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")