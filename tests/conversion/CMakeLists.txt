cmake_minimum_required(VERSION 3.10)
project(TestConversion)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem /usr/include/c++/9 -isystem /usr/include/x86_64-linux-gnu/c++/9 -isystem /usr/lib/gcc/x86_64-linux-gnu/9/include")

# List all input files
file(GLOB TEST_SRCS "in/*.cpp")

# Dummy executable just to generate the database
add_library(dummy_objs OBJECT ${TEST_SRCS})

