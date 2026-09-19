/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Short multi-particle compression example derived from Jha et al. JMPS 2021
 * §4.3 (same M1, lc=R/5, horizon 3 lc, Rc=0.95 h, C-bar=100, plate vy=-0.06).
 * Not the paper’s 502-grain run: 4×3 equal circles, gaps just outside Rc so a
 * few grains enter contact under a stable Δt (≈ 0.25 h/c, same 0.2 μs as Table 2).
 *
 * -inbuiltMesh     Gmsh in-process (default; cup size follows the tight pack)
 * -fileMesh        frozen meshes in JHA2021_COMP_MESH_DIR
 * -finalTime / -numSteps   default T=0.004 s, 20000 steps (Δt=0.2 μs)
 * -requireContact  default on; fail unless a grain–grain pair enters Rc
 * -noRequireContact
 * -assertForce     fail unless plate reaction csv is nonzero
 */

#include "geom/geomObjectsUtil.h"
#include "inp/deckIncludes.h"
#include "inp/input.h"
#include "material/materialUtil.h"
#include "data/modelData.h"
#include "particle/baseParticle.h"
#include "particle/particleMpi.h"
#include "periDEMModel.h"
#include "postprocess/postprocess.h"
#include "util/function.h"
#include "util/io.h"
#include "util/parallelUtil.h"

#include <mpi.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
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

struct PackedParticle {
  double x{}, y{}, z{}, r{}, theta{};
};

std::vector<PackedParticle> generateCircularGrid(int ncols, int nrows, double x_lo,
                                                 double y_lo, double R, double gap) {
  std::vector<PackedParticle> particles;
  particles.reserve(static_cast<size_t>(ncols * nrows));
  const double step = 2.0 * R + gap;
  const double x0 = x_lo + gap + R;
  const double y0 = y_lo + gap + R;
  for (int j = 0; j < nrows; ++j) {
    for (int i = 0; i < ncols; ++i) {
      PackedParticle p;
      p.x = x0 + static_cast<double>(i) * step;
      p.y = y0 + static_cast<double>(j) * step;
      p.z = 0.0;
      p.r = R;
      p.theta = 0.0;
      particles.push_back(p);
    }
  }
  return particles;
}

