cmake_minimum_required(VERSION 3.15)
project(PartB C)

set(CMAKE_C_STANDARD 11)

add_executable(lab1b-client lab1b-client.c)
add_executable(lab1b-server lab1b-server.c)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -lz)

add_executable(TestClient client.c)
add_executable(TestServer server.c)
add_executable(TestZlib zpipe.c)