cmake_minimum_required(VERSION 3.14)
project(mace VERSION 1.0.0 LANGUAGES CXX)

# Options
option(MACE_BUILD_EXAMPLES "Build mace examples" OFF)
option(MACE_BUILD_TESTS "Build mace tests" OFF)

# C++ standard requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Make IDEs show header files
file(GLOB_RECURSE HEADER_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/mace/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/mace/*.h"
)

# Library source files
file(GLOB_RECURSE SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Handle dependencies
include(FetchContent)
# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif ()

# Example of handling libtorch dependency
FetchContent_Declare(
        libtorch
        URL https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.10.0%2Bcpu.zip
        URL_HASH MD5=2141b6c06637d4b290bec57e524da1ac
)
FetchContent_MakeAvailable(libtorch)

# Define the library
add_library(mace ${SOURCE_FILES} ${HEADER_FILES})
add_library(mace::mace ALIAS mace)

# Set up include directories
target_include_directories(mace
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(mace
        PUBLIC
        torch
)

# Install rules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Installation paths
install(TARGETS mace
        EXPORT mace-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/mace
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
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

# Install config and version files
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mace-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/mace-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mace
)

# Examples
if (MACE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# Tests
if (MACE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()