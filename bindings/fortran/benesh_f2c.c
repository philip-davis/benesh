#include <FC.h>
#include <benesh.h>
#include <mpi.h>

#if defined(__cplusplus)
extern "C" {
#endif

void FC_GLOBAL(benesh_init_f2c,
        BENESH_INIT_F2C)(const char *comp_name, const char *conf_file,
                            int *fcomm, int *do_wait, benesh_app_id *bnh, int *ierr)
{
    MPI_Comm comm = MPI_Comm_f2c(*fcomm);
    *ierr = benesh_init(comp_name, conf_file, comm, *do_wait, bnh); 
}

void FC_GLOBAL(benesh_fini_f2c,
        BENESH_FINI_F2C)(benesh_app_id *bnh)
{
    benesh_fini(*bnh);
}

void FC_GLOBAL(benesh_bind_field_mpient_f2c,
        BENESH_BIND_FIELD_MPIENT_F2C)(benesh_app_id *bnh, const char *name, int *index, 
                                        const char *rcn_file, int *fcomm, void **buffer, 
                                        int *length, void **field)
{
     MPI_Comm comm = MPI_Comm_f2c(*fcomm);
     *field = benesh_bind_field_mpient(*bnh, name, *index, rcn_file, comm, *buffer, *length);
} 
        
void FC_GLOBAL(benesh_bind_field_dummy_f2c,
        BENESH_BIND_FIELD_DUMMY_F2C)(benesh_app_id *bnh, char *name, int *idx, void **field)
{
    *field = benesh_bind_field_dummy(*bnh, name, *idx);
}

void FC_GLOBAL(benesh_tpoint_f2c,
        BENESH_TPOINT_F2C)(benesh_app_id *bnh, char *name)
{
    benesh_tpoint(*bnh, name);
}

#if defined(__cplusplus)
}
#endif
