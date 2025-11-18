! Created by Tamas K Stenczel on 2024/07/08.

module mace

  use iso_c_binding
  use mace_comms

  implicit none

  private
  public :: MaceModel, dp

  integer, parameter :: dp = selected_real_kind(15, 300)

  ! MACE Model type with member methods
  type MaceModel
    private
    type(c_ptr) :: ptr ! pointer to the C class
    type(CommEnv) :: comm_env ! communication environment
  contains
    ! member functions
    procedure :: reload => mace_reload
    procedure :: print => mace_print
    procedure :: calculate => mace_calculate
    procedure :: deallocate => mace_finalise ! destructor
  end type MaceModel

  ! MACE Model constructor
  interface MaceModel
    procedure mace_init_mpi
    procedure mace_init_serial
    procedure mace_init_old_mpi
  end interface

  ! ----------------------------------------------------------------------------
  ! C function bindings
  ! ----------------------------------------------------------------------------
  interface
    ! has return value -> ftn function
    function cmace_init(model_path) bind(C)
      use iso_c_binding
      implicit none
      type(c_ptr) :: cmace_init
      character(len = 1, kind = C_CHAR), intent(in) :: model_path(*)
    end function cmace_init

    ! void functions -> ftn subroutine
    subroutine cmace_finalise(self) bind(C)
      use iso_c_binding
      implicit none
      type(c_ptr), value :: self
    end subroutine cmace_finalise

    subroutine cmace_reload(self) bind(C)
      use iso_c_binding
      implicit none
      type(c_ptr), value :: self
    end subroutine cmace_reload

    subroutine cmace_print(self) bind(C)
      use iso_c_binding
      implicit none
      type(c_ptr), value :: self
    end subroutine cmace_print

    subroutine cmace_calculate(self, calc_virial, n_atoms, cell, pbc, atomic_numbers, &
        positions, total_energy, node_energy, forces, virial) bind(C)
      use iso_c_binding
      implicit none
      type(c_ptr), value :: self
      integer(c_int), intent(in), value :: calc_virial ! n.b. 0 or 1 INT for bool flag
      integer(c_int), intent(in), value :: n_atoms
      type(c_ptr), value :: cell
      type(c_ptr), value :: pbc  ! n.b. 0 or 1 INT for bool flags
      type(c_ptr), value :: atomic_numbers
      type(c_ptr), value :: positions
      type(c_ptr), value :: total_energy
      type(c_ptr), value :: node_energy
      type(c_ptr), value :: forces
      type(c_ptr), value :: virial
    end subroutine cmace_calculate

  end interface

  ! type conversions Fortran -> C
  interface f2c
    procedure string_f2c
    procedure logical_f2c
  end interface f2c
