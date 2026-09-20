/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * Quick Multi_Particle MPI identity driver: two circles + one fixed floor wall.
 * Exercises DispBC on the wall, grain–grain contact, and grain–wall contact.
 * Dumps gathered final nodal u,v for serial / Particle-MPI / DOF-MPI compare.
 *
 * -mpiStrategy none|particle|dof
 * -outputDir / -inputDir
 * -nThreads
 */

#include "geom/geomIncludes.h"
#include "inp/deckIncludes.h"
#include "inp/input.h"
#include "material/materialUtil.h"
#include "particle/baseParticle.h"
#include "particle/particleMpi.h"
#include "periDEMModel.h"
#include "postprocess/postprocess.h"
#include "util/function.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "util/point.h"

#include <mpi.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string directoryPathWithTrailingSep(const std::filesystem::path &dir) {
  namespace fs = std::filesystem;
  fs::path n = fs::absolute(dir).lexically_normal();
  std::string s = n.string();
  if (!s.empty() && s.back() != '/' && s.back() != '\\')
    s += fs::path::preferred_separator;
  return s;
}

bool ownsNode(const data::ModelData &data, size_t i) {
  const int rank = util::parallel::mpiRank();
  if (data.d_pdDofMpi) {
    if (i >= data.d_pdNodePartition.size())
      return false;
    return static_cast<int>(data.d_pdNodePartition[i]) == rank;
  }
  for (const auto *p : data.d_particlesListTypeAll) {
    const size_t i0 = p->d_globStart;
    const size_t i1 = i0 + p->getNumNodes();
    if (i < i0 || i >= i1)
      continue;
    if (p->isWall())
      return rank == 0;
    return particle::isLocallyOwned(*p);
  }
  return rank == 0;
}

void dumpFinalNodal(const data::ModelData &data,
                    const std::filesystem::path &out_dir) {
  const size_t n = data.d_u.size();
  std::vector<double> buf(6 * n, 0.);
  for (size_t i = 0; i < n; ++i) {
    if (!ownsNode(data, i))
      continue;
    buf[6 * i + 0] = data.d_u[i].d_x;
    buf[6 * i + 1] = data.d_u[i].d_y;
    buf[6 * i + 2] = data.d_u[i].d_z;
    buf[6 * i + 3] = data.d_v[i].d_x;
    buf[6 * i + 4] = data.d_v[i].d_y;
    buf[6 * i + 5] = data.d_v[i].d_z;
  }
  if (util::parallel::mpiSize() > 1)
    MPI_Allreduce(MPI_IN_PLACE, buf.data(), static_cast<int>(buf.size()),
                  MPI_DOUBLE, MPI_SUM, util::parallel::mpiComm());
  if (util::parallel::mpiRank() != 0)
    return;

  std::filesystem::create_directories(out_dir / "nodal");
  {
    std::ofstream xr(out_dir / "nodal" / "x_ref.bin", std::ios::binary);
    const uint32_t nn = static_cast<uint32_t>(n);
    xr.write(reinterpret_cast<const char *>(&nn), sizeof(nn));
    for (size_t i = 0; i < n; ++i) {
      const double p[3] = {data.d_xRef[i].d_x, data.d_xRef[i].d_y,
                           data.d_xRef[i].d_z};
      xr.write(reinterpret_cast<const char *>(p), sizeof(p));
    }
  }
  {
    std::ofstream os(out_dir / "nodal" / "final_uv.bin", std::ios::binary);
    const char magic[4] = {'P', 'D', 'U', 'V'};
    const uint32_t step32 = static_cast<uint32_t>(data.currentStep());
    const uint32_t nn = static_cast<uint32_t>(n);
    os.write(magic, 4);
    os.write(reinterpret_cast<const char *>(&step32), sizeof(step32));
    os.write(reinterpret_cast<const char *>(&nn), sizeof(nn));
    os.write(reinterpret_cast<const char *>(buf.data()),
             static_cast<std::streamsize>(buf.size() * sizeof(double)));
  }
}

