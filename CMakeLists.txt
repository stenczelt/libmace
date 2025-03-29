cmake_minimum_required(VERSION 3.14)
project(mace VERSION 1.0.0 LANGUAGES CXX)

# Options
option(MACE_BUILD_EXAMPLES "Build mace examples" OFF)
option(MACE_BUILD_TESTS "Build mace tests" OFF)
option(MACE_USE_SYSTEM_TORCH "Use system-installed libtorch" ON)
set(MACE_TORCH_VERSION "1.10.0" CACHE STRING "libtorch version to use if downloading")

# C++ standard requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Make IDEs show header files
file(GLOB_RECURSE HEADER_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# Library source files
file(GLOB_RECURSE SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

message(STATUS HEADER_FILES ${HEADER_FILES})
message(STATUS SOURCE_FILES ${SOURCE_FILES})

# Handle dependencies
include(FetchContent)

# Try to find system libtorch first if preferred
if (MACE_USE_SYSTEM_TORCH)
    find_package(Torch QUIET REQUIRED)
endif ()

# If system libtorch not found or not preferred, fetch it
if (NOT MACE_USE_SYSTEM_TORCH OR NOT Torch_FOUND)
    message(STATUS "Using FetchContent to download libtorch ${MACE_TORCH_VERSION}")
    FetchContent_Declare(
            libtorch
            URL https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-${MACE_TORCH_VERSION}%2Bcpu.zip
            # URL_HASH would need to be updated based on version
    )
    FetchContent_MakeAvailable(libtorch)
else ()
    message(STATUS "Using system-installed libtorch ${Torch_VERSION}")
endif ()

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
install(DIRECTORY include/
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