contains

  ! ----------------------------------------------------------------------------
  ! Ftn versions of class member functions
  ! ----------------------------------------------------------------------------
  function mace_init_mpi(model_path, mpi_comm)  result(model)
    implicit none
    type(MaceModel) :: model

    character(len = *), intent(in) :: model_path
    type(MPI_Comm), intent(in) :: mpi_comm

    ! initialise communication environment
    model%comm_env = comm_env_init(mpi_comm)

    ! initialise model on ROOT ONLY
    if (model%comm_env%is_root_process()) model%ptr = cmace_init(f2c(model_path))
  end function mace_init_mpi

  function mace_init_serial(model_path) result(model)
    ! Serial (no MPI)
    implicit none
    type(MaceModel) :: model

    character(len = *), intent(in) :: model_path

    ! initialise communication environment (serial)
    model%comm_env = comm_env_init()

    ! initialise model
    model%ptr = cmace_init(f2c(model_path))
  end function mace_init_serial

  function mace_init_old_mpi(model_path, mpi_comm_int)  result(model)
    ! OLD MPI: `include "mpif.h"` / `use mpi` where communicators are integers
    implicit none
    type(MaceModel) :: model

    character(len = *), intent(in) :: model_path
    integer, intent(in) :: mpi_comm_int

    ! initialise communication environment
    model%comm_env = comm_env_init(mpi_comm_int)

    ! initialise model on ROOT ONLY
    if (model%comm_env%is_root_process()) model%ptr = cmace_init(f2c(model_path))
  end function mace_init_old_mpi


  subroutine mace_reload(self)
    implicit none
    class(MaceModel), intent(in) :: self
    ! body
    if (self%comm_env%is_root_process()) call cmace_reload(self%ptr)
  end subroutine mace_reload

  subroutine mace_print(self)
    implicit none
    class(MaceModel), intent(in) :: self
    ! body
    if (self%comm_env%is_root_process()) call cmace_print(self%ptr)
  end subroutine mace_print

  subroutine mace_calculate(self, calc_virial, n_atoms, cell, pbc, atomic_numbers, &
      positions, total_energy, node_energy, forces, virial)
    implicit none

    class(MaceModel), intent(in) :: self
    logical, intent(in) :: calc_virial
    integer(c_int), intent(in) :: n_atoms
    real(dp), dimension(3, 3), intent(in), target :: cell
    logical, dimension(3), intent(in) :: pbc
    integer(c_int), dimension(n_atoms), intent(in), target :: atomic_numbers
    real(dp), dimension(3, n_atoms), intent(in), target :: positions

    real(dp), intent(out) :: total_energy
    real(dp), dimension(n_atoms), intent(out) :: node_energy
    real(dp), dimension(3, n_atoms), intent(out) :: forces
    real(dp), dimension(6), intent(out) :: virial

    ! calculate on root process
    if (self%comm_env%is_root_process()) call perform_calculation()

    ! broadcast the results to all processes
    call self%comm_env%broadcast_results(total_energy, node_energy, forces, virial)

  contains

    subroutine perform_calculation()
      !=========================================================================!
      ! Perform the calculation
      !=========================================================================!

      implicit none

      integer(c_int), dimension(3), target :: pbc_cbool ! note: we are passing booleans as INT
      real(c_double), dimension(:), allocatable, target :: c_total_energy
      real(c_double), dimension(:), allocatable, target :: c_node_energy
      real(c_double), dimension(:, :), allocatable, target :: c_forces
      real(c_double), dimension(:), allocatable, target :: c_virial

      ! body
      type(c_ptr) :: total_energy_ptr, node_energy_ptr, forces_ptr, virial_ptr

      ! convert boolean
      pbc_cbool = f2c(pbc)

      ! allocate output arrays for C to use
      allocate(c_total_energy(1))
      allocate(c_node_energy(n_atoms))
      allocate(c_forces(3, n_atoms))
      allocate(c_virial(6))

      total_energy_ptr = C_LOC(c_total_energy(1))
      node_energy_ptr = C_LOC(c_node_energy(1))
      forces_ptr = C_LOC(c_forces(1, 1))
      virial_ptr = C_LOC(c_virial(1))

      call cmace_calculate(self%ptr, f2c(calc_virial), n_atoms, C_LOC(cell(1, 1)), &
          C_LOC(pbc_cbool(1)), C_LOC(atomic_numbers(1)), C_LOC(positions(1, 1)), &
          total_energy_ptr, node_energy_ptr, forces_ptr, virial_ptr)

      ! data into arg arrays & conversions from C_double -> DP
      total_energy = c_total_energy(1)
      node_energy = c_node_energy
      forces = c_forces
      virial = c_virial

      ! deallocate our buffers
      deallocate(c_total_energy)
      deallocate(c_node_energy)
      deallocate(c_forces)
      deallocate(c_virial)

      return
    end subroutine perform_calculation

  end subroutine mace_calculate

  subroutine mace_finalise(self)
    implicit none
    class(MaceModel), intent(in) :: self
    ! body
    call cmace_finalise(self%ptr)
  end subroutine mace_finalise

  ! ----------------------------------------------------------------------------
  ! Internal utilities
  ! ----------------------------------------------------------------------------
  function string_f2c(string)
    ! Convert Fortran string to C string
    implicit none

    character(len = *), intent(in) :: string
    character(len = 1, kind = C_CHAR) :: string_f2c(len_trim(string) + 1)
    integer :: N, i

    N = len_trim(string)
    do i = 1, N
      string_f2c(i) = string(i:i)
    end do
    string_f2c(N + 1) = C_NULL_CHAR

  end function string_f2c

  elemental function logical_f2c(value)
    ! Convert Fortran logical to C_INT: .false. -> 0, .true. -> 1 explicitly
    implicit none
    logical, intent(in) :: value
    integer(c_int) :: logical_f2c
    if (value) then
      logical_f2c = 1
    else
      logical_f2c = 0
    end if
  end function logical_f2c

  subroutine raise()
    implicit none
    error stop
  end subroutine raise

end module mace