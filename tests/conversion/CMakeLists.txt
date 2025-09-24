cmake_minimum_required(VERSION 3.10)
project(TestConversion)

set(CMAKE_CXX_STANDARD 17)

# List all input files
file(GLOB TEST_SRCS "in/*.cpp")

# Dummy executable just to generate the database
add_library(dummy_objs OBJECT ${TEST_SRCS})

