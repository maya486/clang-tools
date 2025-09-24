cmake_minimum_required(VERSION 3.13)
project(FunctionInjector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-frtti)

set(Clang_DIR "/home/mrebholz/llvm-project/build/lib/cmake/clang")  # <-- important!

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(
  /home/mrebholz/llvm-project/clang/include
  /home/mrebholz/llvm-project/build/tools/clang/include
  /home/mrebholz/llvm-project/build/include
  /home/mrebholz/llvm-project/llvm/include
)

message(STATUS "Found LLVM in: ${LLVM_DIR}")
message(STATUS "Found Clang in: ${Clang_DIR}")

# Get LLVM libs you need
llvm_map_components_to_libnames(llvm_libs support core irreader)

add_executable(FunctionInjector main.cpp)

target_link_libraries(FunctionInjector
  PRIVATE
    ${llvm_libs}
    clangTooling
    clangBasic
    clangASTMatchers
)