// Default 4×3 = 12 grains; override with -nCols / -nRows for scaling studies.
json buildInputJson(const std::string &output_path_for_deck,
                    const std::filesystem::path &mesh_cir,
                    const std::filesystem::path &mesh_fixed,
                    const std::filesystem::path &mesh_moving, double final_time,
                    size_t num_steps, bool file_mesh, bool write_meshes,
                    size_t search_interval, int ncols, int nrows,
                    const std::string &mpi_strategy = "auto") {

  if (ncols < 1 || nrows < 1)
    throw std::runtime_error("jha2021_comp: ncols and nrows must be >= 1");
  const int ngrains = ncols * nrows;

  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double R = 0.001;
  const double mesh_size = R / 5.0;
  const double horizon = 3.0 * mesh_size;
  // Realized hmin on this disk is ~0.7 lc. Start just outside Rc so contact
  // is not present at t=0; plate + gravity pull a few pairs into Rc.
  const double h_est = 0.7 * mesh_size;
  const double Rc_est = 0.95 * h_est;
  const double particle_padding = 1.15 * Rc_est;
  const double wpd = particle_padding;
  const double rwp = horizon + wpd;
  constexpr double wall_vy = -0.06;

  const double Lin = 2.0 * particle_padding + 2.0 * R +
                     static_cast<double>(ncols - 1) * (2.0 * R + particle_padding);
  const double Win = 2.0 * particle_padding + 2.0 * R +
                     static_cast<double>(nrows - 1) * (2.0 * R + particle_padding);

  const double wall_t = rwp - wpd;
  const std::vector<double> mw_rect = {center[0] - wpd, Win, center[2],
                                       Lin + wpd, Win + wall_t, center[2]};
  const std::vector<double> cup_channel = {center[0] - rwp, center[1] - rwp,
                                           Lin + rwp, Win + wall_t, wall_t,
                                           center[2]};

  std::vector<geom::GeomData> pGeomVec(3);
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R, center[0], center[1], center[2]};
  pGeomVec[1].d_geomName = "open_rect_channel_2d";
  pGeomVec[1].d_geomParams = cup_channel;
  pGeomVec[2].d_geomName = "rectangle";
  pGeomVec[2].d_geomParams = mw_rect;

  std::vector<PackedParticle> packed =
      generateCircularGrid(ncols, nrows, 0.0, 0.0, R, particle_padding);
  if (static_cast<int>(packed.size()) != ngrains)
    throw std::runtime_error("jha2021_comp: grid count mismatch");

  const double poisson = 0.25;
  const double rho = 1200.0;
  const double K = 2.16e+7;
  const double E = material::toE(K, poisson);
  const double G = material::toGE(E, poisson);
  const double Gc = 50.0;
  const double Kn = 18.0 * util::harmonicMean(K, K) / (M_PI * std::pow(horizon, 5));

  const double dt = final_time / static_cast<double>(num_steps);
  const double c_wave = std::sqrt(E / rho);
  const double dt_cfl = h_est / c_wave;
  util::io::print(std::format(
      "jha2021_comp: N={} ({}x{}), gap={:.6e} (1.15 Rc_est), wpd={:.6e}, "
      "dt={:.6e} s, h/c={:.6e} s, dt/(h/c)={:.3f}\n",
      ngrains, ncols, nrows, particle_padding, wpd, dt, dt_cfl, dt / dt_cfl));

  const size_t num_outputs = 4;
  const size_t dt_out_n = std::max<size_t>(1, num_steps / num_outputs);
  const size_t test_dt_out_n = std::max<size_t>(1, dt_out_n / 10);

  for (auto &g : pGeomVec)
    geom::createGeomObject(g);

  const util::Point cfix = pGeomVec[1].d_geom_p->center();
  const util::Point cmov = pGeomVec[2].d_geom_p->center();
  const size_t n_pack = packed.size();
  const size_t n_wall_fixed = n_pack;
  const size_t n_wall_moving = n_pack + 1;
  const size_t n_total = n_pack + 2;

  auto modelDeckJson = inp::ModelDeck::getExampleJson(
      2, final_time, num_steps, "finite_difference", "central_difference", true, 2,
      "Multi_Particle", 0);
  modelDeckJson["MPI_Strategy"] = mpi_strategy;

  std::vector<std::string> out_tags = {"Displacement", "Velocity", "Force", "Damage_Z",
                                       "Damage", "Particle_ID", "Contact_Nodes"};
  auto outputDeckJson = inp::OutputDeck::getExampleJson(
      "vtu", output_path_for_deck, out_tags, dt_out_n, 1, true, "zlib", true,
      test_dt_out_n, "0", true);

  auto bcDeckJson =
      inp::BCDeck::getExampleJson(0, 2, 0, true, util::Point(0, -10, 0));
  bcDeckJson["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson(
      "Displacement_BC", false, geom::GeomData(), {n_wall_fixed}, {}, "", {}, "", {},
      {1, 2}, true, "", {});
  json set2;
  set2["Particle_List"] = std::vector<size_t>{n_wall_moving};
  set2["Direction"] = std::vector<size_t>{2};
  set2["Time_Function"] = json{{"Type", "linear"}, {"Parameters", std::vector<double>{wall_vy}}};
  set2["Spatial_Function"] = json{{"Type", "constant"}};
  bcDeckJson["Displacement_BC"]["Set_2"] = set2;

  json pDeckJson = json::object();
  pDeckJson["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);

  auto meshEntry = [&](const std::filesystem::path &f) -> json {
    if (file_mesh)
      return json{{"File", f.string()}};
    return json{{"File", f.string()},
                {"CreateMesh",
                 json{{"Flag", true},
                      {"Info", "gmsh_builtin_mesh"},
                      {"Mesh_Size", mesh_size},
                      {"Write_Mesh_File", write_meshes}}}};
  };
  pDeckJson["Mesh"] = json{{"Sets", 3},
                           {"Set_1", meshEntry(mesh_cir)},
                           {"Set_2", meshEntry(mesh_fixed)},
                           {"Set_3", meshEntry(mesh_moving)}};

  json matRoot = json{{"Sets", 2}};
  matRoot["Set_1"] =
      inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho, K, G, Gc, true, 1);
  matRoot["Set_2"] =
      inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho, K, G, Gc, true, 1);
  pDeckJson["Material"] = matRoot;

  json contact_base = inp::ContactPairDeck::getExampleJson(
      0.95, true, true, false, Kn, 0.95, 0.0, 1.0, 100.0, 1.0, 0.0, K);
  json contactRoot = inp::ParticleDeck::getParticleContactExampleJson(2);
  contactRoot["Set_1_1"] = contact_base;
  contactRoot["Set_1_2"] = contact_base;
  contactRoot["Set_2_2"] = contact_base;
  pDeckJson["Contact"] = contactRoot;
  pDeckJson["Neighbor"] =
      inp::PNeighborDeck::getExampleJson("simple_all", 5.0, search_interval, 0.5);

  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");
  pGenJson["Random_Rotation"] = false;
  pGenJson["Data"]["N"] = n_total;
  for (size_t pi = 0; pi < n_pack; ++pi) {
    const auto &p = packed[pi];
    pGenJson["Data"][std::to_string(pi)] = json{{"x", p.x},
                                                 {"y", p.y},
                                                 {"z", 0.0},
                                                 {"theta", 0.0},
                                                 {"s", 1.0},
                                                 {"geom_id", size_t(0)},
                                                 {"mat_id", size_t(0)},
                                                 {"contact_id", size_t(0)}};
  }
  pGenJson["Data"][std::to_string(n_pack)] = json{{"x", cfix.d_x},
                                                  {"y", cfix.d_y},
                                                  {"z", 0.0},
                                                  {"theta", 0.0},
                                                  {"s", 1.0},
                                                  {"geom_id", size_t(1)},
                                                  {"mat_id", size_t(1)},
                                                  {"contact_id", size_t(1)},
                                                  {"is_wall", true}};
  pGenJson["Data"][std::to_string(n_pack + 1)] = json{{"x", cmov.d_x},
                                                       {"y", cmov.d_y},
                                                       {"z", 0.0},
                                                       {"theta", 0.0},
                                                       {"s", 1.0},
                                                       {"geom_id", size_t(2)},
                                                       {"mat_id", size_t(1)},
                                                       {"contact_id", size_t(1)},
                                                       {"is_wall", true}};
  pDeckJson["Particle_Generation"] = pGenJson;

  util::io::print(std::format(
      "jha2021_comp: Lin={:.6f}, Win={:.6f}, horizon={:.6f}, plate v_y={}, Wall_Id={}\n",
      Lin, Win, horizon, wall_vy, n_wall_moving));

  return json{{"Comment", "jha2021_comp_contact"},
              {"Model", modelDeckJson},
              {"Output", outputDeckJson},
              {"Force_BC", bcDeckJson["Force_BC"]},
              {"Displacement_BC", bcDeckJson["Displacement_BC"]},
              {"Particle", pDeckJson["Particle"]},
              {"Mesh", pDeckJson["Mesh"]},
              {"Material", pDeckJson["Material"]},
              {"Contact", pDeckJson["Contact"]},
              {"Neighbor", pDeckJson["Neighbor"]},
              {"Particle_Generation", pDeckJson["Particle_Generation"]},
              {"Test", json{{"Test_Name", "compressive_test"},
                            {"Compressive_Test",
                             json{{"Wall_Id", n_wall_moving},
                                  {"Wall_Force_Direction", 2}}}}}};
}

