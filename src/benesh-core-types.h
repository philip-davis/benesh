#ifndef _BENESH_CORE_TYPES_H
#define _BENESH_CORE_TYPES_H

#include "benesh-cohort.h"
#include "benesh-ekt.h"
#include "benesh-targets.h"
#include "benesh-tasks.h"
#include "benesh-tpoint.h"
#include "benesh-types.h"
#include "parser/xc_config.h"

#include <margo.h>
#include <mpi.h>
#include <stdatomic.h>

struct benesh_handle {
    int rank;           // native
    int grank;          // native
    int comm_size;      // native
    MPI_Comm mycomm;    // native
    MPI_Comm gcomm;     // native
    int root_rank;      // native
    int root_drank;     // native
    char *name;         // native
    atomic_int f_ready; // native;
    int f_dummy;        // native
    struct bnhekt_handle *bekth;
    margo_instance_id mid; // native
    struct benesh_cohort *bco;
    struct benesh_taskman *btm;
    benesh_rulebook rules;
    benesh_tpoints tpoints;

    struct xc_config *conf;             // native?
    struct tpoint_handle *tph;          // touchpoint interface
    struct wf_component *comps;         // component interface
    struct wf_target *tgts;             // target interface
    struct xc_int_hash_map *known_objs; // object db
    int num_tgts;                       // target interface
    ABT_mutex db_mutex;                 // object db
    ABT_mutex data_mutex;               // object db
    ABT_cond data_cond;            // implmenetation internal to data handler
    struct work_node *wqueue_head; // scheduler interface
    struct work_node *wqueue_tail; // scheduler interface
    int gvar_count, ifvar_count;   // variable interface
    struct wf_var *gvars;          // variable interface
    struct wf_var *ifvars;         // variable interface
    int mth_count;                 // method interface
    struct wf_method *mths;        // method interface
    int dom_count;
    struct wf_domain *doms;
    dspaces_client_t dsp;
    int rdvRanks;
    int comp_id;
    int comp_count;

    struct wf_domain *dummy_dom;
    struct wf_component *dummy_comp;
    struct wf_var *dummy_vars;
    int num_dummy_vars;
};

#endif // _BENESH_CORE_TYPES_H