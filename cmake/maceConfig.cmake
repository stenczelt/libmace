# Compute the relative path to the current file's directory
get_filename_component(CURRENT_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# todo: decide if this is a good idea or not...
if(CMAKE_C_COMPILER_ID STREQUAL "NVHPC")
    # This may be a bug, but seems that for Torch to find CUDA, you need to have
    # found it before this way, and enable_language(CUDA) does not do the trick.
    message(STATUS "Using NVHPC compilers -> require CUDA as well")
    enable_language(CXX)
    find_package(CUDA REQUIRED)
endif()

find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Include the targets file
include("${CURRENT_DIR}/maceTargets.cmake")

# Set the library path variable
set(MACE_LIBRARIES MACE::mace)

# Ensure C/C++ standard library is linked
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(MACE_LIBRARIES ${MACE_LIBRARIES} stdc++)
elseif(CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    set(MACE_LIBRARIES ${MACE_LIBRARIES} c++)
endif() # todo: add cases for additional compilers

# Set the path to the Fortran file
# todo: make sure this is from the built output instead of the source tree
set(MACE_FORTRAN_MODULE "${CURRENT_DIR}/../include/mace.f90")
