cmake_minimum_required(VERSION 3.16)
project(mace VERSION 1.0)

enable_language(Fortran)
set(CMAKE_Fortran_FORMAT FREE)
set(CMAKE_Fortran_STANDARD f2003)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# include LibTorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# public headers
include_directories(include)

# C++ source files
set(SOURCES src/library.cpp src/neighbour_list.cpp src/mace.cpp)

# mace library
add_library(mace STATIC ${SOURCES})
target_include_directories(mace PRIVATE include)
target_include_directories(mace PRIVATE src)
target_link_libraries(mace PUBLIC "${TORCH_LIBRARIES}")
