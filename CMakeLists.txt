cmake_minimum_required(VERSION 3.16)
project(mace VERSION 1.0)

# -----------------------------------------------------------------------------
# CMake options - build arguments
# -----------------------------------------------------------------------------
option(USE_CUDA "Compile with CUDA support" OFF)

# -----------------------------------------------------------------------------
# Languages
# -----------------------------------------------------------------------------
# C++
set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Fortran
enable_language(Fortran)
set(CMAKE_Fortran_FORMAT FREE)
set(CMAKE_Fortran_STANDARD f2003)

# CUDA
if (USE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    find_package(CUDA REQUIRED)
else ()
    message(STATUS "Not using CUDA.")
endif ()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

# include LibTorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# -----------------------------------------------------------------------------
# Our code
# -----------------------------------------------------------------------------

# public headers
include_directories(include)

# C++ source files
set(SOURCES src/neighbour_list.cpp src/mace.cpp)

# mace library
add_library(mace STATIC ${SOURCES})
target_include_directories(mace PRIVATE include)
target_include_directories(mace PRIVATE src)
target_link_libraries(mace PUBLIC "${TORCH_LIBRARIES}")


# -----------------------------------------------------------------------------
# Installation & other settings
# -----------------------------------------------------------------------------
