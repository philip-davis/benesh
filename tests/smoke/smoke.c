#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mpi.h>

#include "benesh.h"

int do_analyze(benesh_handle bnh, benesh_arg arg)
{
    fprintf(stderr, "%s\n", __func__);
    return (0);
}

int do_calc_incr(benesh_handle bnh, benesh_arg arg)
{
    fprintf(stderr, "%s\n", __func__);
    return (0);
}

int do_advance(benesh_handle bnh, benesh_arg arg)
{
    fprintf(stderr, "%s\n", __func__);
    return (0);
}

int main(int argc, char **argv)
{
    benesh_handle bnh;
    int rank;
    int comm_size;
    int ndim;
    double *lb, *ub;
    double grid_offset, grid_dims;
    uint64_t grid_points;
    char *dom_name;

    if(argc < 2) {
        printf("don't forget component name...\n");
    }

    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    benesh_init(argv[1], "smoke.xc", MPI_COMM_WORLD, &bnh);

    benesh_get_var_domain(bnh, "u", &dom_name, &ndim, &lb, &ub);
    printf("dom_name = %s\n", dom_name);

    grid_points = 8;
    grid_dims = ((*ub - *lb) / comm_size);
    grid_offset = grid_dims * rank;
    benesh_bind_domain(bnh, dom_name, &grid_offset, &grid_dims, &grid_points);

    if(strcmp(argv[1], "tptest") == 0) {
        fprintf(stderr, "I am sim\n");
        benesh_bind_method(bnh, "calc_incr", do_calc_incr, NULL);
        benesh_bind_method(bnh, "advance", do_advance, NULL);
        sleep(4);
        benesh_tpoint(bnh, "stage.3.2");
    } else {
        fprintf(stderr, "I am analyze\n");
        benesh_bind_method(bnh, "analyze", do_analyze, NULL);
        sleep(2);
        benesh_tpoint(bnh, "test.1");
        benesh_tpoint(bnh, "test.2");
    }

    fprintf(stderr, "done with touchpoints\n");

    MPI_Barrier(MPI_COMM_WORLD);

    sleep(2);

    benesh_fini(bnh);

    MPI_Finalize();

    return (0);
}
