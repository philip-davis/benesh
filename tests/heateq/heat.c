#include<mpi.h>

#include<math.h>
#include<stdio.h>
#include<stdlib.h>

#include "heat.h"

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
#endif

struct var *new_var(int xdim, int ydim, int xgdim, int ygdim, int x_off, int y_off, int ghosts)
{
    struct var *var = calloc(1, sizeof(*var));
    size_t len;
    int i;
  
    //fprintf(stderr, "%s starting func timer.\n", __func__); 
    APEX_FUNC_TIMER_START(new_var);
    var->xdim = xdim;
    var->ydim = ydim;
    var->xgdim = xgdim;
    var->ygdim = ygdim;
    var->x_off = x_off;
    var->y_off = y_off;
    
    len = ydim * sizeof(*var->data) + xdim * ydim * sizeof(**var->data);
    var->data = malloc(len);
    var->data_raw = (double *)&var->data[ydim];
    for(i = 0; i < ydim; i++) {
        var->data[i] = &var->data_raw[i * xdim];
    }

    if(ghosts) {
        if(x_off > 0) {
            var->xlghost = malloc(sizeof(*var->xlghost) * var->ydim);
        }
        if(y_off > 0) {
            var->ylghost = malloc(sizeof(*var->ylghost) * var->xdim);
        }
        if(x_off + xdim < xgdim) {
            var->xhghost = malloc(sizeof(*var->xhghost) * var->ydim);
        }
        if(y_off + ydim < ygdim) {
            var->yhghost = malloc(sizeof(*var->ylghost) * var->xdim);
        }
    }

    //fprintf(stderr, "%s stopping func timer.\n", __func__);
    APEX_TIMER_STOP(0);

    return(var);
}

struct var *new_var_noalloc(int xdim, int ydim, int xgdim, int ygdim, int x_off, int y_off, int ghosts, double *buf)
{
    struct var *var = calloc(1, sizeof(*var));
    int i;

    //fprintf(stderr, "%s starting func timer.\n", __func__);
    APEX_FUNC_TIMER_START(new_var_noalloc);

    var->xdim = xdim;
    var->ydim = ydim;
    var->xgdim = xgdim;
    var->ygdim = ygdim;
    var->x_off = x_off;
    var->y_off = y_off;

    var->data = malloc(ydim * sizeof(*var->data));
    var->data_raw = buf;
    for(i = 0; i < ydim; i++) {
        var->data[i] = &var->data_raw[i * xdim];
    }

    if(ghosts) {
        if(x_off > 0) {
            var->xlghost = malloc(sizeof(*var->xlghost) * var->ydim);
        }
        if(y_off > 0) {
            var->ylghost = malloc(sizeof(*var->ylghost) * var->xdim);
        }
        if(x_off + xdim < xgdim) {
            var->xhghost = malloc(sizeof(*var->xhghost) * var->ydim);
        }
        if(y_off + ydim < ygdim) {
            var->yhghost = malloc(sizeof(*var->ylghost) * var->xdim);
        }
    }

    //fprintf(stderr, "%s stopping func timer.\n", __func__);
    APEX_TIMER_STOP(0);

    return(var);
}


void init_var(struct var *var, struct domain *dom)
{
    int i, j;
    double x, y;

    //fprintf(stderr, "%s starting func timer.\n", __func__);
    APEX_FUNC_TIMER_START(init_var);

    for(i = 0; i < var->ydim; i++) {
        for(j = 0; j < var->xdim; j++) {
            x = ((var->x_off + j) / (double)var->xgdim) * (dom->xh - dom->xl) + dom->xl;
            y = ((var->y_off + i) / (double)var->ygdim) * (dom->yh - dom->yl) + dom->yl;
            var->data[i][j] = sin(x) + sin(y);
        }
    }

    //fprintf(stderr, "%s stopping func timer.\n", __func__);
    APEX_TIMER_STOP(0);
}

void free_var(struct var *var)
{
    free(var->data);
    if(var->xlghost) free(var->xlghost);
    if(var->xhghost) free(var->xhghost);
    if(var->ylghost) free(var->ylghost);
    if(var->yhghost) free(var->yhghost);
    free(var);
}

