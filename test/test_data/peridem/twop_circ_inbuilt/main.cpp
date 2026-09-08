/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * Two-particle circle test with fully in-process setup:
 * - No input file on disk (deck built in C++ only).
 * - Particle meshes from in-process Gmsh (CreateMesh.Info = gmsh_builtin_mesh).
 *
 * Default layout under the current working directory:
 *   ./out/   — VTU results and log.txt (Output.Path must end with a separator)
 *   ./inp/   — input.json (full deck) and mesh_cir_1.msh, mesh_cir_2.msh
 *
 * With PVD_Collection enabled (default here), ./out/output.pvd lists all particle
 * VTU timesteps — open output.pvd in ParaView for a single time animation (not one
 * monolithic VTU; VTK uses one VTU per snapshot plus this index file).
 *
 * -outputDir <path>  sets the output directory (absolute or relative to cwd).
 *                    Input files go to a sibling ./inp next to that directory's parent
 *                    if -outputDir points at .../out; otherwise use -inputDir.
 * -inputDir <path>   optional; defaults to cwd/inp, or <parent of outputDir>/inp when
 *                    -outputDir is used.
 * -finalTime <t>     integration end time (default 0.002; example_twop_circ_contact uses 0.012).
 * -numSteps <n>      number of steps (default 6000; example uses 36000). dt = finalTime/numSteps.
 * -requireContact    fail if max Damage_Z is 0 (particles never damaged / no contact).
 * -jha2021Table2     Jha et al. JMPS 2021 Table 2 test 1 (ε_n = 1, CR = 1).
 * -jha2021Table2Test <n>  Table 2 test n = 1..5 (sets time, IC, ε_n, CR, damping).
 * -inbuiltMesh       with -jha2021Table2*: Gmsh in-process mesh (Mesh_Size=R/5)
 *                    instead of the frozen v0.1.0 .msh. Same dt, IC, horizon, μ.
 * -epsN <ε>          contact Epsilon (paper ε̄_n). ε < 1 turns damping on (C̄ = 100).
 * -betaNFactor <C>   contact Beta_n_Factor (paper C̄, default 100).
 * -zeroIC            drop from rest (paper Fig. 5); default is a free-fall velocity IC.
 * -assertCR <ref>    fail unless sqrt(H1/H0) is within -crTol of <ref>.
 * -crTol <tol>       absolute CR tolerance (default 0.1).
 */

#include "inp/deckIncludes.h"
#include "util/io.h"
#include "util/function.h"
#include "material/materialUtil.h"
#include "periDEMModel.h"
#include "particle/baseParticle.h"
#include "postprocess/postprocess.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

// Jha et al. JMPS 2021 Table 2: same R, M1, H0; CR vs ε̄_n (paper Epsilon).
struct Jha2021Table2 {
  double eps;
  double cr;
};

Jha2021Table2 jha2021Table2(int test_id) {
  static const Jha2021Table2 k[] = {
      {1.0, 1.0}, {0.95, 0.946}, {0.9, 0.893}, {0.85, 0.845}, {0.8, 0.796}};
  if (test_id < 1 || test_id > 5)
    throw std::runtime_error("jha2021Table2Test must be 1..5");
  return k[test_id - 1];
}

