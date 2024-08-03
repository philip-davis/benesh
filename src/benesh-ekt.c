#include "benesh.h"
#include "benesh-logging.h"
#include "benesh-types.h"
#include <ekt.h>

#if defined(__cplusplus)
extern "C" {
#endif

int benesh_init_ekt(struct benesh_handle *bnh)
{
    int err;

    DEBUG_OUT("initializing EKT.\n");

    CHECK_ZERO(ekt_init(&bnh->ekth, bnh->name, bnh->mycomm, bnh->mid), BNH_EEKT, err_out, "ekt_init failed with %i\n", err);
    //CHECK_ZERO(ekt_register(bnh->ekth, BNH_EKT_WORK, serialize_work, deserialize_work, bnh, &bnh->work_type), BNH_EEKT, err_out, "ekt_register failed with %i\n", err);

    return(0);
    
err_out:
    return(err);
}

#if defined(__cplusplus)
}
#endif