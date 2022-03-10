struct var {
    int xdim, ydim, xgdim, ygdim;
    int x_off, y_off;
    double **data;
    double *data_raw;
    double *xlghost, *xhghost, *ylghost, *yhghost;
};

struct proc_dim {
    int rank;
    int xrank, yrank;
    int xranks, yranks;
};

struct domain {
    double xl, xh;
    double yl, yh;
};

struct var *new_var(int xdim, int ydim, int xgdim, int ygdim, int x_off, int y_off, int ghosts);

struct var *new_var_noalloc(int xdim, int ydim, int xgdim, int ygdim, int x_off, int y_off, int ghosts, double *buf);

void init_var(struct var *var, struct domain *dom);

void free_var(struct var *var);

void fill_ghosts(struct var *var, struct proc_dim *pdim);

void euler_solve(struct var *u, struct var *du, struct domain *dom);

void advance(struct var *u, struct var *du, double dt);

double get_l2_norm_sq(struct var *var);
