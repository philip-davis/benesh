#ifndef _BENESH_TARGETS_H
#define _BENESH_TARGETS_H

#include <stdlib.h>

#include "util.h"

struct benesh_target;

struct benesh_rule;

typedef struct bnh_pvec *benesh_rulebook;

int benesh_rule_get_nvar(struct benesh_rule *rule, size_t *nvar);

int benesh_get_num_rules(benesh_rulebook rules);

int benesh_get_num_subrules(struct benesh_rule *rule);

struct benesh_rule *benesh_rule_get_by_id(benesh_rulebook rules, int rule_id);

#endif // _BENESH_TARGETS_H