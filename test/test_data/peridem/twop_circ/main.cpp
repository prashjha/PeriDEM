/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "inp/deckIncludes.h"
#include "mesh_gen/meshGenerator.h"
#include "util/io.h"
#include "util/function.h"
#include "material/materialUtil.h"
#include "model/dem/demModel.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <cmath>

json getInputJson();

int main(int argc, char *argv[]) {

    // init parallel
  util::parallel::initMpi(argc, argv);
  int mpiSize = util::parallel::mpiSize(), mpiRank = util::parallel::mpiRank();
  util::io::print(std::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::parallel::getMpiStatus()->printStr());

  util::io::InputParser input(argc, argv);
  
  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = std::thread::hardware_concurrency();
    util::io::print(std::format("Running test with default number of threads = {}\n", nThreads));
  }
  // set number of threads
  util::parallel::initNThreads(nThreads);
  util::io::print(std::format("Number of threads = {}\n", util::parallel::getNThreads()));
  
  auto inputJson = getInputJson();

  // Create input deck
  auto deck = std::make_shared<inp::Input>(inputJson);

  // PeriDEM
  model::DEMModel dem(deck);
  dem.run(deck);

  return EXIT_SUCCESS;
}

json getInputJson() {

  // Create output directory
  std::string output_dir = "./out/";
  std::string input_dir = "./inp/";
  std::filesystem::create_directories(output_dir);
  std::filesystem::create_directories(input_dir);

  // Simulation parameters
  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double R1 = 0.001;  // Bottom particle radius
  const double R2 = 0.001;  // Top particle radius
  const double mesh_size = std::min(R1, R2) / 5.0;
  const double horizon = 3.0 * mesh_size;
  const double particle_dist = 0.001;

  // Material parameters
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

  // Contact parameters
  const double R_contact_factor = 0.95;
  const double Kn_11 = 18.0 * util::harmonicMean(K1, K1) / (M_PI * std::pow(horizon, 5));
  const double Kn_22 = 18.0 * util::harmonicMean(K2, K2) / (M_PI * std::pow(horizon, 5));
  const double Kn_12 = 18.0 * util::harmonicMean(K1, K2) / (M_PI * std::pow(horizon, 5));
  const double beta_n_eps = 0.9;
  const double friction_coeff = 0.5;
  const double beta_n_factor = 100.0;

  // Generate meshes for both particles
  
  // Bottom particle (Zone 1)
  std::vector<double> p1_center = center;
  std::string mesh1_file_name = input_dir + "mesh_cir_1";
  mesh_gen::CircularParticleMeshGenerator gen1(p1_center, R1, mesh_size, 1, 1);
  gen1.generate(mesh1_file_name);
  gen1.finalize();

  // Top particle (Zone 2)
  std::vector<double> p2_center = center;
  std::string mesh2_file_name = input_dir + "mesh_cir_2";
  mesh_gen::CircularParticleMeshGenerator gen2(p2_center, R2, mesh_size, 2, 1);
  gen2.generate(mesh2_file_name);
  gen2.finalize();

  // Model deck
  size_t num_steps = 20000;
  size_t dt_out_n = num_steps / 10;
  auto modelDeckJson = inp::ModelDeck::getExampleJson(2, 0.006, num_steps, 
      "finite_difference", "central_difference",
      true, 2, "Multi_Particle", 0);

  // Output deck
  auto outputDeckJson = inp::OutputDeck::getExampleJson("vtu", output_dir,
      std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage_Z", "Damage", "Particle_ID"}),
      dt_out_n, 2, true, "zlib", true, 1, "");

  // BC deck
  auto bcDeckJson = inp::BCDeck::getExampleJson(0, 1, 1, true, util::Point(0, -10, 0));

  // Displacement BC for fixing bottom particle
  bcDeckJson["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("Displacement_BC", false, geom::GeomData(),
      {0}, {}, "", {}, "", {},
      {1, 2}, true, "", {});

  // Initial velocity for top particle
  const double free_fall_dist = particle_dist - horizon;
  const double free_fall_vel = -std::sqrt(2.0 * std::abs(-10.0) * free_fall_dist);
  bcDeckJson["IC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("IC", false, geom::GeomData(),
      {1}, {}, "", {}, "", {},
      {}, false, "Constant_Velocity", {0.0, free_fall_vel, 0.0});

  // Particle deck
  auto pDeckJson = json({});

  // Particle geometry
  std::vector<geom::GeomData> pGeomVec(2);
  
  // Bottom particle
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R1, p1_center[0], p1_center[1], p1_center[2]};

  // Top particle
  pGeomVec[1].d_geomName = "circle";
  pGeomVec[1].d_geomParams = {R2, p2_center[0], p2_center[1], p2_center[2]};

  pDeckJson["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);

  // Mesh settings
  pDeckJson["Mesh"] = inp::ParticleDeck::getParticleMeshExampleJson({
      mesh1_file_name + ".msh",
      mesh2_file_name + ".msh"
  });

  // Material settings
  auto pMatJson = inp::ParticleDeck::getParticleMaterialExampleJson(2);

  // Material 1 (bottom particle)
  pMatJson["Set_1"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon,
      0, rho1, K1, G1, Gc1, true, 1);

  // Material 2 (top particle)
  pMatJson["Set_2"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon,
      0, rho2, K2, G2, Gc2, true, 1);

  pDeckJson["Material"] = pMatJson;

  // Contact settings
  auto pContactJson = inp::ParticleDeck::getParticleContactExampleJson(2);

  // Base contact parameters
  json contact_base = inp::ContactPairDeck::getExampleJson(R_contact_factor,
      true, false, false,
      Kn_11, beta_n_eps, friction_coeff, 1.0, beta_n_factor, 1.0, 0.0, 0.0);

  // Contact pair 1-1 (bottom-bottom)
  pContactJson["Set_1_1"] = contact_base;
  pContactJson["Set_1_1"]["Kn"] = Kn_11;

  // Contact pair 1-2 (bottom-top)
  pContactJson["Set_1_2"] = contact_base;
  pContactJson["Set_1_2"]["Kn"] = Kn_12;

  // Contact pair 2-2 (top-top)
  pContactJson["Set_2_2"] = contact_base;
  pContactJson["Set_2_2"]["Kn"] = Kn_22;

  pDeckJson["Contact"] = pContactJson;

  // Neighbor settings
  pDeckJson["Neighbor"] = inp::PNeighborDeck::getExampleJson("simple_all", 10.0, 40, 0.5);

  // Particle generation settings
  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");

  // Add data that will be used to create particles
  pGenJson["Data"]["N"] = 2;  // two particles

  // Bottom particle
  pGenJson["Data"]["0"] = {
      {"x", R1}, {"y", R1}, {"z", 0.0},
      {"theta", 0.0}, {"s", 1.0},
      {"geom_id", 0}, {"mat_id", 0}, {"contact_id", 0}
  };

  // Top particle
  pGenJson["Data"]["1"] = {
      {"x", R1}, {"y", 2.0 * R1 + R2 + particle_dist}, {"z", 0.0},
      {"theta", M_PI * 0.5}, {"s", 1.0},
      {"geom_id", 1}, {"mat_id", 1}, {"contact_id", 1}
  };

  // Add to json
  pDeckJson["Particle_Generation"] = pGenJson;

  // Collect all decks into global input JSON
  auto inputJson = json({{"Model", modelDeckJson}, 
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

  // Write configuration to file
  std::cout << "\n\nPrinting global input deck json:\n";
  std::cout << inputJson.dump(2) << std::endl;
  
  // save to file
  std::ofstream f(input_dir + "/input.json");
  f << inputJson.dump(2);
  f.close();

  return inputJson;
} 