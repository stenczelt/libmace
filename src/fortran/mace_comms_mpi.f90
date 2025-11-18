! Created by Tamas K Stenczel on 2025/11/18.
!
! Communication module - MPI version
! Interface for MPI-parallel communication operations

module mace_comms
  use mpi_f08
  use iso_c_binding

  implicit none

  private
  public :: CommEnv, dp
  public :: comm_env_init, comm_env_free

  integer, parameter :: dp = selected_real_kind(15, 300)
  integer, parameter :: COMM_ROOT_RANK = 0

  ! Communication environment type
  type CommEnv
    private
    type(MPI_Comm) :: comm
    integer :: rank
    logical :: is_root
  contains
    ! Query methods
    procedure :: get_rank => comm_get_rank
    procedure :: is_root_process => comm_is_root

    ! Synchronization
    procedure :: barrier => comm_barrier
    procedure :: barrier_nonbusy => comm_barrier_nonbusy

    ! Broadcast operations (generic interface)
    procedure, private :: bcast_scalar_real
    procedure, private :: bcast_array_real_1d
    procedure, private :: bcast_array_real_2d
    generic :: bcast => bcast_scalar_real, bcast_array_real_1d, bcast_array_real_2d

    ! High-level operation for MACE calculation results
    procedure :: broadcast_results => comm_broadcast_mace_results
  end type CommEnv

  ! Constructor interfaces
  interface comm_env_init
    module procedure comm_env_init_mpi
    module procedure comm_env_init_mpi_old
  end interface

contains

  ! ============================================================================
  ! Constructors
  ! ============================================================================

  function comm_env_init_mpi(mpi_comm) result(env)
    ! Initialize with MPI_f08 communicator
    type(MPI_Comm), intent(in) :: mpi_comm
    type(CommEnv) :: env
    integer :: ierr

    env%comm = mpi_comm
    call MPI_Comm_rank(env%comm, env%rank, ierr)
    if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Comm_rank failed")
    env%is_root = (env%rank == COMM_ROOT_RANK)
  end function comm_env_init_mpi

  function comm_env_init_mpi_old(mpi_comm_int) result(env)
    ! Initialize with old-style integer communicator
    integer, intent(in) :: mpi_comm_int
    type(CommEnv) :: env
    integer :: ierr

    env%comm%MPI_VAL = mpi_comm_int
    call MPI_Comm_rank(env%comm, env%rank, ierr)
    if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Comm_rank failed")
    env%is_root = (env%rank == COMM_ROOT_RANK)
  end function comm_env_init_mpi_old

  subroutine comm_env_free(env)
    ! Destructor (no-op for MPI version - don't free user's communicator)
    type(CommEnv), intent(inout) :: env
    ! Nothing to do - we don't own the communicator
  end subroutine comm_env_free

  ! ============================================================================
  ! Query methods
  ! ============================================================================

  pure function comm_get_rank(self) result(rank)
    class(CommEnv), intent(in) :: self
    integer :: rank
    rank = self%rank
  end function comm_get_rank

  pure function comm_is_root(self) result(is_root)
    class(CommEnv), intent(in) :: self
    logical :: is_root
    is_root = self%is_root
  end function comm_is_root

  ! ============================================================================
  ! Synchronization
  ! ============================================================================

  subroutine comm_barrier(self)
    class(CommEnv), intent(in) :: self
    integer :: ierr
    call MPI_Barrier(self%comm, ierr)
    if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Barrier failed")
  end subroutine comm_barrier

  subroutine comm_barrier_nonbusy(self, interval_usec)
    ! Non-busy-waiting barrier using sleep polling
    class(CommEnv), intent(in) :: self
    integer, intent(in) :: interval_usec

    type(MPI_Request) :: request
    type(MPI_Status) :: status
    integer :: ierr, sleep_ret
    logical :: completed

    ! Start non-blocking barrier
    call MPI_Ibarrier(self%comm, request, ierr)
    if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Ibarrier failed")

    ! Poll until complete, sleeping between checks
    completed = .false.
    do while (.not. completed)
      call MPI_Test(request, completed, status, ierr)
      if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Test failed")
      if (.not. completed) sleep_ret = usleep(interval_usec)
    end do
  end subroutine comm_barrier_nonbusy

  ! ============================================================================
  ! Broadcast operations (generic interface)
  ! ============================================================================

  subroutine bcast_scalar_real(self, buffer)
    class(CommEnv), intent(in) :: self
    real(dp), intent(inout) :: buffer
    integer :: ierr

    call MPI_Bcast(buffer, 1, MPI_DOUBLE_PRECISION, COMM_ROOT_RANK, &
        self%comm, ierr)
    if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Bcast failed")
  end subroutine bcast_scalar_real

  subroutine bcast_array_real_1d(self, buffer)
    class(CommEnv), intent(in) :: self
    real(dp), dimension(:), intent(inout) :: buffer
    integer :: ierr

    call MPI_Bcast(buffer, size(buffer), MPI_DOUBLE_PRECISION, &
        COMM_ROOT_RANK, self%comm, ierr)
    if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Bcast failed")
  end subroutine bcast_array_real_1d

  subroutine bcast_array_real_2d(self, buffer)
    class(CommEnv), intent(in) :: self
    real(dp), dimension(:, :), intent(inout) :: buffer
    integer :: ierr, total_size

    total_size = size(buffer, 1) * size(buffer, 2)
    call MPI_Bcast(buffer, total_size, MPI_DOUBLE_PRECISION, &
        COMM_ROOT_RANK, self%comm, ierr)
    if (ierr /= MPI_SUCCESS) call abort_on_error("MPI_Bcast failed")
  end subroutine bcast_array_real_2d

  ! ============================================================================
  ! High-level operations
  ! ============================================================================

  subroutine comm_broadcast_mace_results(self, total_energy, node_energy, &
      forces, virial)
    ! Broadcast all MACE calculation results in one call
    class(CommEnv), intent(in) :: self
    real(dp), intent(inout) :: total_energy
    real(dp), dimension(:), intent(inout) :: node_energy
    real(dp), dimension(:, :), intent(inout) :: forces
    real(dp), dimension(6), intent(inout) :: virial

    ! Non-busy barrier before broadcasts
    call self%barrier_nonbusy(10000)

    ! Broadcast all results
    call self%bcast(total_energy)
    call self%bcast(node_energy)
    call self%bcast(forces)
    call self%bcast(virial)
  end subroutine comm_broadcast_mace_results

  ! ============================================================================
  ! Utilities
  ! ============================================================================

  subroutine abort_on_error(msg)
    character(len = *), intent(in) :: msg
    write(*, '(A)') "MACE Communication Error: " // trim(msg)
    error stop
  end subroutine abort_on_error

  ! UNIX sleep function
  function usleep(useconds) bind(c)
    use iso_c_binding
    integer(c_int32_t), value :: useconds
    integer(c_int) :: usleep
  end function usleep

end module mace_comms
