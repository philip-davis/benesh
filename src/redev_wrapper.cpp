
#include <redev.h>
#include <redev_comm.h>

struct rdv_comm;

extern "C" struct rdv_comm *new_rdv_comm(MPI_Comm *comm, const int rdvRanks,
                                         int isRdv)
{
    const auto dim = 2;
    auto ranks = redev::LOs(rdvRanks);
    auto cuts = redev::Reals(rdvRanks);
    auto ptn = redev::RCBPtn(dim, ranks, cuts);
    redev::Redev rdv(*comm, ptn, isRdv);
    rdv.Setup();
    std::string name = "rendezvous";
    return (struct rdv_comm *)(new redev::AdiosComm<redev::LO>(
        *comm, ranks.size(), rdv.getToEngine(), rdv.getIO(), name));
}

extern "C" void rdv_send(struct rdv_comm *comm, int count, int32_t *dest,
                         int32_t *offset, size_t buflen, int32_t *buffer)
{
    redev::AdiosComm<redev::LO> *rdv_comm = (redev::AdiosComm<redev::LO> *)comm;
    redev::LOs destv(dest, dest + count);
    redev::LOs offsetv(offset, offset + count + 1);
    redev::LOs msgs(buffer, buffer + buflen);
    rdv_comm->Pack(destv, offsetv, msgs.data());
    rdv_comm->Send();
}

extern "C" void rdv_recv(struct rdv_comm *comm, int knownSizes, void **buffer)
{
    redev::AdiosComm<redev::LO> *rdv_comm = (redev::AdiosComm<redev::LO> *)comm;
    redev::LO *msgs;
    redev::GOs rdvSrcRanks;
    redev::GOs offsets;
    size_t msgStart, msgCount;

    rdv_comm->SetVerbose(5);
    rdv_comm->Unpack(rdvSrcRanks, offsets, msgs, msgStart, msgCount,
                 (bool)knownSizes);
}
