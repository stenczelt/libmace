cmake_minimum_required(VERSION 3.30)
project(libmace_fromscratch)

set(CMAKE_CXX_STANDARD 17)

add_library(libmace_fromscratch STATIC library.cpp)
