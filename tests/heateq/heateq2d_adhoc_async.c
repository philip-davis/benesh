#include<mpi.h>

#include<inttypes.h>
#include<math.h>
#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/time.h>

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
#define APEX_FUNC_TIMER_START(fn) (void)0
#define APEX_NAME_TIMER_START(num, name) (void)0
#endif

#include<dspaces.h>

#include "heat.h"
struct couple_map {
    int l_lb[2], l_ub[2];
    uint64_t g_lb[2], g_ub[2];
    double xl, xh;
    double **buf;
    double *buf_raw;
    dspaces_client_t dsp;
    char stage_var[100];
    char peer_var[100];
};


double get_elapsed_sec(struct timeval *start, struct timeval *stop)
{
    return((stop->tv_sec - start->tv_sec) + (double)(stop->tv_usec - start->tv_usec) / 1000000.0);
}

struct couple_map *init_couple_map(struct var *u, struct domain *dom, struct domain *peer_dom, int isLeft)
{
    double gol_yl;
    double lol_xl, lol_xh, lol_yl, lol_yh;
    double l_xl, l_xh, l_yl, l_yh;
    double h_x, h_y;
    struct couple_map *cmap;
    int ol_size_x, ol_size_y, len;
    int i;

    APEX_FUNC_TIMER_START(init_couple_map);

    h_x = (dom->xh - dom->xl) / u->xgdim;
    h_y = (dom->yh - dom->yl) / u->ygdim;

    l_xl = dom->xl + h_x * u->x_off;
    l_xh = l_xl + h_x * (u->xdim - 1);
    l_yl = dom->yl + h_y * u->y_off;
    l_yh = l_yl + h_y * (u->ydim - 1);
    if(l_xh < peer_dom->xl || l_xl >= peer_dom->xh || l_yh < peer_dom->yl || l_yl > peer_dom->yh) {
        return(NULL);
    }


    cmap = calloc(1, sizeof(*cmap));
    cmap->xl = (dom->xl > peer_dom->xl) ? dom->xl : peer_dom->xl;
    cmap->xh = (dom->xh < peer_dom->xh) ? dom->xh : peer_dom->xh;
    gol_yl = (dom->yl > peer_dom->yl) ? dom->yl : peer_dom->yl;

    lol_xl = (l_xl > peer_dom->xl) ? l_xl : peer_dom->xl;
    lol_xh = (l_xh < peer_dom->xh) ? l_xh : peer_dom->xh;
    lol_yl = (l_yl > peer_dom->yl) ? l_yl : peer_dom->yl;
    lol_yh = (l_yh < peer_dom->yh) ? l_yh : peer_dom->yh;

    cmap->l_lb[0] = (lol_xl - l_xl) / h_x;
    cmap->l_lb[1] = (lol_yl - l_yl) / h_y;
    cmap->l_ub[0] = (lol_xh - l_xl) / h_x;
    cmap->l_ub[1] = (lol_yh - l_yl) / h_y;
    cmap->g_lb[0] = (lol_xl - cmap->xl) / h_x;
    cmap->g_lb[1] = (lol_yl - gol_yl) / h_y;
    cmap->g_ub[0] = (lol_xh - cmap->xl) / h_x;
    cmap->g_ub[1] = (lol_yh - gol_yl) / h_y;

    ol_size_y = (cmap->l_ub[1] - cmap->l_lb[1]) + 1;
    ol_size_x = (cmap->l_ub[0] - cmap->l_lb[0]) + 1;
    len =  ol_size_y * sizeof(*cmap->buf) + ol_size_x * ol_size_y * sizeof(**cmap->buf);
    cmap->buf = malloc(len);
    cmap->buf_raw = (double *)&cmap->buf[ol_size_y];
    for(i = 0; i < ol_size_y; i++) {
        cmap->buf[i] = &cmap->buf_raw[i * ol_size_y];
    }

    if(isLeft) {
        sprintf(cmap->stage_var, "left");
        sprintf(cmap->peer_var, "right");
    } else {
        sprintf(cmap->stage_var, "right");
        sprintf(cmap->peer_var, "left");
    }

    APEX_TIMER_STOP(0);
    return(cmap);
}

void read_peer(struct var *u, dspaces_client_t dsp, struct couple_map *cmap, int ts)
{
    double h_x;
    double frac, left, right;
    int i, j;

    APEX_FUNC_TIMER_START(read_peer);
    h_x = (cmap->xh - cmap->xl) / (cmap->l_ub[0] - cmap->l_lb[0]);
    dspaces_get(dsp, cmap->peer_var, ts, sizeof(double), 2, cmap->g_lb, cmap->g_ub, cmap->buf_raw, -1);

    for(i = 0; i <= cmap->l_ub[1] - cmap->l_lb[1]; i++) {
        for(j = 0; j <= cmap->l_ub[0] - cmap->l_lb[0]; j++) {
            frac = (h_x * (cmap->g_lb[0] + j)) / (cmap->xh - cmap->xl);
            left = u->data[cmap->l_lb[1] + i][cmap->l_lb[0] + j];
            right = cmap->buf[i][j];
            u->data[cmap->l_lb[1] + i][cmap->l_lb[0] + j] = left * (1 - frac) + right * frac;
        }
    }
    APEX_TIMER_STOP(0);
}

