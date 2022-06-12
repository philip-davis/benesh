
#include <redev.h>
#include <redev_comm.h>
#include <string.h>

struct rdv_comm;

static void printTime(std::string mode, double min, double max, double avg) {
    std::cerr << mode << " elapsed time min, max, avg (s): "
        << min << " " << max << " " << avg << "\n";
}

static void timeMinMaxAvg(double time, double& min, double& max, double& avg) {
    const auto comm = MPI_COMM_WORLD;
    int nproc;
    MPI_Comm_size(comm, &nproc);
    double tot = 0;
    MPI_Allreduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, comm);
    MPI_Allreduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, comm);
    MPI_Allreduce(&time, &tot, 1, MPI_DOUBLE, MPI_SUM, comm);
    avg = tot / nproc;
}


extern "C" struct rdv_comm *new_rdv_comm(MPI_Comm *comm, const int rdvRanks,
                                         int isRdv)
{
    const auto dim = 2;
    static auto ranks = redev::LOs(rdvRanks);
    static auto cuts = redev::Reals(rdvRanks);
    static auto ptn = redev::RCBPtn(dim, ranks, cuts);
    static redev::Redev rdv(*comm, ptn, isRdv);
    //rdv.Setup();
    static std::string name = "rendezvous";
    //return (struct rdv_comm *)(new redev::AdiosComm<redev::LO>(
    //    *comm, ranks.size(), rdv.getToEngine(), rdv.getIO(), name));
    adios2::Params params{ {"Streaming", "On"}, {"OpenTimeoutSecs", "2"}};
    return((struct rdv_comm *)(new  redev::CommPair<redev::LO>(rdv.CreateAdiosClient<redev::LO>(name,params,false))));
}

extern "C" void rdv_send(struct rdv_comm *comm, int count, int32_t *dest,
                         int32_t *offset, size_t buflen, int32_t *buffer)
{
    //fprintf(stderr, "in rdv_send\n");
    redev::CommPair<redev::LO> *commPair = (redev::CommPair<redev::LO> *)comm;
    redev::LOs destv(dest, dest + count);
    redev::LOs offsetv(offset, offset + count + 1);
    redev::LOs msgs(buffer, buffer + buflen);
    //fprintf(stderr, "packing\n");
    commPair->c2s.SetOutMessageLayout(destv, offsetv);
    //fprintf(stderr, "sending\n");
    commPair->c2s.Send(msgs.data());
}

extern "C" void rdv_recv(struct rdv_comm *comm, int rank, void **buffer)
{
    //fprintf(stderr, "in redv_recv\n");
    //redev::AdiosComm<redev::LO> *rdv_comm = (redev::AdiosComm<redev::LO> *)comm;
    redev::CommPair<redev::LO> *commPair = (redev::CommPair<redev::LO> *)comm;
    redev::LO *msgs;
    static redev::GOs rdvSrcRanks;
    static redev::GOs offsets;
    static size_t msgStart, msgCount;
    static int knownSizes = 0;

    std::stringstream ss;
    auto start = std::chrono::steady_clock::now();
    //rdv_comm->Unpack(rdvSrcRanks, offsets, msgs, msgStart, msgCount,
    //                 knownSizes);
    commPair->c2s.Recv();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    double min, max, avg;
    timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    ss << " unpack";
    std::string str = ss.str();
    if(!rank) printTime(str, min, max, avg); 
    knownSizes = 1;
    delete msgs;
}
