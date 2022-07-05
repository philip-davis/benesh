#include<mpi.h>

#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<sys/prctl.h>
#include<unistd.h>

#ifdef USE_APEX
#include<apex.h>
#define APEX_FUNC_TIMER_START(fn) \
        apex_profiler_handle profiler0 = apex_start(APEX_FUNCTION_ADDRESS, &fn);
#define APEX_NAME_TIMER_START(num, name) \
        apex_profiler_handle profiler##num = apex_start(APEX_NAME_STRING, name);
#define APEX_TIMER_STOP(num) \
        apex_stop(profiler##num);
#else
#define APEX_TIMER_STOP(num) (void)0
#define APEX_NAME_TIMER_START(num, name) (void)0
#define APEX_FUNC_TIMER_START(fn) (void)0
#endif

#include "benesh.h"

#include "heat.h"

struct heateq {
    struct var *varU, *varDU;
    struct proc_dim *pdim;
    struct domain *dom;
    double dt;
};

double get_elapsed_sec(struct timeval *start, struct timeval *stop)
{
    return((stop->tv_sec - start->tv_sec) + (double)(stop->tv_usec - start->tv_usec) / 1000000.0);
}

int do_solve(benesh_app_id bnh, benesh_arg arg)
{
    struct heateq *heat = (struct heateq *)arg;

    fill_ghosts(heat->varU, heat->pdim);
    euler_solve(heat->varU, heat->varDU, heat->dom);

    return 0;
}

int do_advance(benesh_app_id bnh, benesh_arg arg)
{
    struct heateq *heat = (struct heateq *)arg;

    advance(heat->varU, heat->varDU, heat->dt);
    
    return 0;
}

void print_usage(char *name)
{
    fprintf(stderr, "Usage: %s <x ranks> <y ranks> <x grid pts> <y grid pts> <app name>\n", name);
}

int main(int argc, char **argv)
{
    double x0, x1, y0, y1;
    double *dom_lb, *dom_ub;
    double dims[2];
    uint64_t grid_size[2];
    int ndim;
    char *dom_name;
    int xgrdim, ygrdim, xdim, ydim;
    int xrank, xranks, yrank, yranks;
    double offset[2];
    int rank, size;
    struct var *varU, *varDU;
    struct proc_dim pdim;
    struct domain dom;
    int ts, maxts;
    char *app_name;
    double dt;
    benesh_app_id bnh;
    char tpoint[100];
    void *u_buf, *du_buf;
    struct heateq heat;
    struct timeval start, stop;
    double time, tavg, tmax, tmin;

    if(argc != 6) {
        print_usage(argv[0]);
        return 1;
    } 

    MPI_Init(NULL, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
 
    prctl(PR_SET_THP_DISABLE, 1, 0, 0, 0);

#ifdef USE_APEX
    apex_init("benesh heateq2d", rank, size);
#endif

    //domain setup
    xranks = atoi(argv[1]);
    yranks = atoi(argv[2]);
    xgrdim = atoi(argv[3]);
    ygrdim = atoi(argv[4]);
    app_name = argv[5];
    if(xranks * yranks != size) {
        fprintf(stderr, "should be %i ranks\n", xranks * yranks);
        return(1);
    }
    xdim = xgrdim / xranks;
    ydim = ygrdim / yranks;
    xrank = rank % xranks;
    yrank = rank / xranks;

    if(!rank) {
        fprintf(stderr, "going to init\n");
    }


    gettimeofday(&start, NULL);
    APEX_NAME_TIMER_START(1, "init phase");
    //varU = new_var(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 1);
    //varDU = new_var(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 0); 
    pdim.rank = rank;
    pdim.xrank = xrank;
    pdim.xranks = xranks;
    pdim.yrank = yrank;
    pdim.yranks = yranks;

    benesh_init(app_name, "heateq2d.xc", MPI_COMM_WORLD, 1, &bnh);
    APEX_NAME_TIMER_START(2, "domain config");
    benesh_get_var_domain(bnh, "u", &dom_name, &ndim, &dom_lb, &dom_ub);
    if(ndim != 2) {
        fprintf(stderr, "Dimensional mismatch with benesh config!\n");
        return 1;
    }


    dims[0] = (dom_ub[0] - dom_lb[0]) / xranks;
    dims[1] = (dom_ub[1] - dom_lb[1]) / yranks;
    offset[0] = dims[0] * xrank;
    offset[1] = dims[1] * yrank;
    grid_size[0] = xdim;
    grid_size[1] = ydim;
    benesh_bind_grid_domain(bnh, dom_name, offset, dims, grid_size, 1); 
    x0 = dom_lb[0];
    y0 = dom_lb[1];
    x1 = dom_ub[0];
    y1 = dom_ub[1];
    dom.xl = x0;
    dom.xh = x1;
    dom.yl = y0;
    dom.yh = y1;

    u_buf = benesh_get_var_buf(bnh, "u", NULL);
    du_buf = benesh_get_var_buf(bnh, "du", NULL);
    varU = new_var_noalloc(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 1, u_buf);
    varDU = new_var_noalloc(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 0, du_buf);
    APEX_TIMER_STOP(2);
    init_var(varU, &dom);

    dt = benesh_get_var_val(bnh, "dt");

    maxts = 5;

    heat.varU = varU;
    heat.varDU = varDU;
    heat.pdim = &pdim;
    heat.dom = &dom;
    heat.dt = dt;

    benesh_bind_method(bnh, "solve", do_solve, &heat);
    benesh_bind_method(bnh, "advance", do_advance, &heat);

    APEX_TIMER_STOP(1);
    gettimeofday(&stop, NULL);

    time = get_elapsed_sec(&start, &stop);
    MPI_Reduce(&time, &tavg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time, &tmin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if(rank == 0) {
        tavg /= size;
        fprintf(stderr, "init time, %lf, %lf, %lf\n", tavg, tmax, tmin);
    }

    double norm_part, norm_sum, du_norm;

    MPI_Barrier(MPI_COMM_WORLD);
    gettimeofday(&start, NULL);
    //fprintf(stderr, "%s starting timer 3\n", __func__);
    APEX_NAME_TIMER_START(3, "compute phase");
    for(ts = 1; ts < maxts; ts++) {
        sprintf(tpoint,"ts.%i", ts);
        //printf("start %s\n", tpoint);
        benesh_tpoint(bnh, tpoint);
        norm_part = get_l2_norm_sq(varDU);
        norm_sum = 0;
        MPI_Reduce(&norm_part, &norm_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        du_norm = sqrt(norm_sum);
        if(rank == 0) {
            fprintf(stderr, "timestep %i: l2 norm of du is %lf\n", ts, du_norm);
        }  
    }
    //fprintf(stderr, "%s stopping timer 3\n", __func__);
    APEX_TIMER_STOP(3);
    gettimeofday(&stop, NULL);

    time = get_elapsed_sec(&start, &stop);
    MPI_Reduce(&time, &tavg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time, &tmin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if(rank == 0) {
        tavg /= size;
        fprintf(stderr, "compute time, %lf, %lf, %lf\n", tavg, tmax, tmin);
        //printf("compute time: %lf s avg, %lf s max, %lf s min\n", tavg, tmax, tmin);
    }

#ifdef USE_APEX
    apex_finalize();
#endif

    benesh_fini(bnh);    

    if(rank == 0) {
        fprintf(stderr, "all done!\n");
    }
    MPI_Finalize();

    return(0);
 
    }