namespace {

/** OutputDeck concatenates Path + "log.txt" without inserting a separator; Path must end with one. */
std::string directoryPathWithTrailingSep(const std::filesystem::path &dir) {
  namespace fs = std::filesystem;
  fs::path n = fs::absolute(dir).lexically_normal();
  std::string s = n.string();
  if (!s.empty() && s.back() != '/' && s.back() != '\\')
    s += fs::path::preferred_separator;
  return s;
}

json buildInputJson(const std::string &output_path_for_deck,
                    const std::filesystem::path &mesh_file_1,
                    const std::filesystem::path &mesh_file_2,
                    double final_time, size_t num_steps, bool zero_ic,
                    double mesh_size_in, double horizon_in, bool damping_on,
                    double eps_n, bool two_particle_test, bool file_mesh,
                    double beta_n_factor) {

  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double R1 = 0.001;
  const double R2 = 0.001;
  const double mesh_size =
      mesh_size_in > 0. ? mesh_size_in : std::min(R1, R2) / 5.0;
  const double horizon = horizon_in > 0. ? horizon_in : 3.0 * mesh_size;
  const double particle_dist = 0.001;

  const double poisson1 = 0.25;
  const double rho1 = 1200.0;
  const double K1 = 2.16e+7;
  const double E1 = material::toE(K1, poisson1);
  const double G1 = material::toGE(E1, poisson1);
  const double Gc1 = 50.0;

  const double poisson2 = 0.25;
  const double rho2 = 1200.0;
  const double K2 = 2.16e+7;
  const double E2 = material::toE(K2, poisson2);
  const double G2 = material::toGE(E2, poisson2);
  const double Gc2 = 50.0;

  const double R_contact_factor = 0.95;
  const double Kn_11 = 18.0 * util::harmonicMean(K1, K1) / (M_PI * std::pow(horizon, 5));
  const double Kn_22 = 18.0 * util::harmonicMean(K2, K2) / (M_PI * std::pow(horizon, 5));
  const double Kn_12 = 18.0 * util::harmonicMean(K1, K2) / (M_PI * std::pow(horizon, 5));
  const double friction_coeff = 0.5;

  std::vector<double> p1_center = center;
  std::vector<double> p2_center = center;

  const double H0_drop = particle_dist; // paper drop height, 1 mm
  // Table 2 energy: remaining gap = horizon, rest of H0 already in IC velocity.
  const double top_gap = two_particle_test ? horizon : particle_dist;

  const size_t dt_out_n = num_steps / 10;
  auto modelDeckJson = inp::ModelDeck::getExampleJson(2, final_time, num_steps,
                                                        "finite_difference", "central_difference",
                                                        true, 2, "Multi_Particle", 0);

  auto outputDeckJson = inp::OutputDeck::getExampleJson("vtu", output_path_for_deck,
      std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage_Z", "Damage", "Particle_ID"}),
      dt_out_n, 2, true, "zlib", true, 1, "", true);

  auto bcDeckJson = inp::BCDeck::getExampleJson(0, 1, 1, true, util::Point(0, -10, 0));

  bcDeckJson["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("Displacement_BC", false, geom::GeomData(),
      {0}, {}, "", {}, "", {},
      {1, 2}, true, "", {});

  std::vector<double> ic_vel = {0.0, 0.0, 0.0};
  if (!zero_ic) {
    const double fallen =
        two_particle_test ? (H0_drop - top_gap) : (particle_dist - horizon);
    if (fallen > 0.)
      ic_vel[1] = -std::sqrt(2.0 * std::abs(-10.0) * fallen);
  }
  bcDeckJson["IC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("IC", false, geom::GeomData(),
      {1}, {}, "", {}, "", {},
      {}, false, "Constant_Velocity", ic_vel);

  auto pDeckJson = json({});

  std::vector<geom::GeomData> pGeomVec(2);
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R1, p1_center[0], p1_center[1], p1_center[2]};
  pGeomVec[1].d_geomName = "circle";
  pGeomVec[1].d_geomParams = {R2, p2_center[0], p2_center[1], p2_center[2]};