/** Records whether PP and wall contacts engaged at any step (floor top y=0). */
class ContactWitness : public postprocess::Postprocess {
public:
  void checkStop(data::ModelData &data) override {
    postprocess::Postprocess::checkStop(data);
    if (data.d_particlesListTypeParticle.size() < 2)
      return;
    const auto *g0 = data.d_particlesListTypeParticle[0];
    const auto *g1 = data.d_particlesListTypeParticle[1];
    double Rc = 0.;
    try {
      Rc = data.d_particleDeck_p->d_contactDeck.getContact(0, 0).d_contactR;
    } catch (...) {
      return;
    }
    d_Rc = Rc;
    const double gap = g0->getXCenter().dist(g1->getXCenter()) -
                       g0->d_geom_p->boundingRadius() -
                       g1->d_geom_p->boundingRadius();
    d_minPpGap = std::min(d_minPpGap, gap);
    if (gap < Rc)
      d_sawPp = true;

    double min_y = 1.e300;
    for (size_t i = 0; i < data.d_x.size(); ++i) {
      if (data.getParticleFromAllList(data.d_ptId[i])->isWall())
        continue;
      min_y = std::min(min_y, data.d_x[i].d_y);
    }
    if (util::parallel::mpiSize() > 1)
      MPI_Allreduce(MPI_IN_PLACE, &min_y, 1, MPI_DOUBLE, MPI_MIN,
                    util::parallel::mpiComm());
    d_minGrainY = std::min(d_minGrainY, min_y);
    if (min_y < Rc)
      d_sawWall = true;
  }

  void assertOk() const {
    int saw_pp = d_sawPp ? 1 : 0;
    int saw_wall = d_sawWall ? 1 : 0;
    double min_pp = d_minPpGap;
    double min_y = d_minGrainY;
    double rc = d_Rc;
    if (util::parallel::mpiSize() > 1) {
      int saw_pp_g = 0, saw_wall_g = 0;
      double min_pp_g = 0., min_y_g = 0., rc_g = 0.;
      MPI_Allreduce(&saw_pp, &saw_pp_g, 1, MPI_INT, MPI_MAX,
                    util::parallel::mpiComm());
      MPI_Allreduce(&saw_wall, &saw_wall_g, 1, MPI_INT, MPI_MAX,
                    util::parallel::mpiComm());
      MPI_Allreduce(&min_pp, &min_pp_g, 1, MPI_DOUBLE, MPI_MIN,
                    util::parallel::mpiComm());
      MPI_Allreduce(&min_y, &min_y_g, 1, MPI_DOUBLE, MPI_MIN,
                    util::parallel::mpiComm());
      MPI_Allreduce(&rc, &rc_g, 1, MPI_DOUBLE, MPI_MAX,
                    util::parallel::mpiComm());
      saw_pp = saw_pp_g;
      saw_wall = saw_wall_g;
      min_pp = min_pp_g;
      min_y = min_y_g;
      rc = rc_g;
    }
    if (!saw_pp)
      throw std::runtime_error(std::format(
          "particle-particle contact missing: min_gap={:.6e} Rc={:.6e}", min_pp,
          rc));
    if (!saw_wall)
      throw std::runtime_error(std::format(
          "particle-wall contact missing: min_grain_y={:.6e} Rc={:.6e}", min_y,
          rc));
    if (util::parallel::mpiRank() == 0)
      util::io::print(std::format(
          "contacts OK: min_pp_gap={:.6e} min_grain_y={:.6e} Rc={:.6e}\n",
          min_pp, min_y, rc));
  }

private:
  bool d_sawPp = false;
  bool d_sawWall = false;
  double d_minPpGap = 1.e300;
  double d_minGrainY = 1.e300;
  double d_Rc = 0.;
};

