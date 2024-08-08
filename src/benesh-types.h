#ifndef _BENESH_TYPES_H
#define _BENESH_TYPES_H

#include "benesh.h"
#include "util.h"
#include <abt.h>
#include <dspaces.h>
#include <ekt.h>
#include <margo.h>
#include <mpi.h>

#define BNH_WORK_OBJ 0
#define BNH_WORK_RULE 1
#define BNH_WORK_CHAIN 2
#define BNH_WORK_ANNOUNCE 3
#define BNH_WORK_PENDING 4

#define BNH_TYPE_INT 0
#define BNH_TYPE_FP 1

#define BNH_COMM_DSP 0
#define BNH_COMM_RDV_SRV 1
#define BNH_COMM_RDV_CLI 2

struct wf_domain {
    char *name;
    char *full_name;
    int dim;
    union {
        double *lb;
        int class_range[2];
    };
    double *ub;
    double *l_offset;
    uint64_t *l_grid_pts;
    double *l_grid_dims;
    int subdom_count;
    struct wf_domain *subdoms;
    struct omegah_mesh *mesh;
    struct rdv_comm *rdv;
    size_t rdv_dst_count;
    uint32_t *rdv_dest;
    uint32_t *rdv_offset;
    struct rdv_ptn *rptn;
    struct cpl_hndl *cph;
    size_t rdv_count;
    int type;
    int comm_type;
};

struct tpoint_rule {
    size_t nmappings;
    char **map_names;
    char **rule;
    int source;
    struct xc_list_node **tgts;
    int num_tgts;
};

struct tpoint_announce {
    uint32_t rule_id;
    int64_t *tp_vars;
    uint32_t comp_id;
};

struct work_announce {
    uint32_t comp_id;
    uint32_t rule_id;
    int64_t *tgt_vars;
    int32_t subrule_id;
};

struct tpoint_handle {
    ekt_id ekth;
    struct tpoint_rule *rules;
};

struct pq_obj {
    char **val;
    int len;
};

struct work_node;

struct obj_sub_node {
    struct obj_sub_node *next;
    int done;
    struct work_node *sub;
};

#define BNH_SUB_START 0
#define BNH_SUB_FINISH 1

struct obj_entry {
    struct obj_sub_node *subs;
    int realized;
    int pending;
};

struct var_ver {
    int var_id;
    struct xc_list_node *ver;
};

struct sub_rule {
    int type;
    int comp_id;
    struct xc_expr *expr;
    union {
        int var_id;
        int mth_id;
        char *inj_id;
    };
    int num_invar;
    struct var_ver *invars;
    int num_outvar;
    struct var_ver *outvars;
};

struct wf_target {
    char **obj_name;
    int name_len;
    int num_vars;
    int *tgt_locs;
    char **tgt_vars;
    int ndep;
    struct xc_list_node **deps;
    int num_subrules;
    struct xc_list_node **subrules;
    struct sub_rule *subrule;
};

struct work_node {
    struct work_node *prev, *next;
    int type;
    union {
        struct wf_target *tgt;
        struct work_node *link;
        struct benesh_rule *rule;
    };
    int subrule;
    int64_t *var_maps;
    dspaces_sub_t req;
    int announce;
    int realize;
    int sub_req;
    int deps;
    int num_invar;
    struct var_ver *invars;
    int num_outvar;
    struct var_ver *outvars;
    struct data_sub *ds;
};

struct wf_component {
    char *app;
    char *name;
    struct rdv_comm *rdv;
    struct app_hndl *cpl_apph;
    int recv_phase_open;
    int send_phase_open;
    int size;
    int isme;
};

struct wf_var {
    char *name;
    int type;
    size_t buf_size;
    union {
        double val;
        void *buf;
    };
    struct wf_domain *dom;
    struct xc_int_hash_map *versions;
    struct field_handle **fields;
    int num_fields;
    int comp_id;
    int comm_type;
};

struct wf_method {
    char *name;
    benesh_method method;
    void *arg;
};

struct benesh_handle;

#endif // _BENESH_TYPES_H