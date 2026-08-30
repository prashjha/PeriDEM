/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include <PeriDEMConfig.h>

#include "appDamping.h"
#include "appPostprocess.h"
#include "contact/contact.h"
#include "geom/geomObjects.h"
#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "mesh_gen/meshGenerator.h"
#include "periDEMModel.h"
#include "util/function.h"
#include "util/io.h"
#include "util/parallelUtil.h"

#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

json builtinTwoParticleJson();

int main(int argc, char *argv[]) {

  util::parallel::initMpi(argc, argv);
  int mpiSize = util::parallel::mpiSize(), mpiRank = util::parallel::mpiRank();
  util::io::print(std::format("Initialized MPI. MPI size = {}, MPI rank = {}\n",
                              mpiSize, mpiRank));

  util::io::InputParser input(argc, argv);

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads"))
    nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = std::thread::hardware_concurrency();
    util::io::print(std::format(
        "Running with default number of threads = {}\n", nThreads));
  }
  util::parallel::initNThreads(nThreads);

  std::cout << "TwoParticle_Custom (PeriDEM " << MAJOR_VERSION << "."
            << MINOR_VERSION << "." << UPDATE_VERSION << ")\n";

  std::shared_ptr<inp::Input> deck;
  if (input.cmdOptionExists("-i")) {
    std::string filename = input.getCmdOption("-i");
    if (!std::filesystem::exists(filename))
      throw std::runtime_error(
          std::format("Input file {} does not exist.", filename));
    std::ifstream f(filename);
    deck = std::make_shared<inp::Input>(json::parse(f));
  } else {
    deck = std::make_shared<inp::Input>(builtinTwoParticleJson());
  }

  PeriDEMModel dem(deck);
  auto contact = std::make_unique<contact::Contact>();
  contact->setDamping(std::make_unique<twoparticle_custom::AppDamping>());
  dem.setContact(std::move(contact));
  dem.setPostprocess(std::make_unique<twoparticle_custom::AppPostprocess>());
  dem.run(deck);

  return EXIT_SUCCESS;
}

