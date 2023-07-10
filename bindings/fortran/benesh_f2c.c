#include <FC.h>
#include <benesh.h>

#if defined(__cplusplus)
extern "C" {
#endif

void FC_GLOBAL(benesh_init_f2c,
        BENESH_INIT_F2C)(int *rank, benesh_app_id *bnh, int *ierr)
{
    *ierr = 0;
}

#if defined(__cplusplus)
}
#endif
