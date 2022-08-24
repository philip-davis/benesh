#ifndef __BNH_CPL_WRAPPER_H_
#define __BNH_CPL_WRAPPER_H_

#include<mpi.h>

#include<inttypes.h>
#include<vector>

#include<wdmcpl.h>

#include <Omega_h_mesh.hpp>

#include "omegah_wrapper.h"
#include "redev_wrapper.h"

extern "C" struct omegah_array *mark_mesh_overlap(struct omegah_mesh *meshp, int min_class, int max_class);
extern "C" struct omegah_array *mark_server_mesh_overlap(struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class);

struct cpl_hndl {
    wdmcpl::Coupler *cpl;
    bool server;
    union {
       Omega_h::Read<Omega_h::I8> *overlap_h;
       Omega_h::HostRead<Omega_h::I8> *srv_overlap_h; 
    };
    int64_t *buffer;
    size_t num_elem;
};

struct cpl_gid_field {
    wdmcpl::FieldCommunicatorT<wdmcpl::GO> *comm;
    std::vector<wdmcpl::GO> gid_field;
};

extern "C" struct cpl_hndl *create_cpl_hndl(const char *wfname, struct omegah_mesh *meshp, struct rdv_ptn *ptnp, int server)
{
    auto mesh = (Omega_h::Mesh *)meshp;
    auto ptn = (redev::ClassPtn *)ptnp;
    struct cpl_hndl *cpl_h = (struct cpl_hndl *)malloc(sizeof(*cpl_h));
    cpl_h->cpl = new wdmcpl::Coupler(wfname, (server ? wdmcpl::ProcessType::Server : wdmcpl::ProcessType::Client), MPI_COMM_WORLD, *ptn);
    cpl_h->server = (bool)server;
    return(cpl_h);
}

extern "C" void mark_cpl_overlap(struct cpl_hndl *cph, struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class)
{
    if(cph->server) {
        cph->srv_overlap_h = (Omega_h::HostRead<Omega_h::I8> *)mark_server_mesh_overlap(meshp, rptn, min_class, max_class);
    } else {
        cph->overlap_h = (Omega_h::Read<Omega_h::I8> *)mark_mesh_overlap(meshp, min_class, max_class);
    }
}

extern "C" struct cpl_gid_field *create_gid_field(const char *app_name, const char *field_name, struct cpl_hndl *cphp, struct omegah_mesh *meshp, void *field_buf)
{
    auto cpl_h = (wdmcpl::Coupler *)(cphp->cpl);
    auto &app = cpl_h->AddApplication(app_name);
    auto mesh = (Omega_h::Mesh *)meshp;
    struct cpl_gid_field *field = (struct cpl_gid_field *)malloc(sizeof(*field));
    field->gid_field = std::vector<wdmcpl::GO>();

    if(cphp->server) {
        field->comm = &app.AddField<wdmcpl::GO>(field_name, OmegaHGids{*mesh, *cphp->srv_overlap_h}, OmegaHReversePartition{*mesh}, SerializeServer{field->gid_field}, DeserializeServer{field->gid_field});
    } else {
        field->comm = &app.AddField<wdmcpl::GO>(field_name, OmegaHGids{*mesh, *cphp->overlap_h}, OmegaHReversePartition{*mesh}, SerializeOmegaHGids{*mesh, *cphp->overlap_h},  DeserializeOmegaH{*mesh, *cphp->overlap_h, NULL, NULL});
    }

    return(field);
}

extern "C" void cpl_send_field(struct cpl_gid_field *field)
{
    wdmcpl::FieldCommunicatorT<wdmcpl::GO> *comm = field->comm;
    comm->Send();
}

extern "C" void cpl_recv_field(struct cpl_gid_field *field, double **buffer, size_t *num_elem)
{
    wdmcpl::FieldCommunicatorT<wdmcpl::GO> *comm = field->comm;
    field->comm->Receive();
}

#endif
