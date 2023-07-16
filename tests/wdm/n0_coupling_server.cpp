#include <wdmcpl/wdmcpl.h>
#include <wdmcpl/types.h>
#include <Omega_h_file.hpp>
#include <Omega_h_for.hpp>
#include "test_support.h"
#include "benesh.h"
#include <wdmcpl/omega_h_field.h>
#include <wdmcpl/xgc_field_adapter.h>
#include <chrono>

using wdmcpl::Copy;
using wdmcpl::CouplerClient;
using wdmcpl::CouplerServer;
using wdmcpl::FieldEvaluationMethod;
using wdmcpl::FieldTransferMethod;
using wdmcpl::GO;
using wdmcpl::LO;
using wdmcpl::OmegaHFieldAdapter;

namespace ts = test_support;

// TODO: we should communicate the geometric ids in the overlap regions.
// is there a way to use the isOverlap functor to do this. This allows for
// maximum flexibility moving forward
//

[[nodiscard]]
static wdmcpl::ConvertibleCoupledField* AddField(wdmcpl::Application *application, const std::string& name, const std::string& path, Omega_h::Read<Omega_h::I8> is_overlap, const std::string& numbering, Omega_h::Mesh& mesh, int plane) {
      WDMCPL_ALWAYS_ASSERT(application != nullptr);
      std::stringstream field_name;
      field_name << name;
      if(plane >=0) {
        field_name<< "_" << plane;
      }
      return application->AddField(field_name.str(),
                   wdmcpl::OmegaHFieldAdapter<wdmcpl::Real>(
                   path+field_name.str(), mesh, is_overlap, numbering),
                   FieldTransferMethod::Copy, // to Omega_h
                   FieldEvaluationMethod::None,
                   FieldTransferMethod::Copy, // from Omega_h
                   FieldEvaluationMethod::None, is_overlap);
}

struct XGCAnalysis {
  using FieldVec = std::vector<wdmcpl::ConvertibleCoupledField*>;
  std::array<FieldVec,2> dpot;
  FieldVec pot0;
  std::array<FieldVec,2> edensity;
  std::array<FieldVec,2> idensity;
  wdmcpl::ConvertibleCoupledField* psi;
  wdmcpl::ConvertibleCoupledField* gids;
};

static void ReceiveFields(const std::vector<wdmcpl::ConvertibleCoupledField*> & fields) {
  for(auto* field : fields) {
    field->Receive();
  }
}
static void SendFields(const std::vector<wdmcpl::ConvertibleCoupledField*> & fields) {
  for(auto* field : fields) {
    field->Send();
  }
}
static void CopyFields(const std::vector<wdmcpl::ConvertibleCoupledField*> & from_fields,
                       const std::vector<wdmcpl::ConvertibleCoupledField*> & to_fields) {
  WDMCPL_ALWAYS_ASSERT(from_fields.size() == to_fields.size());
  for(size_t i=0; i<from_fields.size(); ++i) {
    const auto* from = from_fields[i]->GetFieldAdapter<wdmcpl::OmegaHFieldAdapter<wdmcpl::Real>>();
    auto* to = to_fields[i]->GetFieldAdapter<wdmcpl::OmegaHFieldAdapter<wdmcpl::Real>>();
    copy_field(from->GetField(), to->GetField());
  }
}

template <typename T>
static void AverageAndSetField(const wdmcpl::OmegaHField<T> & a, wdmcpl::OmegaHField<T> & b) {
  const auto a_data = get_nodal_data(a);
  const auto b_data = get_nodal_data(b);
  Omega_h::Write<T> combined_data(a_data.size());
  Omega_h::parallel_for(combined_data.size(), OMEGA_H_LAMBDA(size_t i) {
    combined_data[i] = (a_data[i] + b_data[i]) / 2.0;
  });
  auto combined_view = wdmcpl::make_array_view(Omega_h::Read<T>(combined_data));
  wdmcpl::set_nodal_data(b, combined_view);
}

/*
 * Takes the average of each pair of fields and sets the results in the the second
 * argument
 */
static void AverageAndSetFields(const std::vector<wdmcpl::ConvertibleCoupledField*> & from_fields,
                       const std::vector<wdmcpl::ConvertibleCoupledField*> & to_fields) {
  WDMCPL_ALWAYS_ASSERT(from_fields.size() == to_fields.size());
  for(size_t i=0; i<from_fields.size(); ++i) {
    const auto* from = from_fields[i]->GetFieldAdapter<wdmcpl::OmegaHFieldAdapter<wdmcpl::Real>>();
    auto* to = to_fields[i]->GetFieldAdapter<wdmcpl::OmegaHFieldAdapter<wdmcpl::Real>>();
    AverageAndSetField(from->GetField(),to->GetField());
  }
}