void writeLocations(const std::filesystem::path &csv,
                    const std::vector<PackedParticle> &packed) {
  std::ofstream os(csv);
  os << "i, x, y, z, r, o\n";
  for (const auto &p : packed)
    os << std::format("0, {:.6f}, {:.6f}, {:.6f}, {:.6f}, 0.000000\n", p.x, p.y, p.z, p.r);
}

double maxAbsForceFromCsv(const std::filesystem::path &csv) {
  std::ifstream is(csv);
  if (!is)
    return 0.;
  std::string line;
  std::getline(is, line);
  double max_abs = 0.;
  while (std::getline(is, line)) {
    if (line.empty())
      continue;
    const auto c2 = line.rfind(',');
    if (c2 == std::string::npos)
      continue;
    max_abs = std::max(max_abs, std::abs(std::stod(line.substr(c2 + 1))));
  }
  return max_abs;
}

class GrainContactProbe : public postprocess::Postprocess {
public:
  GrainContactProbe(std::filesystem::path out_dir = {}, size_t interval = 0)
      : d_outDir(std::move(out_dir)), d_interval(interval) {
    if (!d_outDir.empty() && util::parallel::mpiRank() == 0) {
      std::filesystem::create_directories(d_outDir / "nodal");
      d_metricOs.open(d_outDir / "mpi_metric_ts.csv");
      d_metricOs << "step,t,max_u,com0x,com0y\n";
    }
  }

