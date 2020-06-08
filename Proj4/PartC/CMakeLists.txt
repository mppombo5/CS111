cmake_minimum_required(VERSION 3.5)
project(PartC C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -lmraa -lm -I/usr/local/opt/openssl/include/")

add_executable(lab4c_tcp lab4c_tcp.c utilities.c utilities.h)
add_executable(lab4c_tls lab4c_tls.c utilities.c utilities.h)