  pDeckJson["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);

  const std::string f1 = mesh_file_1.string();
  const std::string f2 = mesh_file_2.string();
  json meshSet1, meshSet2;
  if (file_mesh) {
    meshSet1 = json({{"File", f1}});
    meshSet2 = json({{"File", f2}});
  } else {
    meshSet1 = json({{"File", f1},
                     {"CreateMesh",
                      {{"Flag", true},
                       {"Info", "gmsh_builtin_mesh"},
                       {"Mesh_Size", mesh_size},
                       {"Write_Mesh_File", true}}}});
    meshSet2 = json({{"File", f2},
                     {"CreateMesh",
                      {{"Flag", true},
                       {"Info", "gmsh_builtin_mesh"},
                       {"Mesh_Size", mesh_size},
                       {"Write_Mesh_File", true}}}});
  }
  pDeckJson["Mesh"] = json({{"Sets", 2}, {"Set_1", meshSet1}, {"Set_2", meshSet2}});

  auto pMatJson = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  pMatJson["Set_1"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon,
      0, rho1, K1, G1, Gc1, true, 1);
  pMatJson["Set_2"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon,
      0, rho2, K2, G2, Gc2, true, 1);
  pDeckJson["Material"] = pMatJson;

  auto pContactJson = inp::ParticleDeck::getParticleContactExampleJson(2);
  // v0.1.0 circ_damp: Friction_On: false leaves μ = 0 (coeff is not read).
  // Nonzero μ with the flag off still applies a tangential force in PairForce
  // and walks the falling particle sideways (Table 2 test 4).
  const double mu = two_particle_test ? 0. : friction_coeff;
  json contact_base = inp::ContactPairDeck::getExampleJson(
      R_contact_factor, true, damping_on, false, Kn_11, eps_n, mu,
      1.0, beta_n_factor, 1.0, 0.0, K1);

  pContactJson["Set_1_1"] = contact_base;
  pContactJson["Set_1_1"]["Kn"] = Kn_11;
  pContactJson["Set_1_1"]["K"] = K1;
  pContactJson["Set_1_2"] = contact_base;
  pContactJson["Set_1_2"]["Kn"] = Kn_12;
  pContactJson["Set_1_2"]["K"] = util::harmonicMean(K1, K2);
  pContactJson["Set_2_2"] = contact_base;
  pContactJson["Set_2_2"]["Kn"] = Kn_22;
  pContactJson["Set_2_2"]["K"] = K2;
  pDeckJson["Contact"] = pContactJson;

  pDeckJson["Neighbor"] = two_particle_test
      ? inp::PNeighborDeck::getExampleJson("simple_all", 5.0, 1, 0.5)
      : inp::PNeighborDeck::getExampleJson("simple_all", 10.0, 40, 0.5);

  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");
  pGenJson["Random_Rotation"] = false;
  pGenJson["Data"]["N"] = 2;
  pGenJson["Data"]["0"] = {
      {"x", R1}, {"y", R1}, {"z", 0.0},
      {"theta", 0.0}, {"s", 1.0},
      {"geom_id", 0}, {"mat_id", 0}, {"contact_id", 0}
  };
  pGenJson["Data"]["1"] = {
      {"x", R1}, {"y", 2.0 * R1 + R2 + top_gap}, {"z", 0.0},
      {"theta", two_particle_test ? M_PI / 2.0 : M_PI}, {"s", 1.0},
      {"geom_id", 1}, {"mat_id", 1}, {"contact_id", 1}
  };
  pDeckJson["Particle_Generation"] = pGenJson;

  auto j = json({{"Model", modelDeckJson},
               {"Output", outputDeckJson},
               {"Force_BC", bcDeckJson["Force_BC"]},
               {"Displacement_BC", bcDeckJson["Displacement_BC"]},
               {"IC", bcDeckJson["IC"]},
               {"Particle", pDeckJson["Particle"]},
               {"Mesh", pDeckJson["Mesh"]},
               {"Material", pDeckJson["Material"]},
               {"Contact", pDeckJson["Contact"]},
               {"Neighbor", pDeckJson["Neighbor"]},
               {"Particle_Generation", pDeckJson["Particle_Generation"]}});
  if (two_particle_test)
    j["Test"] = json{{"Test_Name", "two_particle"}};
  return j;
}

} // namespace

class RestitutionProbe : public postprocess::Postprocess {
public:
  explicit RestitutionProbe(double H0) : d_H0(H0) {}

  void checkStop(data::ModelData &data) override {
    postprocess::Postprocess::checkStop(data);
    if (data.d_particlesListTypeAll.size() < 2)
      return;
    const auto *p0 = data.d_particlesListTypeAll[0];
    const auto *p1 = data.d_particlesListTypeAll[1];
    const double gap = p0->getXCenter().dist(p1->getXCenter()) -
                       p0->d_geom_p->boundingRadius() -
                       p1->d_geom_p->boundingRadius();
    if (data.currentStep() % 50 == 0)
      d_samples.push_back({data.d_time, gap});
    if (d_done)
      return;
    const double leave = 1.0e-5;
    const double significant = 0.05 * d_H0;
    if (!d_contacted) {
      if (gap < d_minGap) {
        d_minGap = gap;
        d_tMin = data.d_time;
      } else if (d_minGap < 0.3 * d_H0 && gap > d_minGap + leave) {
        d_contacted = true;
        d_H1 = gap;
        d_tH1 = data.d_time;
      }
    } else if (gap > d_H1) {
      d_H1 = gap;
      d_tH1 = data.d_time;
    } else if (d_H1 > d_minGap + significant && gap < d_H1 - leave) {
      d_done = true;
    }
  }