  void checkStop(data::ModelData &data) override {
    postprocess::Postprocess::checkStop(data);
    sampleMetric(data);
    if (data.d_particlesListTypeAll.size() < 3)
      return;
    double Rc = 0.;
    try {
      Rc = data.d_particleDeck_p->d_contactDeck.getContact(0, 0).d_contactR;
    } catch (...) {
      return;
    }
    if (Rc <= 0.)
      return;
    d_Rc = Rc;
    double min_gap = 1.0e9;
    int n_in = 0;
    const auto &all = data.d_particlesListTypeAll;
    for (size_t i = 0; i < all.size(); ++i) {
      if (all[i]->getGroupId("contact_id") != 0)
        continue;
      for (size_t j = i + 1; j < all.size(); ++j) {
        if (all[j]->getGroupId("contact_id") != 0)
          continue;
        const double gap = all[i]->getXCenter().dist(all[j]->getXCenter()) -
                           all[i]->d_geom_p->boundingRadius() -
                           all[j]->d_geom_p->boundingRadius();
        min_gap = std::min(min_gap, gap);
        if (gap < Rc)
          ++n_in;
      }
    }
    if (d_gap0 == 0. && min_gap < 1.0e8)
      d_gap0 = min_gap;
    if (min_gap < d_minGap) {
      d_minGap = min_gap;
      d_tMin = data.d_time;
    }
    if (n_in > d_maxPairs) {
      d_maxPairs = n_in;
      util::io::print(std::format(
          "grain contact: {} pair(s) in Rc at t={:.6e}, min_gap={:.6e}, Rc={:.6e}\n",
          n_in, data.d_time, min_gap, Rc));
    }
  }

  double minGap() const { return d_minGap; }
  double gap0() const { return d_gap0; }
  double Rc() const { return d_Rc; }
  double tMin() const { return d_tMin; }
  int maxPairs() const { return d_maxPairs; }

private:
  static bool ownsNode(const data::ModelData &data, size_t i) {
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

  void sampleMetric(data::ModelData &data) {
    if (d_outDir.empty() || d_interval == 0)
      return;
    const size_t nstep = data.currentStep();
    if (nstep % d_interval != 0 && nstep < data.numTimeSteps())
      return;
    double max_u = 0.;
    for (const auto &u : data.d_u)
      max_u = std::max(max_u, u.length());
    if (util::parallel::mpiSize() > 1)
      MPI_Allreduce(MPI_IN_PLACE, &max_u, 1, MPI_DOUBLE, MPI_MAX,
                    util::parallel::mpiComm());
    double com0x = 0., com0y = 0.;
    if (!data.d_particlesListTypeParticle.empty()) {
      const auto c = data.d_particlesListTypeParticle[0]->getXCenter();
      com0x = c.d_x;
      com0y = c.d_y;
    }
    if (util::parallel::mpiRank() == 0) {
      d_metricOs << std::format("{},{:.12e},{:.12e},{:.12e},{:.12e}\n", nstep,
                                data.d_time, max_u, com0x, com0y);
      d_metricOs.flush();
    }
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
    if (!d_wroteXref) {
      std::ofstream xr(d_outDir / "nodal" / "x_ref.bin", std::ios::binary);
      const uint32_t nn = static_cast<uint32_t>(n);
      xr.write(reinterpret_cast<const char *>(&nn), sizeof(nn));
      for (size_t i = 0; i < n; ++i) {
        const double p[3] = {data.d_xRef[i].d_x, data.d_xRef[i].d_y,
                             data.d_xRef[i].d_z};
        xr.write(reinterpret_cast<const char *>(p), sizeof(p));
      }
      d_wroteXref = true;
    }
    std::ofstream os(d_outDir / "nodal" / std::format("uv_{:06d}.bin", nstep),
                     std::ios::binary);
    const char magic[4] = {'P', 'D', 'U', 'V'};
    const uint32_t step32 = static_cast<uint32_t>(nstep);
    const uint32_t nn = static_cast<uint32_t>(n);
    os.write(magic, 4);
    os.write(reinterpret_cast<const char *>(&step32), sizeof(step32));
    os.write(reinterpret_cast<const char *>(&nn), sizeof(nn));
    os.write(reinterpret_cast<const char *>(buf.data()),
             static_cast<std::streamsize>(buf.size() * sizeof(double)));
  }

  double d_minGap = 1.0e9;
  double d_gap0 = 0.;
  double d_Rc = 0.;
  double d_tMin = 0.;
  int d_maxPairs = 0;
  std::filesystem::path d_outDir;
  size_t d_interval = 0;
  std::ofstream d_metricOs;
  bool d_wroteXref = false;
};

} // namespace

