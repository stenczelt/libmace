cmake_minimum_required(VERSION 3.14)
project(mace VERSION 1.0.0 LANGUAGES CXX Fortran)

# Fortran
enable_language(Fortran)
set(CMAKE_Fortran_FORMAT FREE)
set(CMAKE_Fortran_STANDARD f2003)

# Options
option(MACE_BUILD_EXAMPLES "Build mace examples" OFF)
option(MACE_BUILD_TESTS "Build mace tests" OFF)
option(MACE_USE_SYSTEM_TORCH "Use system-installed libtorch" ON)
option(MACE_BUILD_FORTRAN "Build Fortran interface" ON)
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

# Fortran interface files
file(GLOB_RECURSE FORTRAN_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/fortran/*.f90"
)

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

# Define the C++ library
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

# Fortran interface library
if (MACE_BUILD_FORTRAN)
    # MPI
    find_package(MPI REQUIRED)

    # MACE Fortran module: interface only, needs MPI
    add_library(mace_fortran INTERFACE)
    add_library(mace::fortran ALIAS mace_fortran)
    target_sources(mace_fortran INTERFACE src/fortran/mace.f90)
    target_include_directories(mace_fortran INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/fortran
            ${MPI_Fortran_INCLUDE_DIRS}
    )
    target_link_libraries(mace_fortran INTERFACE
            mace
            ${MPI_Fortran_LIBRARIES}
    )
endif ()

# Install rules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Installation paths for C++ library
install(TARGETS mace
        EXPORT mace-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install Fortran library and modules if built
if (MACE_BUILD_FORTRAN)
    install(TARGETS mace_fortran
            EXPORT mace-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    #    # Install Fortran module files
    #    install(DIRECTORY "${MACE_FORTRAN_MODULE_DIR}/"
    #            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mace/fortran
    #            FILES_MATCHING PATTERN "*.mod"
    #    )
endif ()

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