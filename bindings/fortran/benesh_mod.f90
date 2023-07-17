module benesh
    use iso_c_binding

    type benesh_app_id
        type(c_ptr) :: handle
    end type

contains
    subroutine benesh_init(comp_name, conf_file, comm, do_wait, handle, ierr)
        character*(*), intent(in) :: comp_name
        character*(*), intent(in) :: conf_file
        integer, intent(in) :: comm
        logical, intent(in) :: do_wait
        type(benesh_app_id), intent(out) :: handle
        integer, intent(out) :: ierr
        integer :: wait_arg
            
        if(do_wait) then
            wait_arg = 1
        else
            wait_arg = 0
        end if

        call benesh_init_f2c(comp_name // C_NULL_CHAR, conf_file // C_NULL_CHAR, comm, wait_arg, handle%handle, ierr)
    end subroutine

    subroutine benesh_fini(handle)
        type(benesh_app_id), intent(in) :: handle

        call benesh_fini_f2c(handle%handle)
    end subroutine

    subroutine benesh_bind_field_domain(handle, name)
        type(benesh_app_id), intent(in) :: handle
        character*(*), intent(in) :: name

        call benesh_bind_field_domain_f2c(handle%handle, name // C_NULL_CHAR)
    end subroutine

    subroutine benesh_bind_field_mpient(handle, name, index, rcn_file, comm, buffer, length, participates, field)
        type(benesh_app_id), intent(in) :: handle
        character*(*), intent(in) :: name, rcn_file
        integer, intent(in) :: index, comm, length, participates
        type(C_PTR), intent(in) :: buffer
        type(C_PTR), intent(out) :: field

        call benesh_bind_field_mpient_f2c(handle%handle, name // C_NULL_CHAR, index, rcn_file // C_NULL_CHAR, comm, buffer, length, participates, field)
    end subroutine

    subroutine benesh_bind_field_dummy(handle, name, index, participates, field)
        type(benesh_app_id), intent(in) :: handle
        integer, intent(in) :: index, participates
        character*(*), intent(in) :: name
        type(C_PTR), intent(out) :: field
        
        call benesh_bind_field_dummy_f2c(handle%handle, name // C_NULL_CHAR, index, participates, field)
    end subroutine

    subroutine benesh_tpoint(handle, name)
        type(benesh_app_id), intent(in) :: handle
        character*(*), intent(in) :: name

        call benesh_tpoint_f2c(handle%handle, name // C_NULL_CHAR)
    end subroutine
end module benesh
