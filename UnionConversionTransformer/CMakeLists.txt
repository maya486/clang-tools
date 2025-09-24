cmake_minimum_required(VERSION 3.13)
project(UnionConversionTransformer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-frtti)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(UnionConversionTransformer main.cpp)

include_directories(
  /home/mrebholz/llvm-project/clang/include
  /home/mrebholz/llvm-project/build/tools/clang/include
  /home/mrebholz/llvm-project/build/include
  /home/mrebholz/llvm-project/llvm/include
)

target_link_libraries(UnionConversionTransformer
    PRIVATE
    clangTooling
    clangBasic
    clangASTMatchers
    clangAnalysis
    clangFrontend
    clangRewrite
    clangSerialization
    clangSema
    clangEdit
)

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(UnionConversionTransformer PRIVATE ${llvm_libs})