void write_stage(struct var *u, dspaces_client_t dsp, struct couple_map *cmap, int ts)
{
    int i, j;

    APEX_FUNC_TIMER_START(write_stage);
    for(i = 0; i <= cmap->l_ub[1] - cmap->l_lb[1]; i++) {
        for(j = 0; j <= cmap->l_ub[0] - cmap->l_lb[0]; j++) {
            cmap->buf[i][j] = u->data[cmap->l_lb[1] + i][cmap->l_lb[0] + j];
        }
    }

    dspaces_put(dsp, cmap->stage_var, ts, sizeof(double), 2, cmap->g_lb, cmap->g_ub, cmap->buf_raw);
    APEX_TIMER_STOP(0);
}

void print_usage(char *name)
{
    fprintf(stderr, "Usage: %s <x ranks> <y ranks> <lb x> <lb y> <ub x> <ub y> <x grid pts> <y grid pts>\n", name);
}

int main(int argc, char **argv)
{
    double x0, x1, y0, y1;
    double peer_x0, peer_x1;
    int xgrdim, ygrdim, xdim, ydim;
    int xrank, xranks, yrank, yranks;
    int rank, size;
    struct var *varU, *varDU;
    struct proc_dim pdim;
    struct domain dom, peer_dom;
    struct couple_map *cmap = NULL;
    int ts, maxts;
    dspaces_client_t dsp;
    double dt;
    struct timeval start, stop;
    double time, tavg, tmax, tmin;
   
    if(argc != 11) {
        print_usage(argv[0]);
    } 

    MPI_Init(NULL, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
  
#ifdef USE_APEX
    apex_init("adhoc heateq2d", rank, size);
#endif


    //domain setup
    xranks = atoi(argv[1]);
    yranks = atoi(argv[2]);
    x0 = atof(argv[3]);
    y0 = atof(argv[4]);
    x1 = atof(argv[5]);
    y1 = atof(argv[6]);
    xgrdim = atoi(argv[7]);
    ygrdim = atoi(argv[8]);
    peer_x0 = atof(argv[9]);
    peer_x1 = atof(argv[10]);
    if(xranks * yranks != size) {
        fprintf(stderr, "should be %i ranks\n", xranks * yranks);
        return(1);
    }
    xdim = xgrdim / xranks;
    ydim = ygrdim / yranks;
    xrank = rank % xranks;
    yrank = rank / xranks;

    gettimeofday(&start, NULL);
    APEX_NAME_TIMER_START(1, "init phase");
    varU = new_var(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 1);
    varDU = new_var(xdim, ydim, xgrdim, ygrdim, xdim * xrank, ydim * yrank, 0); 
    pdim.rank = rank;
    pdim.xrank = xrank;
    pdim.xranks = xranks;
    pdim.yrank = yrank;
    pdim.yranks = yranks;
    APEX_NAME_TIMER_START(2, "domain config");
    dom.xl = x0;
    dom.xh = x1;
    dom.yl = y0;
    dom.yh = y1;
    peer_dom.xl = peer_x0;
    peer_dom.xh = peer_x1;
    peer_dom.yl = y0;
    peer_dom.yh = y1;
    APEX_TIMER_STOP(2);
    init_var(varU, &dom);
    APEX_NAME_TIMER_START(3, "dspaces init");
    dspaces_init_mpi(MPI_COMM_WORLD, &dsp);
    APEX_TIMER_STOP(3);

    cmap = init_couple_map(varU, &dom, &peer_dom, dom.xl < peer_dom.xl);
    APEX_TIMER_STOP(1);
    gettimeofday(&stop, NULL);
    time = get_elapsed_sec(&start, &stop);
    MPI_Reduce(&time, &tavg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time, &tmin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if(rank == 0) {
        tavg /= size;
	    printf("init time, %lf, %lf, %lf\n", tavg, tmax, tmin);
        //printf("compute time: %lf s avg, %lf s max, %lf s min\n", tavg, tmax, tmin);
    }

    dt = 0.00000000001;
    
    maxts = 4;

    double norm_part, norm_sum, du_norm;

    MPI_Barrier(MPI_COMM_WORLD);
    gettimeofday(&start, NULL);
    APEX_NAME_TIMER_START(4, "compute phase");
    for(ts = 1; ts <= maxts; ts++) {
        if(cmap && ts > 1) {
            read_peer(varU, dsp, cmap, ts);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        fill_ghosts(varU, &pdim);    
        euler_solve(varU, varDU, &dom);
        advance(varU, varDU, dt);
        if(cmap) {
            write_stage(varU, dsp, cmap, ts);
        }
        norm_part = get_l2_norm_sq(varDU);
        norm_sum = 0;
        MPI_Reduce(&norm_part, &norm_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        du_norm = sqrt(norm_sum);
        if(rank == 0) {
            fprintf(stderr, "timestep %i: l2 norm of du is %lf\n", ts, du_norm);
        }        
    }
    APEX_TIMER_STOP(4);
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


    if(rank == 0) {
        dspaces_kill(dsp);
    }

    dspaces_fini(dsp);

#ifdef USE_APEX
    apex_finalize();
#endif

    MPI_Finalize();

    return(0);
 
}