struct analyses {
    XGCAnalysis *core_analysis;
    XGCAnalysis *edge_analysis;
};

int copy_densities(benesh_app_id bnh, void *arg)
{
    struct analyses *ans = (struct analyses *)arg;

    CopyFields(ans->core_analysis->edensity[0], ans->edge_analysis->edensity[0]);
    CopyFields(ans->core_analysis->edensity[1], ans->edge_analysis->edensity[1]);
    CopyFields(ans->core_analysis->idensity[0], ans->edge_analysis->idensity[0]);
    CopyFields(ans->core_analysis->idensity[1], ans->edge_analysis->idensity[1]);
    
    return(0);
}

int copy_potentials(benesh_app_id bnh, void *arg)
{
    struct analyses *ans = (struct analyses *)arg;

    for(int i=0; i< ans->edge_analysis->dpot.size(); ++i){
        CopyFields(ans->edge_analysis->dpot[i], ans->core_analysis->dpot[i]);
        CopyFields(ans->edge_analysis->dpot[i], ans->core_analysis->dpot[i]);
    }

    return(0);
}

void SendRecvDensity(benesh_app_id bnh, XGCAnalysis& core_analysis, XGCAnalysis& edge_analysis, int rank, int step) {
    std::chrono::duration<double> elapsed_seconds;
    double min, max, avg;
    if(!rank) std::cerr<<"Send/Recv Density\n"; 
    auto sr_time1 = std::chrono::steady_clock::now();
    char tpname[100];
    
    // Gather
    sprintf(tpname, "gather_density.%i", step);
    benesh_tpoint(bnh, tpname);
     
    auto sr_time2 = std::chrono::steady_clock::now();
    elapsed_seconds = sr_time2-sr_time1;
    ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank) ts::printTime("Recv Density", min, max, avg);

    CopyFields(core_analysis.edensity[0], edge_analysis.edensity[0]);
    CopyFields(core_analysis.edensity[1], edge_analysis.edensity[1]);
    CopyFields(core_analysis.idensity[0], edge_analysis.idensity[0]);
    CopyFields(core_analysis.idensity[1], edge_analysis.idensity[1]);
    //AverageAndSetFields(core_analysis.edensity[0], edge_analysis.edensity[0]);
    //AverageAndSetFields(core_analysis.edensity[1], edge_analysis.edensity[1]);
    //AverageAndSetFields(core_analysis.idensity[0], edge_analysis.idensity[0]);
    //AverageAndSetFields(core_analysis.idensity[1], edge_analysis.idensity[1]);

    sr_time1 = std::chrono::steady_clock::now();
    elapsed_seconds = sr_time1-sr_time2;
    ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank) ts::printTime("Average Density", min, max, avg);
    sprintf(tpname, "scatter_density.%i", step);
    benesh_tpoint(bnh, tpname);
    auto sr_time3 = std::chrono::steady_clock::now();
    elapsed_seconds = sr_time3-sr_time1;
    ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank) ts::printTime("Send Density", min, max, avg);
}

void SendRecvPotential(benesh_app_id bnh, XGCAnalysis& core_analysis, XGCAnalysis& edge_analysis, int rank, int step) {
    std::chrono::duration<double> elapsed_seconds;
     double min, max, avg;
    if(!rank) std::cerr<<"Send/Recv Potential\n"; 
    auto sr_time3 = std::chrono::steady_clock::now();
    char tpname[100];

    // Gather
    sprintf(tpname, "get_potential.%i", step);
    benesh_tpoint(bnh, tpname);
    
    auto sr_time4 = std::chrono::steady_clock::now();
    elapsed_seconds = sr_time4-sr_time3;
    ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank) ts::printTime("Receive Potential", min, max, avg);
    // 2. Copy fields from Edge->Core
    for(int i=0; i<edge_analysis.dpot.size(); ++i){
      CopyFields(edge_analysis.dpot[i], core_analysis.dpot[i]);
      CopyFields(edge_analysis.dpot[i], core_analysis.dpot[i]);
    }
    CopyFields(edge_analysis.pot0, core_analysis.pot0);
    auto sr_time5 = std::chrono::steady_clock::now();
    elapsed_seconds = sr_time5-sr_time4;
    ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank) ts::printTime("Copy Potential", min, max, avg);
   
    sprintf(tpname, "send_potential.%i", step); 
    benesh_tpoint(bnh, tpname);
    auto sr_time6 = std::chrono::steady_clock::now();
    elapsed_seconds = sr_time6-sr_time5;
    ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank) ts::printTime("Send Potential", min, max, avg);
}