json buildInputJson(const std::string &output_path,
                    const std::filesystem::path &mesh_cir,
                    const std::filesystem::path &mesh_wall,
                    const std::string &mpi_strategy, double final_time,
                    size_t num_steps) {
  const double R = 0.001;
  const double mesh_size = R / 4.0;
  const double horizon = 3.0 * mesh_size;
  const double h_est = 0.7 * mesh_size;
  const double Rc_est = 0.95 * h_est;

  // Start inside PP Rc and near the floor; soft Kn (below) keeps contact stable.
  const double gap_pp = 0.5 * Rc_est;
  const double cy = R + 0.35 * Rc_est;
  const double cx0 = -(R + 0.5 * gap_pp);
  const double cx1 = +(R + 0.5 * gap_pp);

  const double wall_half = 3.5 * R;
  const double wall_thick = std::max(2.0 * mesh_size, horizon);

  geom::GeomData grain;
  grain.d_geomName = "circle";
  grain.d_geomParams = {R, 0., 0., 0.};
  geom::GeomData floor;
  floor.d_geomName = "rectangle";
  floor.d_geomParams = {-wall_half, -wall_thick, 0., wall_half, 0., 0.};

  const double poisson = 0.25;
  const double rho = 1200.0;
  const double K = 2.16e7;
  const double E = material::toE(K, poisson);
  const double G = material::toGE(E, poisson);
  const double Gc = 50.0;
  const double Kn =
      util::normalContactStiffness(K, K, horizon);

  auto model = inp::ModelDeck::getExampleJson(
      json{{"Dimension", 2},
           {"Final_Time", final_time},
           {"Time_Steps", num_steps},
           {"Discretization_Type", json{{"Spatial", "finite_difference"}, {"Time", "central_difference"}}},
           {"Populate_ElementNodeConnectivity", true},
           {"Quad_Approximation_Order", 2},
           {"Particle_Sim_Type", "Multi_Particle"},
           {"Seed", 0}});
  model["MPI_Strategy"] = mpi_strategy;
  model["Wall_Contact"] = "meshed";

  auto output = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", output_path},
           {"Tags", std::vector<std::string>({"Displacement", "Velocity", "Force",
                                "Particle_ID"})},
           {"Output_Interval", std::max<size_t>(1, num_steps)},
           {"Debug", 1},
           {"Perform_FE_Out", false},
           {"Compress_Type", "zlib"},
           {"Perform_Out", false},
           {"Test_Output_Interval", num_steps},
           {"Tag_PP", ""},
           {"PVD_Collection", false}});

  // Wall (particle index 2) fully fixed; downward IC on both grains.
  auto bc = inp::BCDeck::getExampleJson(
      json{{"Displacement_BC_Sets", 1},
           {"IC_Sets", 1},
           {"Gravity", util::Point(0, -10, 0).toVec()}});
  bc["Displacement_BC"]["Set_1"] = json{
      {"Particle_List", std::vector<size_t>{2}},
      {"Direction", std::vector<size_t>{1, 2}},
      {"Time_Function",
       {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
      {"Spatial_Function", {{"Type", "constant"}}},
      {"Zero_Displacement", true}};
  bc["IC"]["Set_1"] = inp::BCBaseDeck::getExampleJson(
      json{{"Type", "IC"},
           {"Particle_List", {0, 1}},
           {"IC_Type", "Constant_Velocity"},
           {"IC_Vector", {0., -0.02, 0.}}});

  auto particle = inp::ParticleDeck::getParticleGeomExampleJson({grain, floor});
  auto mesh_set = [&](const std::filesystem::path &f) {
    return json{{"File", f.string()},
                {"CreateMesh",
                 {{"Flag", true},
                  {"Info", "gmsh_builtin_mesh"},
                  {"Mesh_Size", mesh_size},
                  {"Write_Mesh_File", true}}}};
  };
  json mesh = {{"Sets", 2},
               {"Set_1", mesh_set(mesh_cir)},
               {"Set_2", mesh_set(mesh_wall)}};

  json material = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  material["Set_1"] = inp::MaterialDeck::getExampleJson(
      json{{"Type", "PDState"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", Gc},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 1}}}});
  material["Set_2"] = inp::MaterialDeck::getExampleJson(
      json{{"Type", "PDState"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", Gc},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 1}}}});

  json contact_base = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius_Factor", 0.95},
               {"Kn", Kn},
               {"Damping_On", false},
               {"Epsilon", 0.95},
               {"Friction_On", false},
               {"Friction_Coeff", 0.0},
               {"Kn_Factor", 1.0},
               {"Beta_n_Factor", 100.0},
               {"K", K}});
  // Soften contact spring so short MPI identity runs stay stable and bit-reproducible.
  contact_base["Kn_Factor"] = 1.0e-4;
  json contact = inp::ParticleDeck::getParticleContactExampleJson(2);
  contact["Set_1_1"] = contact_base;
  contact["Set_1_2"] = contact_base;
  contact["Set_2_2"] = contact_base;
  contact["Damping_Law"] = "off";
  contact["Friction_Law"] = "coulomb_simple";

  auto pgen = inp::PGenDeck::getExampleJson(json{{"Method", "From_File"}});
  pgen["Random_Rotation"] = false;
  pgen["Data"]["N"] = 3;
  pgen["Data"]["0"] = json{{"x", cx0},
                           {"y", cy},
                           {"z", 0.},
                           {"theta", 0.},
                           {"s", 1.},
                           {"geom_id", 0},
                           {"mat_id", 0},
                           {"contact_id", 0}};
  pgen["Data"]["1"] = json{{"x", cx1},
                           {"y", cy},
                           {"z", 0.},
                           {"theta", 0.},
                           {"s", 1.},
                           {"geom_id", 0},
                           {"mat_id", 0},
                           {"contact_id", 0}};
  pgen["Data"]["2"] = json{{"x", 0.},
                           {"y", -0.5 * wall_thick},
                           {"z", 0.},
                           {"theta", 0.},
                           {"s", 1.},
                           {"geom_id", 1},
                           {"mat_id", 1},
                           {"contact_id", 1},
                           {"is_wall", true}};

  util::io::print(std::format(
      "mpi_identity_twop_wall: R={:.4g} mesh={:.4g} Rc~{:.4g} gap_pp={:.4g} "
      "cy={:.4g} T={:.3g} N={}\n",
      R, mesh_size, Rc_est, gap_pp, cy, final_time, num_steps));

  return json{{"Model", model},
              {"Output", output},
              {"Force_BC", bc["Force_BC"]},
              {"Displacement_BC", bc["Displacement_BC"]},
              {"IC", bc["IC"]},
              {"Particle", particle},
              {"Mesh", mesh},
              {"Material", material},
              {"Contact", contact},
              {"Neighbor",
               inp::PNeighborDeck::getExampleJson(
          json{{"Update_Criteria", "simple_all"},
               {"Search_Factor", 5.0},
               {"Search_Interval", 1},
               {"Near_Bd_Nodes_Tol", 0.5}})},
              {"Particle_Generation", pgen}};
}

} // namespace

int main(int argc, char *argv[]) {
  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);

  unsigned n_threads = 1;
  if (input.cmdOptionExists("-nThreads"))
    n_threads = static_cast<unsigned>(std::stoi(input.getCmdOption("-nThreads")));
  util::parallel::initNThreads(n_threads);

  std::string mpi_strategy = "none";
  if (input.cmdOptionExists("-mpiStrategy"))
    mpi_strategy = input.getCmdOption("-mpiStrategy");

  namespace fs = std::filesystem;
  fs::path out_dir = fs::current_path() / "out";
  fs::path inp_dir = fs::current_path() / "inp";
  if (input.cmdOptionExists("-outputDir"))
    out_dir = input.getCmdOption("-outputDir");
  if (input.cmdOptionExists("-inputDir"))
    inp_dir = input.getCmdOption("-inputDir");
  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);

  // Short stable window with soft contact.
  const double final_time = 2.0e-4;
  const size_t num_steps = 1000;

  auto input_json =
      buildInputJson(directoryPathWithTrailingSep(out_dir),
                     inp_dir / "mesh_cir.msh", inp_dir / "mesh_wall.msh",
                     mpi_strategy, final_time, num_steps);
  {
    std::ofstream os(inp_dir / "input.json");
    os << input_json.dump(2);
  }

  auto deck = std::make_shared<inp::Input>(input_json);
  PeriDEMModel dem(deck);
  auto witness = std::make_unique<ContactWitness>();
  ContactWitness *witness_p = witness.get();
  dem.setPostprocess(std::move(witness));
  dem.init();
  dem.integrate();
  witness_p->assertOk();
  dumpFinalNodal(dem, out_dir);
  dem.close();

  if (util::parallel::mpiRank() == 0)
    util::io::print(std::format("mpi_identity_twop_wall done (strategy={})\n",
                                mpi_strategy));
  util::parallel::finalizeMpi();
  return 0;
}
