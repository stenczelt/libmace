cmake_minimum_required(VERSION 3.16)
project(mace VERSION 1.0)

enable_language(Fortran)
set(CMAKE_Fortran_FORMAT FREE)
set(CMAKE_Fortran_STANDARD f2003)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# public headers
include_directories(include)

# C++ source files
set(SOURCES src/library.cpp src/arrays.h)

# mace library
add_library(mace STATIC ${SOURCES})
