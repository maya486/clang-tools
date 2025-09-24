cmake_minimum_required(VERSION 3.13)
project(TestFiles C)

file(GLOB_RECURSE TEST_SOURCES "*.c")

# Add each .c file as an "object library" — this will compile it,
# but not link (so it won't complain about missing main())
foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_library(${test_name} OBJECT ${test_file})
endforeach()