  bool contacted() const { return d_contacted; }
  double H1() const { return d_H1; }
  double minGap() const { return d_minGap; }
  double tMin() const { return d_tMin; }
  double tH1() const { return d_tH1; }
  const std::vector<std::pair<double, double>> &samples() const {
    return d_samples;
  }
  double CR() const {
    if (d_H0 <= 0. || d_H1 <= 0.)
      return 0.;
    return std::sqrt(d_H1 / d_H0);
  }

private:
  double d_H0;
  double d_H1 = 0.;
  double d_minGap = 1.0e9;
  double d_tMin = 0.;
  double d_tH1 = 0.;
  bool d_contacted = false;
  bool d_done = false;
  std::vector<std::pair<double, double>> d_samples;
};

int main(int argc, char *argv[]) {

  util::parallel::initMpi(argc, argv);
  int mpiSize = util::parallel::mpiSize(), mpiRank = util::parallel::mpiRank();
  util::io::print(std::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::parallel::getMpiStatus()->printStr());

  util::io::InputParser input(argc, argv);

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads"))
    nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = std::thread::hardware_concurrency();
    util::io::print(std::format("Running test with default number of threads = {}\n", nThreads));
  }
  util::parallel::initNThreads(nThreads);
  util::io::print(std::format("Number of threads = {}\n", util::parallel::getNThreads()));

#ifndef TWOP_CONTACT_EXAMPLE
  double final_time = 0.002;
  size_t num_steps = 6000;
#else
  double final_time = 0.012;
  size_t num_steps = 36000;
