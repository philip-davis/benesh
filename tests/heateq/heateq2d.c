#include<mpi.h>

#include<math.h>
#include<stdio.h>
#include<stdlib.h>

#include "heat.h"

void print_usage(char *name)
{
    fprintf(stderr, "Usage: %s <x ranks> <y ranks> <lb x> <lb y> <ub x> <ub y> <x grid pts> <y grid pts>\n", name);
}

int main(int argc, char **argv)
{
    double x0, x1, y0, y1;
    int xgrdim, ygrdim, xdim, ydim;
    int xrank, xranks, yrank, yranks;
    int rank, size;
    struct var *varU, *varDU;
    struct proc_dim pdim;
    struct domain dom;
    int ts, maxts;
    double dt;

    if(argc != 9) {
        print_usage(argv[0]);
    } 

    MPI_Init(NULL, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
   
    //domain setup
    xranks = atoi(argv[1]);
    yranks = atoi(argv[2]);
    x0 = atof(argv[3]);
    y0 = atof(argv[4]);
    x1 = atof(argv[5]);
    y1 = atof(argv[6]);
    xgrdim = atoi(argv[7]);
    ygrdim = atoi(argv[8]);
    if(xranks * yranks != size) {
        fprintf(stderr, "should be %i ranks\n", xranks * yranks);
        return(1);
    }
    xdim = xgrdim / xranks;
    ydim = ygrdim / yranks;
    xrank = rank % xranks;
    yrank = rank / xranks;

    varU = new_var(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 1);
    varDU = new_var(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 0); 
    pdim.rank = rank;
    pdim.xrank = xrank;
    pdim.xranks = xranks;
    pdim.yrank = yrank;
    pdim.yranks = yranks;
    dom.xl = x0;
    dom.xh = x1;
    dom.yl = y0;
    dom.yh = y1;

    init_var(varU, &dom);

    dt = 0.000006;       
    
    maxts = 10000;

    double norm_part, norm_sum, du_norm;

    for(ts = 1; ts <= maxts; ts++) {
        fill_ghosts(varU, &pdim);    
        euler_solve(varU, varDU, &dom);
        advance(varU, varDU, dt);
        norm_part = get_l2_norm_sq(varDU);
        norm_sum = 0;
        MPI_Reduce(&norm_part, &norm_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        du_norm = sqrt(norm_sum);
        if(rank == 0) {
            printf("timestep %i: l2 norm of du is %lf\n", ts, du_norm);
        }        
        advance(varU, varDU, dt);
    }
     

    MPI_Finalize();

    return(0);
 
}