json builtinTwoParticleJson() {

  std::string output_dir = "./out/";
  std::string input_dir = "./inp/";
  std::filesystem::create_directories(output_dir);
  std::filesystem::create_directories(input_dir);

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
  const double Kn_11 =
      18.0 * util::harmonicMean(K1, K1) / (M_PI * std::pow(horizon, 5));
  const double Kn_22 =
      18.0 * util::harmonicMean(K2, K2) / (M_PI * std::pow(horizon, 5));
  const double Kn_12 =
      18.0 * util::harmonicMean(K1, K2) / (M_PI * std::pow(horizon, 5));
  const double beta_n_eps = 0.9;
  const double friction_coeff = 0.5;
  const double beta_n_factor = 100.0;

  std::string mesh1_file_name = input_dir + "mesh_cir_1";
  auto mesh_geom_1 = std::make_shared<geom::Circle>(
      R1, util::Point(center[0], center[1], center[2]));
  mesh_gen::generateBuiltinParticleMeshGmsh(mesh_geom_1, mesh_size,
                                            mesh1_file_name, false, true,
                                            nullptr, nullptr, nullptr);

  std::string mesh2_file_name = input_dir + "mesh_cir_2";
  auto mesh_geom_2 = std::make_shared<geom::Circle>(
      R2, util::Point(center[0], center[1], center[2]));
  mesh_gen::generateBuiltinParticleMeshGmsh(mesh_geom_2, mesh_size,
                                            mesh2_file_name, false, true,
                                            nullptr, nullptr, nullptr);

  const double final_time = 0.00001;
  const size_t num_steps = 50;
  const size_t dt_out_n = std::max(size_t(1), num_steps / 5);
  auto modelDeckJson = inp::ModelDeck::getExampleJson(
      2, final_time, num_steps, "finite_difference", "central_difference", true,
      2, "Multi_Particle", 0);

  auto outputDeckJson = inp::OutputDeck::getExampleJson(
      "vtu", output_dir,
      std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage_Z",
                                "Damage", "Particle_ID"}),
      dt_out_n, 2, true, "zlib", true, 1, "");

  auto bcDeckJson =
      inp::BCDeck::getExampleJson(0, 1, 1, true, util::Point(0, -10, 0));

  bcDeckJson["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson(
      "Displacement_BC", false, geom::GeomData(), {0}, {}, "", {}, "", {},
      {1, 2}, true, "", {});

  const double free_fall_dist = particle_dist - horizon;
  const double free_fall_vel =
      -std::sqrt(2.0 * std::abs(-10.0) * free_fall_dist);
  bcDeckJson["IC"]["Set_1"] = inp::BCBaseDeck::getExampleJson(
      "IC", false, geom::GeomData(), {1}, {}, "", {}, "", {}, {}, false,
      "Constant_Velocity", {0.0, free_fall_vel, 0.0});

  auto pDeckJson = json({});
  std::vector<geom::GeomData> pGeomVec(2);
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R1, center[0], center[1], center[2]};
  pGeomVec[1].d_geomName = "circle";
  pGeomVec[1].d_geomParams = {R2, center[0], center[1], center[2]};
  pDeckJson["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);
  pDeckJson["Mesh"] = inp::ParticleDeck::getParticleMeshExampleJson(
      {mesh1_file_name + ".msh", mesh2_file_name + ".msh"});

  auto pMatJson = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  pMatJson["Set_1"] = inp::MaterialDeck::getExampleJson(
      "PDState", false, horizon, 0, rho1, K1, G1, Gc1, true, 1);
  pMatJson["Set_2"] = inp::MaterialDeck::getExampleJson(
      "PDState", false, horizon, 0, rho2, K2, G2, Gc2, true, 1);
  pDeckJson["Material"] = pMatJson;

  auto pContactJson = inp::ParticleDeck::getParticleContactExampleJson(2);
  json contact_base = inp::ContactPairDeck::getExampleJson(
      R_contact_factor, true, false, false, Kn_11, beta_n_eps, friction_coeff,
      1.0, beta_n_factor, 1.0, 0.0, 0.0);
  pContactJson["Set_1_1"] = contact_base;
  pContactJson["Set_1_1"]["Kn"] = Kn_11;
  pContactJson["Set_1_2"] = contact_base;
  pContactJson["Set_1_2"]["Kn"] = Kn_12;
  pContactJson["Set_2_2"] = contact_base;
  pContactJson["Set_2_2"]["Kn"] = Kn_22;
  pDeckJson["Contact"] = pContactJson;
  pDeckJson["Neighbor"] =
      inp::PNeighborDeck::getExampleJson("simple_all", 10.0, 40, 0.5);

  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");
  pGenJson["Data"]["N"] = 2;
  pGenJson["Data"]["0"] = {{"x", R1},
                           {"y", R1},
                           {"z", 0.0},
                           {"theta", 0.0},
                           {"s", 1.0},
                           {"geom_id", 0},
                           {"mat_id", 0},
                           {"contact_id", 0}};
  pGenJson["Data"]["1"] = {{"x", R1},
                           {"y", 2.0 * R1 + R2 + particle_dist},
                           {"z", 0.0},
                           {"theta", M_PI * 0.5},
                           {"s", 1.0},
                           {"geom_id", 1},
                           {"mat_id", 1},
                           {"contact_id", 1}};
  pDeckJson["Particle_Generation"] = pGenJson;

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
                         {"Particle_Generation", pDeckJson["Particle_Generation"]},
                         {"Test", json{{"Test_Name", "two_particle"}}}});

  std::ofstream f(input_dir + "/input.json");
  f << inputJson.dump(2);
  f.close();

  return inputJson;
}
