#ifndef _BENESH_COHORT_H
#define _BENESH_COHORT_H

#define BNH_COMP_NULL -1

struct benesh_cohort;

struct benesh_component;

int benesh_comp_disconnect(struct benesh_cohort *bco, int comp_id);

int benesh_connected_count(struct benesh_cohort *bco, int *count);

struct bnh_pvec *benesh_get_components(struct benesh_cohort *bco);

struct benesh_component *benesh_comp_get_by_id(struct benesh_cohort *bco,
                                               int comp_id);

int benesh_get_component_count(struct benesh_cohort *bco);

int benesh_comp_is_me(struct benesh_component *comp, int *flag);

char *benesh_comp_name(struct benesh_component *comp);

int benesh_comp_id(struct benesh_component *comp, int *id);

int benesh_comp_any(struct benesh_cohort *bco, int *eout);

#endif //_BENESH_COHORT_H