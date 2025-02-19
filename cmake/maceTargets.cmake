include(CMakePackageConfigHelpers)
export(TARGETS mace mace_fortran
        NAMESPACE MACE::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/maceTargets.cmake")
