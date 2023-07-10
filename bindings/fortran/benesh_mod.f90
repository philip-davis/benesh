module benesh
    use iso_c_binding

    type benesh_app_id
        type(c_ptr) :: handle
    end type

contains
    subroutine benesh_init(rank, handle, ierr)
        integer, intent(in) :: rank
        type(benesh_app_id), intent(out) :: handle
        integer, intent(out) :: ierr

        call benesh_init_f2c(rank, handle%handle, ierr)
    end subroutine

end module benesh