void fill_ghosts(struct var *var, struct proc_dim *pdim)
{
    MPI_Request rxl, rxh, ryl, ryh;
    double *xlbuf, *xhbuf;
    int i;

    //fprintf(stderr, "%s starting func timer.\n", __func__);
    APEX_FUNC_TIMER_START(fill_ghosts);

    if(var->xlghost) {
        xlbuf = malloc(sizeof(double) * var->ydim);
        for(i = 0; i < var->ydim; i++) {
            xlbuf[i] = var->data[i][0];
        }
        MPI_Isend(xlbuf, var->ydim, MPI_DOUBLE, pdim->rank - 1, 0, MPI_COMM_WORLD, &rxl);
    }
    
    if(var->xhghost) {
        xhbuf = malloc(sizeof(double) * var->ydim);
        for(i = 0; i < var->ydim; i++) {
            xhbuf[i] = var->data[i][var->xdim-1];
        }
        MPI_Isend(xhbuf, var->ydim, MPI_DOUBLE, pdim->rank + 1, 0, MPI_COMM_WORLD, &rxh); 
    }

    if(var->ylghost) {
        MPI_Isend(var->data[0], var->xdim, MPI_DOUBLE, pdim->rank - pdim->xranks, 0, MPI_COMM_WORLD, &ryl);
    }

    if(var->yhghost) {
        MPI_Isend(var->data[var->ydim-1], var->xdim, MPI_DOUBLE, pdim->rank + pdim->xranks, 0, MPI_COMM_WORLD, &ryh);
    }

    if(var->xlghost) {
        MPI_Recv(var->xlghost, var->ydim,  MPI_DOUBLE, pdim->rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if(var->xhghost) {
        MPI_Recv(var->xhghost, var->ydim, MPI_DOUBLE, pdim->rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if(var->ylghost) {
        MPI_Recv(var->ylghost, var->xdim, MPI_DOUBLE, pdim->rank - pdim->xranks, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if(var->yhghost) {
        MPI_Recv(var->yhghost, var->xdim, MPI_DOUBLE, pdim->rank + pdim->xranks, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if(var->xlghost) free(xlbuf);
    if(var->xhghost) free(xhbuf);

    //fprintf(stderr, "%s stopping func timer.\n", __func__);
    APEX_TIMER_STOP(0);
}

void euler_solve(struct var *u, struct var *du, struct domain *dom)
{
    double xl, xh, yl, yh;
    double h_x, h_y, d2x, d2y;
    int i, j;

    //fprintf(stderr, "%s starting func timer.\n", __func__);
    APEX_FUNC_TIMER_START(euler_solve);

    h_x = (dom->xh - dom->xl) / u->xgdim;
    h_y = (dom->yh - dom->yl) / u->ygdim;

    for(i = 0; i < u->ydim; i++) {
        for(j = 0; j < u->xdim; j++) {
            if(u->y_off + i == 0 ||
                    u->y_off + i == u->ygdim - 1 ||
                    u->x_off + j == 0 ||
                    u->x_off + j == u->xgdim - 1) {
                du->data[i][j] = 0;
                continue;
            }
            if(i == 0) {
                yl = u->ylghost[j];
            } else {
                yl = u->data[i-1][j];
            }
            if(i == u->ydim - 1) {
                yh = u->yhghost[j];
            } else {
                yh = u->data[i+1][j];
            }
            if(j == 0) {
                xl = u->xlghost[i];
            } else {
                xl = u->data[i][j-1];
            }
            if(j == u->xdim - 1) {
                xh = u->xhghost[i];
            } else {
                xh = u->data[i][j+1];
            }
            d2x = (xl + xh - (2 * u->data[i][j])) / (h_x * h_x);
            d2y = (yl + yh - (2 * u->data[i][j])) / (h_y * h_y);
            du->data[i][j] = d2x + d2y;
        }
    }
    //fprintf(stderr, "%s stopping func timer.\n", __func__);
    APEX_TIMER_STOP(0);
} 

void advance(struct var *u, struct var *du, double dt)
{
    int i, j;

    //fprintf(stderr, "%s starting func timer.\n", __func__);
    APEX_FUNC_TIMER_START(advance);

    for(i = 0; i < u->ydim; i++) {
        for(j = 0; j < u->xdim; j++) {
            u->data[i][j] += dt * du->data[i][j];
        }
    }
    //fprintf(stderr, "%s stopping func timer.\n", __func__);
    APEX_TIMER_STOP(0);
}

double get_l2_norm_sq(struct var *var)
{
    int i, j;
    double norm = 0;

    for(i = 0; i < var->ydim; i++) {
        for(j = 0; j < var->xdim; j++) {
            norm += var->data[i][j] * var->data[i][j];
        }
    }

    return(norm);
}
