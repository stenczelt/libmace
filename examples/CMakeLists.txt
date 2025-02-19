# example programs


# C++ program
add_executable(mace_c++ main.cpp)
target_link_libraries(mace_c++ mace "${TORCH_LIBRARIES}")

# F90 program
add_executable(mace_f90 main.f90)
target_link_libraries(mace_f90 mace_fortran)
