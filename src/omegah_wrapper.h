#ifndef _BNH_OH_WRAPPER_
#define _BNH_OH_WRAPPER_

struct omegah_mesh;

struct omegah_mesh *new_oh_mesh(const char *meshFile);

struct rdv_ptn *create_oh_partition(struct omegah_mesh *meshp,
                                    const char *cpnFile);

void get_omegah_layout(struct omegah_mesh *meshp, struct rdv_ptn *rptn,
                       uint32_t **dest, uint32_t **offset, size_t *num_dest);
void mark_mesh_overlap(struct omegah_mesh *meshp, int min_class, int max_class);

int get_mesh_nverts(struct omegah_mesh *meshp);

#endif
