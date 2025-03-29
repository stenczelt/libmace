! Created by Tamas K Stenczel on 2024/07/07.

program main
  use mace, only : MaceModel, dp

  implicit none

  ! parameters

  integer, parameter :: N = 2

  ! cell & atoms
  real(dp), dimension(3, 3) :: lattice
  logical, dimension(3) :: pbc
  integer, dimension(N) :: atomic_numbers
  real(dp), dimension(3, N) :: abs_pos

  ! outputs
  real(dp) :: total_energy
  real(dp), dimension(N) :: node_energy
  real(dp), dimension(3, N) :: forces
  real(dp), dimension(6) :: virial

  ! MACE model
  type(MaceModel) :: calc
  character(len = 256) :: model_path

  ! misc
  integer :: i

  ! read model path argument
  call get_command_argument(1, model_path)

  ! read MACE model
  calc = MaceModel(adjustl(trim(model_path)))
  call calc%print()

  ! set cell & positions
  lattice = reshape(&
      (/2.15_dp, 2.15_dp, 0.0_dp, &
          0.0_dp, 2.15_dp, 2.20_dp, &
          2.01_dp, 0.0_dp, 2.15_dp /), &
      shape(lattice))
  pbc = (/.true., .true., .true./)
  atomic_numbers = (/6, 14/)
  abs_pos = reshape(&
      ! notice: this is written as rows here, but reshaped & filled in correctly
      (/ 1.077_dp, 1.2_dp, 1.0_dp, &
          0.0_dp, 0.0_dp, 0.01_dp/), &
      shape(abs_pos), order = (/1, 2/))

  write(*, *) "cell(:, 1)", lattice(:, 1)
  write(*, *) "cell(:, 2)", lattice(:, 2)
  write(*, *) "cell(:, 3)", lattice(:, 3)

  do i = 1, N
    write(*, *) "pos", i, abs_pos(:, i)
  end do

  ! ----------------------------------
  ! run calculation 1x
  ! ----------------------------------
  call calc%calculate(.true., N, lattice, pbc, atomic_numbers, abs_pos, total_energy, node_energy, forces, virial)
  write(*, *) "CALC 1"
  write(*, *) "total_energy", total_energy
  write(*, *) "node_energy(:)", node_energy
  write(*, *) "forces(:, 1)", forces(:, 1)
  write(*, *) "forces(:, 2)", forces(:, 2)
  write(*, *) "virial", virial
  ! ----------------------------------
  ! run calculation again
  ! ----------------------------------
  call calc%calculate(.true., N, lattice, pbc, atomic_numbers, abs_pos, total_energy, node_energy, forces, virial)
  write(*, *) "CALC 2"
  write(*, *) "total_energy", total_energy
  write(*, *) "node_energy(:)", node_energy
  write(*, *) "forces(:, 1)", forces(:, 1)
  write(*, *) "forces(:, 2)", forces(:, 2)
  write(*, *) "virial", virial

  call calc%deallocate()

end program main