cmake_minimum_required(VERSION 3.16)
project(mace
        VERSION 25.03.2
)

# -----------------------------------------------------------------------------
# CMake options - build arguments
# -----------------------------------------------------------------------------
option(USE_CUDA "Compile with CUDA support" OFF)
option(WITH_TESTS "Compile test suite" OFF)
option(WITH_EXAMPLES "Compile example programs" OFF)

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

# -----------------------------
# C++ source files
set(MACE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(SOURCES src/neighbour_list.cpp src/mace.cpp src/mace_c_api.cpp)

# mace library: underlying C++ code
add_library(mace STATIC ${SOURCES})
target_include_directories(mace
        # public ones with compatibility for install setup
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        # private headers & implementation
        PRIVATE
        ${MACE_SOURCE_DIR}
)
target_link_libraries(mace
        PUBLIC
        torch
)
# Ensure C/C++ standard library is linked (for F90 usage)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(mace INTERFACE stdc++)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_libraries(mace INTERFACE c++)
else ()
    # todo: add cases for additional compilers
    message(FATAL_ERROR "Unhandled compiler.")
endif ()

# -----------------------------
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
if (WITH_EXAMPLES)
    add_subdirectory(examples)
endif ()

# -----------------------------------------------------------------------------
# Installation & Packaging (w/ cmake)
# -----------------------------------------------------------------------------

# foreplay
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# exports & alias targets
add_library(mace::mace ALIAS mace)
add_library(mace::fortran ALIAS mace_fortran)

# exported & saved variables
set(MACE_FORTRAN_MODULE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/fortran/mace.f90"
        CACHE STRING
        "MACE F90 wrapper path to access directly.")

# install: libraries & executables
install(TARGETS mace
        EXPORT mace-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# install: headers
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# install: F90 interface source
install(FILES
        src/fortran/mace.f90
        src/fortran/mace.stub.f90
        DESTINATION include
)

# -----------------------------
# CMake package files
install(EXPORT mace-targets
        FILE mace-targets.cmake
        NAMESPACE mace::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mace
)
# Create config file from template
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mace-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/mace-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mace
)
# Create version file
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/mace-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
# install: config and version files
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mace-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/mace-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mace
)
