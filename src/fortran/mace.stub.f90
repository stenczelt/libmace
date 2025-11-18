! Created by Tamas K Stenczel on 2024/07/08.
! Stub module: use instead of mace.f90 and adapt to your code's error handling.
!
! usage: you can copy this into the host code & implent `raise()` as you wish, e.g.
! handling exit across all MPI processes, etc. and use in place of the library when
! compiled without it.

module mace
  use constants, only : dp

  implicit none

  private
  public :: MaceModel

  type MaceModel
  contains
    procedure :: reload => mace_reload
    procedure :: print => mace_print
    procedure :: calculate => mace_calculate
    procedure :: deallocate => mace_finalise
  end type MaceModel

  type MPI_Comm
    ! placeholder, if you don't have mpi_f08 to include
    integer :: MPI_VAL
  end type MPI_Comm

  interface MaceModel
    procedure mace_init_mpi
    procedure mace_init_no_mpi
    procedure mace_init_old_mpi
  end interface
contains
  subroutine raise()
    implicit none
    ! todo: implement this for your code's needs
    error stop
  end subroutine raise

  function mace_init(model_path)
    implicit none
    type(MaceModel) :: mace_init
    character(len = *), intent(in) :: model_path
    call raise()
  end function mace_init

  function mace_init_mpi(model_path, comm)  result(model)
    implicit none
    type(MaceModel) :: model
    character(len = *), intent(in) :: model_path
    type(MPI_Comm), intent(in) :: comm
    call raise()
  end function mace_init_mpi

  function mace_init_no_mpi(model_path) result(model)
    implicit none
    type(MaceModel) :: model
    character(len = *), intent(in) :: model_path
    call raise()
  end function mace_init_no_mpi

  function mace_init_old_mpi(model_path, comm)  result(model)
    implicit none
    type(MaceModel) :: model
    character(len = *), intent(in) :: model_path
    integer, intent(in) :: comm
    call raise()
  end function mace_init_old_mpi

  subroutine mace_reload(self)
    implicit none
    class(MaceModel), intent(in) :: self
    call raise()
  end subroutine mace_reload

  subroutine mace_print(self)
    implicit none
    class(MaceModel), intent(in) :: self
    call raise()
  end subroutine mace_print

  subroutine mace_calculate(self, calc_virial, n_atoms, cell, pbc, atomic_numbers, &
      positions, total_energy, node_energy, forces, virial)
    implicit none

    class(MaceModel), intent(in) :: self
    logical, intent(in) :: calc_virial
    integer, intent(in) :: n_atoms
    real(dp), dimension(3, 3), intent(in), target :: cell
    logical, dimension(3), intent(in) :: pbc
    integer, dimension(n_atoms), intent(in), target :: atomic_numbers
    real(dp), dimension(3, n_atoms), intent(in), target :: positions
    real(dp), intent(out) :: total_energy
    real(dp), dimension(n_atoms), intent(out) :: node_energy
    real(dp), dimension(3, n_atoms), intent(out) :: forces
    real(dp), dimension(6), intent(out) :: virial

    call raise()

  end subroutine mace_calculate

  subroutine mace_finalise(self)
    implicit none
    class(MaceModel), intent(in) :: self
    call raise()
  end subroutine mace_finalise
end module mace