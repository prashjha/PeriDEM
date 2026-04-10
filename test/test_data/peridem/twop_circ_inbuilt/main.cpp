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
 */

#include "inp/deckIncludes.h"
#include "util/io.h"
#include "util/function.h"
#include "material/materialUtil.h"
#include "model/dem/demModel.h"
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <thread>

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
                    const std::filesystem::path &mesh_file_2) {

  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double R1 = 0.001;
  const double R2 = 0.001;
  const double mesh_size = std::min(R1, R2) / 5.0;
  const double horizon = 3.0 * mesh_size;
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
  const double beta_n_eps = 0.9;
  const double friction_coeff = 0.5;
  const double beta_n_factor = 100.0;

  std::vector<double> p1_center = center;
  std::vector<double> p2_center = center;

  const double final_time = 0.002;
  const size_t num_steps = 6000;
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

  const double free_fall_dist = particle_dist - horizon;
  const double free_fall_vel = -std::sqrt(2.0 * std::abs(-10.0) * free_fall_dist);
  bcDeckJson["IC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("IC", false, geom::GeomData(),
      {1}, {}, "", {}, "", {},
      {}, false, "Constant_Velocity", {0.0, free_fall_vel, 0.0});

  auto pDeckJson = json({});

  std::vector<geom::GeomData> pGeomVec(2);
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R1, p1_center[0], p1_center[1], p1_center[2]};
  pGeomVec[1].d_geomName = "circle";
  pGeomVec[1].d_geomParams = {R2, p2_center[0], p2_center[1], p2_center[2]};

  pDeckJson["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);

  const std::string f1 = mesh_file_1.string();
  const std::string f2 = mesh_file_2.string();
  auto meshSet1 = json({{"File", f1},
                        {"CreateMesh",
                         {{"Flag", true},
                          {"Info", "gmsh_builtin_mesh"},
                          {"Mesh_Size", mesh_size},
                          {"Write_Mesh_File", true}}}});
  auto meshSet2 = json({{"File", f2},
                        {"CreateMesh",
                         {{"Flag", true},
                          {"Info", "gmsh_builtin_mesh"},
                          {"Mesh_Size", mesh_size},
                          {"Write_Mesh_File", true}}}});
  pDeckJson["Mesh"] = json({{"Sets", 2}, {"Set_1", meshSet1}, {"Set_2", meshSet2}});

  auto pMatJson = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  pMatJson["Set_1"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon,
      0, rho1, K1, G1, Gc1, true, 1);
  pMatJson["Set_2"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon,
      0, rho2, K2, G2, Gc2, true, 1);
  pDeckJson["Material"] = pMatJson;

  auto pContactJson = inp::ParticleDeck::getParticleContactExampleJson(2);
  json contact_base = inp::ContactPairDeck::getExampleJson(R_contact_factor,
      true, false, false,
      Kn_11, beta_n_eps, friction_coeff, 1.0, beta_n_factor, 1.0, 0.0, 0.0);

  pContactJson["Set_1_1"] = contact_base;
  pContactJson["Set_1_1"]["Kn"] = Kn_11;
  pContactJson["Set_1_2"] = contact_base;
  pContactJson["Set_1_2"]["Kn"] = Kn_12;
  pContactJson["Set_2_2"] = contact_base;
  pContactJson["Set_2_2"]["Kn"] = Kn_22;
  pDeckJson["Contact"] = pContactJson;

  pDeckJson["Neighbor"] = inp::PNeighborDeck::getExampleJson("simple_all", 10.0, 40, 0.5);

  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");
  pGenJson["Data"]["N"] = 2;
  pGenJson["Data"]["0"] = {
      {"x", R1}, {"y", R1}, {"z", 0.0},
      {"theta", 0.0}, {"s", 1.0},
      {"geom_id", 0}, {"mat_id", 0}, {"contact_id", 0}
  };
  pGenJson["Data"]["1"] = {
      {"x", R1}, {"y", 2.0 * R1 + R2 + particle_dist}, {"z", 0.0},
      {"theta", M_PI}, {"s", 1.0},
      {"geom_id", 1}, {"mat_id", 1}, {"contact_id", 1}
  };
  pDeckJson["Particle_Generation"] = pGenJson;

  return json({{"Model", modelDeckJson},
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
}

} // namespace

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

  util::io::print(std::format("Output directory (VTU, log.txt): {}\n", fs::absolute(out_dir).string()));
  util::io::print(std::format("Input directory (input.json, meshes): {}\n", fs::absolute(inp_dir).string()));

  auto inputJson = buildInputJson(output_path_for_deck, mesh1, mesh2);

  const fs::path input_json_path = inp_dir / "input.json";
  {
    std::ofstream os(input_json_path);
    if (!os)
      throw std::runtime_error("Failed to open " + input_json_path.string() + " for writing.");
    os << inputJson.dump(2);
  }
  util::io::print(std::format("Wrote deck to {}\n", fs::absolute(input_json_path).string()));

  auto deck = std::make_shared<inp::Input>(inputJson);

  model::DEMModel dem(deck);
  dem.run(deck);

  return EXIT_SUCCESS;
}
