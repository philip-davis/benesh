
#include <redev.h>
#include <redev_comm.h>

struct rdv_comm;

extern "C" struct rdv_comm *new_rdv_comm(MPI_Comm *comm, const int rdvRanks,
                                         int isRdv)
{
    const auto dim = 2;
    static auto ranks = redev::LOs(rdvRanks);
    static auto cuts = redev::Reals(rdvRanks);
    static auto ptn = redev::RCBPtn(dim, ranks, cuts);
    static redev::Redev rdv(*comm, ptn, isRdv);
    rdv.Setup();
    static std::string name = "rendezvous";
    return (struct rdv_comm *)(new redev::AdiosComm<redev::LO>(
        *comm, ranks.size(), rdv.getToEngine(), rdv.getIO(), name));
}

extern "C" void rdv_send(struct rdv_comm *comm, int count, int32_t *dest,
                         int32_t *offset, size_t buflen, int32_t *buffer)
{
    //fprintf(stderr, "in rdv_send\n");
    redev::AdiosComm<redev::LO> *rdv_comm = (redev::AdiosComm<redev::LO> *)comm;
    redev::LOs destv(dest, dest + count);
    redev::LOs offsetv(offset, offset + count + 1);
    redev::LOs msgs(buffer, buffer + buflen);
    //fprintf(stderr, "packing\n");
    rdv_comm->Pack(destv, offsetv, msgs.data());
    //fprintf(stderr, "sending\n");
    rdv_comm->Send();
}

extern "C" void rdv_recv(struct rdv_comm *comm, void **buffer)
{
    //fprintf(stderr, "in redv_recv\n");
    redev::AdiosComm<redev::LO> *rdv_comm = (redev::AdiosComm<redev::LO> *)comm;
    static redev::LO *msgs;
    static redev::GOs rdvSrcRanks;
    static redev::GOs offsets;
    static size_t msgStart, msgCount;
    static int knownSizes = 0;

    rdv_comm->SetVerbose(5);
    rdv_comm->Unpack(rdvSrcRanks, offsets, msgs, msgStart, msgCount,
                     knownSizes);
    knownSizes = 1;
    
}