#endif
  bool zero_ic = false;
  double mesh_size_in = -1.;
  double horizon_in = -1.;
  bool assert_cr = false;
  double cr_ref = 1.;
  double cr_tol = 0.1;
  bool damping_on = false;
  double eps_n = 0.9;
  double beta_n_factor = 100.0;
  int jha_test = 0;
  if (input.cmdOptionExists("-jha2021Table2Test"))
    jha_test = std::stoi(input.getCmdOption("-jha2021Table2Test"));
  else if (input.cmdOptionExists("-jha2021Table2"))
    jha_test = 1;
  if (jha_test > 0) {
    const auto c = jha2021Table2(jha_test);
    // Paper §4.1 / v0.1.0 circ_damp: T=0.04 s, Δt=0.2 μs, horizon 0.6 mm.
    // Mesh is the frozen v0.1.0 .msh (hmin = 0.1423 mm), not in-process Gmsh.
    final_time = 0.04;
    num_steps = 200000;
    horizon_in = 0.0006;
    // v0.1.0 circ_damp: not drop-from-rest. Gap is shortened to the horizon
    // and the leftover 0.4 mm of the 1 mm drop is an impact velocity.
    zero_ic = false;
    assert_cr = true;
    cr_ref = c.cr;
    cr_tol = 0.05;
    eps_n = c.eps;
    damping_on = c.eps < 1.0 - 1.0e-12;
  }
  if (input.cmdOptionExists("-finalTime"))
    final_time = std::stod(input.getCmdOption("-finalTime"));
  if (input.cmdOptionExists("-numSteps"))
    num_steps = std::stoul(input.getCmdOption("-numSteps"));
  if (input.cmdOptionExists("-zeroIC"))
    zero_ic = true;
  if (input.cmdOptionExists("-meshSize"))
    mesh_size_in = std::stod(input.getCmdOption("-meshSize"));
  if (input.cmdOptionExists("-horizon"))
    horizon_in = std::stod(input.getCmdOption("-horizon"));
  if (input.cmdOptionExists("-epsN")) {
    eps_n = std::stod(input.getCmdOption("-epsN"));
    damping_on = eps_n < 1.0 - 1.0e-12;
  }
  if (input.cmdOptionExists("-betaNFactor"))
    beta_n_factor = std::stod(input.getCmdOption("-betaNFactor"));
  if (input.cmdOptionExists("-assertCR")) {
    assert_cr = true;
    cr_ref = std::stod(input.getCmdOption("-assertCR"));
  }
  if (input.cmdOptionExists("-crTol"))
    cr_tol = std::stod(input.getCmdOption("-crTol"));
  util::io::print(std::format(
      "final_time = {}, num_steps = {}, zero_ic = {}, eps_n = {}, C_bar = {}, damping = {}\n",
      final_time, num_steps, zero_ic, eps_n, beta_n_factor, damping_on));

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
  } else if (input.cmdOptionExists("-outputDir")) {
    // Place inp next to out: .../run/out -> .../run/inp
    inp_dir = out_dir.parent_path() / "inp";
  }

  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);

  const std::string output_path_for_deck = directoryPathWithTrailingSep(out_dir);
  const fs::path mesh1 = inp_dir / "mesh_cir_1.msh";
  const fs::path mesh2 = inp_dir / "mesh_cir_2.msh";
  const bool inbuilt_mesh = input.cmdOptionExists("-inbuiltMesh");
  const bool file_mesh = jha_test > 0 && !inbuilt_mesh;
  if (file_mesh) {
#ifndef JHA2021_MESH_DIR
    throw std::runtime_error("JHA2021_MESH_DIR is not set (v0.1.0 circ_damp meshes).");
#else
    const fs::path src(JHA2021_MESH_DIR);
    fs::copy_file(src / "mesh_cir_1.msh", mesh1, fs::copy_options::overwrite_existing);
    fs::copy_file(src / "mesh_cir_2.msh", mesh2, fs::copy_options::overwrite_existing);
    util::io::print(std::format("Using v0.1.0 circ_damp meshes from {}\n", src.string()));
#endif
  } else if (jha_test > 0)
    util::io::print("Using in-process Gmsh (gmsh_builtin_mesh) instead of frozen v0.1.0 .msh\n");

  util::io::print(std::format("Output directory (VTU, log.txt): {}\n", fs::absolute(out_dir).string()));
  util::io::print(std::format("Input directory (input.json, meshes): {}\n", fs::absolute(inp_dir).string()));

  auto inputJson = buildInputJson(output_path_for_deck, mesh1, mesh2, final_time,
                                  num_steps, zero_ic, mesh_size_in, horizon_in,
                                  damping_on, eps_n, jha_test > 0, file_mesh,
                                  beta_n_factor);

  const fs::path input_json_path = inp_dir / "input.json";
  {
    std::ofstream os(input_json_path);
    if (!os)
      throw std::runtime_error("Failed to open " + input_json_path.string() + " for writing.");
    os << inputJson.dump(2);
  }
  util::io::print(std::format("Wrote deck to {}\n", fs::absolute(input_json_path).string()));
  if (jha_test > 0) {
    const auto &p1 = inputJson["Particle_Generation"]["Data"]["1"];
    const auto &v = inputJson["IC"]["Set_1"]["Constant_Velocity"]["Velocity_Vector"];
    const double y1 = p1.at("y").get<double>();
    const double gap0 = y1 - 0.001 - 0.001 - 0.001; // y_top - R_bottom_center - R_bot - R_top
    util::io::print(std::format(
        "Table 2 kinematics: top y = {}, surface gap = {}, v_y = {}\n",
        y1, gap0, v.at(1).get<double>()));
  }

  auto deck = std::make_shared<inp::Input>(inputJson);

  PeriDEMModel dem(deck);
  RestitutionProbe *probe = nullptr;
  if (assert_cr) {
    auto p = std::make_unique<RestitutionProbe>(0.001);
    probe = p.get();
    dem.setPostprocess(std::move(p));
  }
  dem.run(deck);

  if (input.cmdOptionExists("-requireContact")) {
    const float zmax =
        dem.d_Z.empty() ? 0.f : *std::max_element(dem.d_Z.begin(), dem.d_Z.end());
    util::io::print(std::format("requireContact: max Damage_Z = {}\n", zmax));
    if (zmax <= 0.f) {
      util::io::print("requireContact: no damage; particles did not contact.\n");
      return EXIT_FAILURE;
    }
  }

  if (assert_cr) {
    {
      const fs::path gap_csv = out_dir / "gap.csv";
      std::ofstream gs(gap_csv);
      gs << "t,gap\n";
      for (const auto &s : probe->samples())
        gs << s.first << "," << s.second << "\n";
      util::io::print(std::format("Wrote {}\n", fs::absolute(gap_csv).string()));
    }
    const double cr = probe->CR();
    util::io::print(std::format(
        "assertCR: contacted={}, min_gap={} (t={}), H1={} (t={}), CR={}, ref={}, tol={}\n",
        probe->contacted(), probe->minGap(), probe->tMin(), probe->H1(),
        probe->tH1(), cr, cr_ref, cr_tol));
    if (cr > 1.02) {
      util::io::print(
          "assertCR: CR > 1.02 on the 1 mm energy-equivalent drop "
          "(surface gap = horizon, leftover height already in v_y). "
          "Unphysical energy gain.\n");
      return EXIT_FAILURE;
    }
    if (!probe->contacted() || std::abs(cr - cr_ref) > cr_tol) {
      util::io::print("assertCR: coefficient of restitution out of range.\n");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