int main(int argc, char *argv[]) {

  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads"))
    nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else
    nThreads = std::thread::hardware_concurrency();
  util::parallel::initNThreads(nThreads);
  util::io::print(std::format("Number of threads = {}\n", util::parallel::getNThreads()));

  // Close ~1.08 Rc at vy=-0.06, then squeeze. 0.004 s × 0.06 m/s = 0.24 mm.
  double final_time = 0.004;
  size_t num_steps = 20000;
  if (input.cmdOptionExists("-finalTime"))
    final_time = std::stod(input.getCmdOption("-finalTime"));
  if (input.cmdOptionExists("-numSteps"))
    num_steps = std::stoul(input.getCmdOption("-numSteps"));
  size_t search_interval = 40;
  if (input.cmdOptionExists("-searchInterval"))
    search_interval = std::stoul(input.getCmdOption("-searchInterval"));
  int ncols = 4;
  int nrows = 3;
  if (input.cmdOptionExists("-nCols"))
    ncols = std::stoi(input.getCmdOption("-nCols"));
  if (input.cmdOptionExists("-nRows"))
    nrows = std::stoi(input.getCmdOption("-nRows"));
  std::string mpi_strategy = "auto";
  if (input.cmdOptionExists("-mpiStrategy"))
    mpi_strategy = input.getCmdOption("-mpiStrategy");
  util::io::print(std::format("MPI_Strategy = {}, pack = {}x{} ({} grains)\n",
                              mpi_strategy, ncols, nrows, ncols * nrows));

  namespace fs = std::filesystem;
  const fs::path cwd = fs::current_path();
  fs::path out_dir = cwd / "out";
  fs::path inp_dir = cwd / "inp";
  if (input.cmdOptionExists("-outputDir")) {
    fs::path p = input.getCmdOption("-outputDir");
    out_dir = p.is_absolute() ? std::move(p) : cwd / p;
  }
  if (input.cmdOptionExists("-inputDir")) {
    fs::path p = input.getCmdOption("-inputDir");
    inp_dir = p.is_absolute() ? std::move(p) : cwd / p;
  } else if (input.cmdOptionExists("-outputDir"))
    inp_dir = out_dir.parent_path() / "inp";

  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);

  const fs::path mesh_cir = inp_dir / "mesh_cir.msh";
  const fs::path mesh_fixed = inp_dir / "mesh_fixed_container.msh";
  const fs::path mesh_moving = inp_dir / "mesh_moving_container.msh";
  const bool file_mesh = input.cmdOptionExists("-fileMesh");
  const bool inbuilt = !file_mesh;
  const bool write_meshes = input.cmdOptionExists("-writeMeshes") || inbuilt;
  if (file_mesh) {
#ifndef JHA2021_COMP_MESH_DIR
    throw std::runtime_error("JHA2021_COMP_MESH_DIR is not set.");
#else
    const fs::path src(JHA2021_COMP_MESH_DIR);
    for (const char *name :
         {"mesh_cir.msh", "mesh_fixed_container.msh", "mesh_moving_container.msh"}) {
      const fs::path from = src / name;
      if (!fs::exists(from))
        throw std::runtime_error("Missing frozen mesh " + from.string());
      fs::copy_file(from, inp_dir / name, fs::copy_options::overwrite_existing);
    }
    util::io::print(std::format("Using frozen meshes from {}\n", src.string()));
#endif
  }

  const std::string output_path_for_deck = directoryPathWithTrailingSep(out_dir);
  auto inputJson = buildInputJson(output_path_for_deck, mesh_cir, mesh_fixed, mesh_moving,
                                  final_time, num_steps, file_mesh, write_meshes,
                                  search_interval, ncols, nrows, mpi_strategy);

  {
    std::ofstream os(inp_dir / "input.json");
    os << inputJson.dump(2);
  }

  {
    const double R = 0.001;
    const double mesh_size = R / 5.0;
    const double pad = 1.15 * 0.95 * 0.7 * mesh_size;
    writeLocations(inp_dir / "particle_locations.csv",
                   generateCircularGrid(ncols, nrows, 0.0, 0.0, R, pad));
  }

  auto deck = std::make_shared<inp::Input>(inputJson);
  PeriDEMModel dem(deck);
  const size_t ts_every = std::max<size_t>(1, num_steps / 100);
  auto probe = std::make_unique<GrainContactProbe>(out_dir, ts_every);
  GrainContactProbe *probe_p = probe.get();
  dem.setPostprocess(std::move(probe));
  dem.run(deck);

  // Under particle-MPI, ranks only refresh ghost centers; far grains stay stale.
  // Reduce contact probe metrics so requireContact is rank-consistent.
  int max_pairs = probe_p->maxPairs();
  double min_gap = probe_p->minGap();
  double gap0 = probe_p->gap0();
  if (util::parallel::mpiSize() > 1) {
    int max_pairs_g = 0;
    double min_gap_g = 0., gap0_g = 0.;
    MPI_Allreduce(&max_pairs, &max_pairs_g, 1, MPI_INT, MPI_MAX,
                  util::parallel::mpiComm());
    MPI_Allreduce(&min_gap, &min_gap_g, 1, MPI_DOUBLE, MPI_MIN,
                  util::parallel::mpiComm());
    MPI_Allreduce(&gap0, &gap0_g, 1, MPI_DOUBLE, MPI_MAX,
                  util::parallel::mpiComm());
    max_pairs = max_pairs_g;
    min_gap = min_gap_g;
    gap0 = gap0_g;
  }

  const float zmax =
      dem.d_Z.empty() ? 0.f : *std::max_element(dem.d_Z.begin(), dem.d_Z.end());
  util::io::print(std::format(
      "grain contact: gap0={}, min_gap={} (t={}), Rc={}, pairs_in_Rc={}, max Damage_Z={}\n",
      gap0, min_gap, probe_p->tMin(), probe_p->Rc(), max_pairs, zmax));

  const bool require_contact = !input.cmdOptionExists("-noRequireContact");
  if (require_contact) {
    const bool grains_touched = max_pairs > 0 && min_gap < gap0 - 1.0e-8;
    if (!grains_touched) {
      util::io::print("requireContact: no grain–grain pair entered the contact radius.\n");
      util::parallel::finalizeMpi();
      return EXIT_FAILURE;
    }
    if (!(gap0 > probe_p->Rc())) {
      util::io::print("requireContact: grains already in Rc at t=0; packing is too tight.\n");
      util::parallel::finalizeMpi();
      return EXIT_FAILURE;
    }
  }

  if (input.cmdOptionExists("-assertForce")) {
    // Rank 0 owns the reaction CSV; broadcast pass/fail.
    int force_ok = 1;
    if (util::parallel::mpiRank() == 0) {
      const fs::path csv = out_dir / "pp_compressive_test_0.csv";
      const double fmax = maxAbsForceFromCsv(csv);
      util::io::print(std::format("assertForce: max |plate reaction| = {} from {}\n", fmax,
                                  csv.string()));
      if (fmax <= 0.) {
        util::io::print("assertForce: plate reaction is zero.\n");
        force_ok = 0;
      }
    }
    if (util::parallel::mpiSize() > 1)
      MPI_Bcast(&force_ok, 1, MPI_INT, 0, util::parallel::mpiComm());
    if (!force_ok) {
      util::parallel::finalizeMpi();
      return EXIT_FAILURE;
    }
  }

  // Metric for serial vs MPI checks (rank 0). Allreduce max|u| for DOF-MPI.
  {
    double max_u = 0.;
    for (const auto &u : dem.d_u)
      max_u = std::max(max_u, u.length());
    double com0x = 0., com0y = 0.;
    if (!dem.d_particlesListTypeParticle.empty()) {
      const auto c = dem.d_particlesListTypeParticle[0]->getXCenter();
      com0x = c.d_x;
      com0y = c.d_y;
    }
    if (util::parallel::mpiSize() > 1)
      MPI_Allreduce(MPI_IN_PLACE, &max_u, 1, MPI_DOUBLE, MPI_MAX,
                    util::parallel::mpiComm());
    if (util::parallel::mpiRank() == 0) {
      std::ofstream os(out_dir / "mpi_metric.txt");
      os << std::format("{:.12e} {:.12e} {:.12e}\n", max_u, com0x, com0y);
      util::io::print(std::format("mpi_metric: max|u|={:.12e} grain0_com=({:.12e},{:.12e})\n",
                                  max_u, com0x, com0y));
    }
  }

  util::parallel::finalizeMpi();
  return EXIT_SUCCESS;
}
