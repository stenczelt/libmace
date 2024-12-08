cmake_minimum_required(VERSION 3.30)
project(mace)

set(CMAKE_CXX_STANDARD 17)

# public headers
include_directories(include)

# C++ source files
SET(SOURCES
        src/library.cpp
)

# mace library
add_library(mace STATIC ${SOURCES})