void omegah_coupler(benesh_app_id bnh, MPI_Comm comm, const char *meshFile,
                    const char *cpn_file, int nphi)
{
  std::chrono::duration<double> elapsed_seconds;
  double min, max, avg;
  int rank;
  MPI_Comm_rank(comm, &rank);
  auto time1 = std::chrono::steady_clock::now();
  char *dom_name;

  XGCAnalysis core_analysis;
  XGCAnalysis edge_analysis;

  struct analyses ans = {&core_analysis, &edge_analysis}; 

  benesh_get_var_domain(bnh, "psi\\core", &dom_name, NULL, NULL, NULL);
  benesh_bind_mesh_domain(bnh, dom_name, meshFile, cpn_file, 0);
  benesh_bind_method(bnh, "xfr_density", copy_densities, &ans);
  benesh_bind_method(bnh, "xfr_potential", copy_potentials, &ans);
  auto time2 = std::chrono::steady_clock::now();
  elapsed_seconds = time2-time1;
  ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
  if(!rank) ts::printTime("Initialize Coupler/Mesh", min, max, avg);

  std::cerr << "ADDING FIELDS\n";
  core_analysis.psi = (wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "psi\\core", NULL, 0);
  edge_analysis.psi = (wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "psi\\edge", NULL, 0);
  core_analysis.gids = (wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "gid_debug\\core",NULL,  0);
  edge_analysis.gids = (wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "gid_debug\\edge",NULL,  0);

  for (int i = 0; i < nphi; ++i) {
    //bind vars with benesh
    core_analysis.dpot[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "dpot_0_plane\\core", &i, 1));
    core_analysis.dpot[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "dpot_1_plane\\core", &i, 1));
    core_analysis.pot0.push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "pot0_plane\\core", &i, 1));
    core_analysis.edensity[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "edensity_1_plane\\core", &i, 1));
    core_analysis.edensity[1].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "edensity_2_plane\\core", &i, 1));
    core_analysis.idensity[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "idensity_1_plane\\core", &i, 1));
    core_analysis.idensity[1].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "idensity_2_plane\\core", &i, 1));

    edge_analysis.dpot[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "dpot_0_plane\\edge", &i, 1));
    edge_analysis.dpot[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "dpot_1_plane\\edge", &i, 1));
    edge_analysis.pot0.push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "pot0_plane\\edge", &i, 1));
    edge_analysis.edensity[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "edensity_1_plane\\edge",  &i, 1));
    edge_analysis.edensity[1].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "edensity_2_plane\\edge",  &i, 1));
    edge_analysis.idensity[0].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "idensity_1_plane\\edge",  &i, 1));
    edge_analysis.idensity[1].push_back((wdmcpl::ConvertibleCoupledField *)benesh_bind_var_mesh(bnh, "idensity_2_plane\\edge",  &i, 1));
  }
  auto time3 = std::chrono::steady_clock::now();
  elapsed_seconds = time3-time2;
  ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
  if(!rank) ts::printTime("Add Meshes", min, max, avg);

  benesh_tpoint(bnh, "init");
  auto time4 = std::chrono::steady_clock::now();
  elapsed_seconds = time4-time3;
  ts::timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
  if(!rank) ts::printTime("Receive Psi", min, max, avg);
  int step = 0;
  while (true) {
    std::stringstream ss;
    SendRecvDensity(bnh, core_analysis, edge_analysis, rank, step);
    SendRecvPotential(bnh, core_analysis, edge_analysis, rank, step);
    step++;
  }
}

int main(int argc, char** argv)
{
  auto lib = Omega_h::Library(&argc, &argv);
  auto world = lib.world();
  const int rank = world->rank();
  int size = world->size();
  //Kokkos::ScopeGuard kokkos{};
  if (argc != 4) {
    if (!rank) {
      std::cerr << "Usage: " << argv[0]
                << "</path/to/omega_h/mesh> "
                   "</path/to/partitionFile.cpn> "
                   "sml_nphi_total";
    }
    exit(EXIT_FAILURE);
  }

  const auto meshFile = argv[1];
  const auto classPartitionFile = argv[2];
  const int sml_nphi_total = std::atoi(argv[3]);

  benesh_app_id bnh;
  benesh_init("coupler", "wdm.xc", MPI_COMM_WORLD, 1, &bnh);
  
  //Omega_h::Mesh mesh(&lib);
  //Omega_h::binary::read(meshFile, lib.world(), &mesh);
  //MPI_Comm mpi_comm = lib.world()->get_impl();
  MPI_Comm mpi_comm = MPI_COMM_WORLD;
  omegah_coupler(bnh, mpi_comm, meshFile, classPartitionFile, sml_nphi_total);

  benesh_fini(bnh);
  return 0;
}
