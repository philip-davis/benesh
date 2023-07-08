
#include <redev.h>
#include <redev_comm.h>
#include <string.h>

struct rdv_comm;
struct rdv_ptn;
struct omegah_mesh;

static void printTime(std::string mode, double min, double max, double avg)
{
    std::cerr << mode << " elapsed time min, max, avg (s): " << min << " "
              << max << " " << avg << "\n";
}

static void timeMinMaxAvg(double time, double &min, double &max, double &avg)
{
    const auto comm = MPI_COMM_WORLD;
    int nproc;
    MPI_Comm_size(comm, &nproc);
    double tot = 0;
    MPI_Allreduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, comm);
    MPI_Allreduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, comm);
    MPI_Allreduce(&time, &tot, 1, MPI_DOUBLE, MPI_SUM, comm);
    avg = tot / nproc;
}
/*
extern "C" struct rdv_comm *new_rdv_comm(MPI_Comm *comm, const int rdvRanks,
                                         int isRdv)
{
    const auto dim = 2;
    static auto ranks = redev::LOs(rdvRanks);
    static auto cuts = redev::Reals(rdvRanks);
    static auto ptn = redev::RCBPtn(dim, ranks, cuts);
    static redev::Redev rdv(*comm, ptn, (isRdv ? redev::ProcessType::Server : redev::ProcessType::Client));
    // rdv.Setup();
    static std::string name = "rendezvous";
    // return (struct rdv_comm *)(new redev::AdiosComm<redev::LO>(
    //    *comm, ranks.size(), rdv.getToEngine(), rdv.getIO(), name));
    adios2::Params params{{"Streaming", "On"}, {"OpenTimeoutSecs", "60"}};
#ifdef USE_DSP_REDEV
     return ((struct rdv_comm *)(new redev::BidirectionalComm<redev::LO>(
        rdv.CreateDSpacesClient<redev::LO>(name))));
#else
    return ((struct rdv_comm *)(new redev::BidirectionalComm<redev::LO>(
        rdv.CreateAdiosClient<redev::LO>(name, params, redev::TransportType::SST))));
#endif
}

extern "C" struct rdv_comm *new_rdv_comm_ptn(MPI_Comm *comm, const char *name,
                                             int isRdv, struct rdv_ptn *rptn)
{
    auto ptn = (redev::ClassPtn *)rptn;
    static redev::Redev rdv(MPI_COMM_WORLD, *ptn, (isRdv ? redev::ProcessType::Server : redev::ProcessType::Client)); //TODO
    adios2::Params params{{"Streaming", "On"}, {"OpenTimeoutSecs", "600"}};
#ifdef USE_DSP_REDEV
    return ((struct rdv_comm *)(new redev::BidirectionalComm<redev::GO>(
        rdv.CreateDSpacesClient<redev::GO>(std::string(name)))));
#else
    return ((struct rdv_comm *)(new redev::BidirectionalComm<redev::GO>(
        rdv.CreateAdiosClient<redev::GO>(std::string(name), params, redev::TransportType::SST))));
#endif
}

extern "C" void rdv_layout(struct rdv_comm *comm, int count, uint32_t *dest,
                           uint32_t *offset)
{
    redev::BidirectionalComm<redev::GO> *commPair = (redev::BidirectionalComm<redev::GO> *)comm;
    redev::LOs destv(dest, dest + count);
    redev::LOs offsetv(offset, offset + count + 1);
    commPair->SetOutMessageLayout(destv, offsetv);
}

extern "C" void rdv_send(struct rdv_comm *comm, int rank, void *buffer)
{
    redev::BidirectionalComm<redev::GO> *commPair = (redev::BidirectionalComm<redev::GO> *)comm;
    auto start = std::chrono::steady_clock::now();
    commPair->Send((redev::GO *)buffer);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    double min, max, avg;
    timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    std::stringstream ss;
    ss << " send";
    std::string str = ss.str();
    if(!rank)
        printTime(str, min, max, avg);

}

extern "C" void rdv_recv(struct rdv_comm *comm, int rank, void **buffer, size_t *buflen)
{
    redev::BidirectionalComm<redev::GO> *commPair = (redev::BidirectionalComm<redev::GO> *)comm;
    redev::GOs msgs;

    std::stringstream ss;
    auto start = std::chrono::steady_clock::now();
    msgs = commPair->Recv();
    *buflen = msgs.size();
    *buffer = malloc(msgs.size() * sizeof(*msgs.data()));
    memcpy(*buffer, msgs.data(), msgs.size() * sizeof(*msgs.data()));
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    double min, max, avg;
    timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    ss << " recv";
    std::string str = ss.str();
    if(!rank)
        printTime(str, min, max, avg);
}
*/
