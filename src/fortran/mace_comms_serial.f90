! Created by Tamas K Stenczel on 2025/11/18.
!
! Communication module - Serial version
! Interface for serial opertions matching the MPI-parallel communication module

module mace_comms
  use iso_c_binding

  implicit none

  private
  public :: CommEnv, dp
  public :: comm_env_init, comm_env_free

  integer, parameter :: dp = selected_real_kind(15, 300)
  integer, parameter :: COMM_ROOT_RANK = 0

  ! Communication environment type (serial stub)
  type CommEnv
    private
    integer :: rank = COMM_ROOT_RANK
    logical :: is_root = .true.
  contains
    ! Query methods
    procedure :: get_rank => comm_get_rank
    procedure :: is_root_process => comm_is_root

    ! Synchronization - no-ops
    procedure :: barrier => comm_barrier
    procedure :: barrier_nonbusy => comm_barrier_nonbusy

    ! Broadcast operations (generic interface) - no-ops
    procedure, private :: bcast_scalar_real
    procedure, private :: bcast_array_real_1d
    procedure, private :: bcast_array_real_2d
    generic :: bcast => bcast_scalar_real, bcast_array_real_1d, bcast_array_real_2d

    ! High-level operation
    procedure :: broadcast_results => comm_broadcast_mace_results
  end type CommEnv

  ! Placeholder type for serial version (for API compatibility)
  type MPI_Comm
    integer :: MPI_VAL = 0
  end type MPI_Comm

  ! Constructor interfaces
  interface comm_env_init
    module procedure comm_env_init_serial
    module procedure comm_env_init_mpi_compat
    module procedure comm_env_init_mpi_old_compat
  end interface

contains

  ! ============================================================================
  ! Constructors (all return serial environment)
  ! ============================================================================

  function comm_env_init_serial() result(env)
    ! Serial initialization (no arguments)
    type(CommEnv) :: env
    ! All fields already initialized with defaults
  end function comm_env_init_serial

  function comm_env_init_mpi_compat(mpi_comm) result(env)
    ! Compatibility with MPI_f08 interface (ignores communicator)
    type(MPI_Comm), intent(in) :: mpi_comm
    type(CommEnv) :: env
    ! All fields already initialized with defaults
  end function comm_env_init_mpi_compat

  function comm_env_init_mpi_old_compat(mpi_comm_int) result(env)
    ! Compatibility with old-style MPI interface (ignores communicator)
    integer, intent(in) :: mpi_comm_int
    type(CommEnv) :: env
    ! All fields already initialized with defaults
  end function comm_env_init_mpi_old_compat

  subroutine comm_env_free(env)
    type(CommEnv), intent(inout) :: env
    ! Nothing to do in serial version
  end subroutine comm_env_free

  ! ============================================================================
  ! Query methods
  ! ============================================================================

  pure function comm_get_rank(self) result(rank)
    class(CommEnv), intent(in) :: self
    integer :: rank
    rank = self%rank  ! Always 0
  end function comm_get_rank

  pure function comm_is_root(self) result(is_root)
    class(CommEnv), intent(in) :: self
    logical :: is_root
    is_root = self%is_root  ! Always .true.
  end function comm_is_root

  ! ============================================================================
  ! Synchronization (no-ops)
  ! ============================================================================

  subroutine comm_barrier(self)
    class(CommEnv), intent(in) :: self
    ! No-op in serial version
  end subroutine comm_barrier

  subroutine comm_barrier_nonbusy(self, interval_usec)
    class(CommEnv), intent(in) :: self
    integer, intent(in) :: interval_usec
    ! No-op in serial version
  end subroutine comm_barrier_nonbusy

  ! ============================================================================
  ! Broadcast operations (no-ops)
  ! ============================================================================

  subroutine bcast_scalar_real(self, buffer)
    class(CommEnv), intent(in) :: self
    real(dp), intent(inout) :: buffer
    ! No-op - data already present
  end subroutine bcast_scalar_real

  subroutine bcast_array_real_1d(self, buffer)
    class(CommEnv), intent(in) :: self
    real(dp), dimension(:), intent(inout) :: buffer
    ! No-op - data already present
  end subroutine bcast_array_real_1d

  subroutine bcast_array_real_2d(self, buffer)
    class(CommEnv), intent(in) :: self
    real(dp), dimension(:, :), intent(inout) :: buffer
    ! No-op - data already present
  end subroutine bcast_array_real_2d

  ! ============================================================================
  ! High-level operations (no-ops)
  ! ============================================================================

  subroutine comm_broadcast_mace_results(self, total_energy, node_energy, &
      forces, virial)
    class(CommEnv), intent(in) :: self
    real(dp), intent(inout) :: total_energy
    real(dp), dimension(:), intent(inout) :: node_energy
    real(dp), dimension(:, :), intent(inout) :: forces
    real(dp), dimension(6), intent(inout) :: virial
    ! No-op - all data already present in serial version
  end subroutine comm_broadcast_mace_results

end module mace_comms
