cmake_minimum_required(VERSION 3.16)
project(mace)
set(libmace_VERSION 2025.02.22)

# -----------------------------------------------------------------------------
# CMake options - build arguments
# -----------------------------------------------------------------------------
option(USE_CUDA "Compile with CUDA support" OFF)
option(WITH_TESTS "Compile test suite" ON)

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

# MPI
find_package(MPI REQUIRED)

# -----------------------------------------------------------------------------
# Our code
# -----------------------------------------------------------------------------

# C++ source files
set(MACE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(SOURCES src/neighbour_list.cpp src/mace.cpp src/mace_c_api.cpp)

# mace library: underlying C++ code
add_library(mace SHARED ${SOURCES})
target_include_directories(mace PUBLIC
        # for compatibility with install setup
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
# public headers
target_include_directories(mace PRIVATE ${MACE_SOURCE_DIR}) # private headers & implementation
target_link_libraries(mace PRIVATE "${TORCH_LIBRARIES}") # torch

# MACE Fortran module: interface only, needs MPI
add_library(mace_fortran INTERFACE)
target_sources(mace_fortran INTERFACE src/fortran/mace.f90)
target_include_directories(mace_fortran INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fortran
        ${MPI_Fortran_INCLUDE_DIRS}
)
target_link_libraries(mace_fortran INTERFACE
        mace
        ${MPI_Fortran_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Tests & Examples
# -----------------------------------------------------------------------------
if (WITH_TESTS)
    enable_testing()

    # library for testing: internals are available to test code as well
    add_library(mace_testing INTERFACE)
    target_link_libraries(mace_testing INTERFACE mace)
    target_include_directories(mace_testing INTERFACE ${MACE_SOURCE_DIR})

    # tests
    set(INSTALL_GTEST OFF) # see https://github.com/google/googletest/issues/2829
    add_subdirectory(tests)
endif ()

# examples
add_subdirectory(examples)

# -----------------------------------------------------------------------------
# Installation & Packaging (w/ cmake)
# -----------------------------------------------------------------------------

# install: libraries & executables
install(TARGETS mace
        EXPORT maceTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
# install: headers & F90 interfaces
install(FILES
        include/mace.h
        src/fortran/mace.f90
        src/fortran/mace.stub.f90
        DESTINATION include
)

# CMake package:
#   - write a version file for the CMake
#   - namespace & contents
#   - copy all into {install_dir}/cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/maceConfigVersion.cmake"
        VERSION ${libmace_VERSION}
        COMPATIBILITY AnyNewerVersion
)
install(EXPORT maceTargets
        FILE maceTargets.cmake
        NAMESPACE MACE::
        DESTINATION cmake
)
install(FILES
        cmake/maceConfig.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/maceConfigVersion.cmake"
        DESTINATION cmake)

