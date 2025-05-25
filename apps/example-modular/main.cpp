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

#include <format>
#include <fstream>
#include <iostream>
#include <random>

std::string example_name = "Example_Modular";

int main(int argc, char *argv[]) {
  util::io::InputParser input(argc, argv);

  // read input arguments
  unsigned int nThreads = 1;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  util::parallel::initNThreads(nThreads);

  // +------------------+
  // + create input decks +
  // +------------------+
  auto *deck = new inp::Input();

  // setup
  // three particles
  // h = mesh size
  // annulus rectangle -> fixed dof, geometry group 1, material group 1, mesh group 1, contact group 1
  //    outer rec: (-h, -h, 0); (0.01+h, 0.01+h, 0)
  //    inner rec: (0, 0, 0); (0.01, 0.01, 0)
  //
  // drum2d -> velocity ic, geometry group 2, material group 2, mesh group 2, contact group 2
  //    radius = 0.002
  // circle -> velocity ic, geometry group 3, material group 2, mesh group 3, contact group 2
  //    radius = 0.002
  double r = 0.002;
  double h = r/5;

  // <<<<<<<<<<<<<<
  // Model deck
  // <<<<<<<<<<<<<<
  // set values manually
  deck->d_modelDeck_p = std::make_shared<inp::ModelDeck>(2, 0.005, 50000,
      "finite_difference", "central_difference",
      true, 2, "Multi_Particle",
      0);

  // create json object using in-built function and read it (circular but good to see the json file format and how to read it)
  auto modelDeckJson = inp::ModelDeck::getExampleJson(2, 0.005, 50000,
      "finite_difference", "central_difference",
      true, 2, "Multi_Particle",
      0);

  std::cout << "\n\nPrinting model deck json:\n";
  std::cout << modelDeckJson.dump(2) << std::endl;

//  std::cout << "\n\nPrinting model deck:\n";
//  std::cout << deck->d_modelDeck_p->printStr() << std::endl;



  // <<<<<<<<<<<<<<
  // Output deck
  // <<<<<<<<<<<<<<
  // set values manually
  deck->d_outputDeck_p = std::make_shared<inp::OutputDeck>("vtu", "./",
      std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage_Z", "Damage", "Particle_ID"}),
      1, 2, true, "zlib", true, 1, "");

  // or create json object and set deck from the object
  auto outputDeckJson = inp::OutputDeck::getExampleJson("vtu", "./",
      {"Displacement", "Velocity", "Force", "Damage_Z", "Damage", "Particle_ID"},
      1, 2, true, "zlib", true, 1, "");

  std::cout << "\n\nPrinting output deck json:\n";
  std::cout << outputDeckJson.dump(2) << std::endl;

//  std::cout << "\n\nPrinting output deck:\n";
//  std::cout << deck->d_outputDeck_p->printStr() << std::endl;

  // <<<<<<<<<<<<<<
  // Restart deck
  // <<<<<<<<<<<<<<
  deck->d_restartDeck_p = std::make_shared<inp::RestartDeck>();

  // <<<<<<<<<<<<<<
  // Test deck
  // <<<<<<<<<<<<<<
  deck->d_testDeck_p = std::make_shared<inp::TestDeck>(std::string(""));

  // or
  auto testDeckJson = inp::TestDeck::getExampleJson();

  std::cout << "\n\nPrinting test deck json:\n";
  std::cout << testDeckJson.dump(2) << std::flush;

  std::cout << "\n\nPrinting test deck:\n";
  std::cout << deck->d_testDeck_p->printStr() << std::endl;

  if (false) deck->d_testDeck_p = std::make_shared<inp::TestDeck>(testDeckJson);

  // <<<<<<<<<<<<<<
  // BC deck
  // <<<<<<<<<<<<<<
  // Zero force boundary condition, One displacement boundary condition for fixing annulus rectangle
  // Two Initial conditions to specify initial velocity of the particles
  auto bcDeckJson = inp::BCDeck::getExampleJson(0, 1, 2, false, util::Point());

  // create this block from the BCData static function
  bcDeckJson["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("Displacement_BC", false, geom::GeomData(),
        {0}, {}, "", {}, "", {},
        {1, 2}, true, "", {});
  // or explicitly, we can do
  // dispBCJson.at("Particle_List") = std::vector<size_t>({0});
  // dispBCJson.at("Zero_Displacement") = true;
  // dispBCJson.at("Direction") = std::vector<size_t>({0, 1});

  // fix initial condition
  double v_mag = 0.1; // m/s
  {
    // create this block from the ICData static function
    bcDeckJson["IC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("IC", false, geom::GeomData(),
                                                                   {1}, {}, "", {}, "", {},
                                                                   {}, true, "Constant_Velocity", {v_mag*0.1, v_mag*0.9, 0.});
    // or explicitly, we can do
    // initCondJson.at("Constant_Velocity").at("Particle_List") = std::vector<size_t>({1});
    // initCondJson.at("Constant_Velocity").at("Velocity_Vector") = std::vector<double>{v_mag*0.1, v_mag*0.9, 0.};
  }
  {
    bcDeckJson["IC"]["Set_2"] = inp::BCBaseDeck::getExampleJson("IC", false, geom::GeomData(),
                                                                   {2}, {}, "", {}, "", {},
                                                                   {}, true, "Constant_Velocity", {v_mag*0.5, v_mag*0.5, 0.});
  }

  // create bc deck after fixing json object
  deck->d_bcDeck_p = std::make_shared<inp::BCDeck>(bcDeckJson);

  std::cout << "\n\nPrinting bc deck json:\n";
  std::cout << bcDeckJson.dump(2) << std::flush;

  std::cout << "\n\nPrinting bc deck:\n";
  std::cout << deck->d_bcDeck_p->printStr() << std::endl;

  // <<<<<<<<<<<<<<
  // Particle deck
  // <<<<<<<<<<<<<<
  auto pDeckJson = json({});

  //// create particle geometry json
  std::vector<geom::GeomData> pGeomVec(3); // 3 geometry groups

  pGeomVec[0].d_geomName = "rectangle_minus_rectangle";
  // parameters
  pGeomVec[0].d_geomParams = std::vector<double>({0., 0., 0., 0.01, 0.01, 0., -h, -h, 0., 0.01+h, 0.01+h, 0.});

  pGeomVec[1].d_geomName = "drum2d";
  // parameters to create drum2d: R, neck width, center, axis
  pGeomVec[1].d_geomParams = std::vector<double>({r, r*0.5, 0.004, 0.0065, 0., 1., 0., 0.});

  pGeomVec[2].d_geomName = "circle";
  // parameters to create circle: R, center
  pGeomVec[2].d_geomParams = std::vector<double>({1.25*r, 0.007, 0.004, 0.});

  // create block for particle geometry
  auto pGeomJson = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);
  // add to the particle deck json
  pDeckJson["Particle"] = pGeomJson;

  //// create particle mesh deck
  auto pMeshJson = inp::ParticleDeck::getParticleMeshExampleJson({"mesh_annulus_rectangle.msh",
      "mesh_drum2d.msh", "mesh_circle.msh"});
  pDeckJson["Mesh"] = pMeshJson;

  //// create particle material json
  auto pMatJson = inp::ParticleDeck::getParticleMaterialExampleJson(2); // two material groups

  // material 1
  pMatJson["Set_1"] = inp::MaterialDeck::getExampleJson("PDState", false, -1.,
    2.2, 1200., 25000., 1200., 500., true, 1);

  // material 2 (copy from material 1)
  pMatJson["Set_2"] = {{"Copy_Data", 1}};

  // add to the json
  pDeckJson["Material"] = pMatJson;

  //// create particle contact json
  auto pContactJson = inp::ParticleDeck::getParticleContactExampleJson(2);

  // contact pair 1 - 1
  pContactJson["Set_1_1"] = inp::ContactPairDeck::getExampleJson(0.95,
      true, false, false,
      1e+22, 0.95, 0., 1., 1., 1., 0., 25000.);
  // copy other pairs
  pContactJson["Set_1_2"] = {{"Copy_Data", std::vector<int>({1, 1})}};
  pContactJson["Set_2_2"] = {{"Copy_Data", std::vector<int>({1, 1})}};

  // add to the json
  pDeckJson["Contact"] = pContactJson;

  //// create particle neighbor json
  pDeckJson["Neighbor"] = inp::PNeighborDeck::getExampleJson("simple_all", 5, 10, 0.5);

  //// create particle generation json
  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");

  // add data that will be used to create particles
  pGenJson["Data"]["N"] = 3; // three particles

  // p1
  pGenJson["Data"]["0"] = {{"x", 0.}, {"y", 0.}, {"z", 0.}, {"theta", 0.}, {"s", 1.},
                           {"geom_id", 0}, {"mat_id", 0}, {"contact_id", 0}};

  // p1
  pGenJson["Data"]["1"] = {{"x", 0.}, {"y", 0.}, {"z", 0.}, {"theta", 0.}, {"s", 1.},
                           {"geom_id", 1}, {"mat_id", 1}, {"contact_id", 1}};

  // p1
  pGenJson["Data"]["2"] = {{"x", 0.}, {"y", 0.}, {"z", 0.}, {"theta", 0.}, {"s", 1.},
                           {"geom_id", 2}, {"mat_id", 1}, {"contact_id", 1}};

  // add to json
  pDeckJson["Particle_Generation"] = pGenJson;

  //// we now have the particle information in json object, so we can read them one by one
  // read json object
  std::cout << "\n\nPrinting particle deck json:\n";
  std::cout << pDeckJson.dump(2) << std::endl;

  deck->d_particleDeck_p = std::make_shared<inp::ParticleDeck>(pDeckJson, deck->d_modelDeck_p->d_particleSimType);

  std::cout << "\n\nPrinting particle deck:\n";
  std::cout << deck->d_particleDeck_p->printStr() << std::endl;


  //// collect json objects into global json object that will show the structure of input file
  auto inputJson = json({{"Model", modelDeckJson}, {"Test", testDeckJson},
    {"Output", outputDeckJson}, {"Displacement_BC", bcDeckJson["Displacement_BC"]},
    {"IC", bcDeckJson["IC"]}, {"Particle", pDeckJson["Particle"]}, {"Mesh", pDeckJson["Mesh"]},
    {"Material", pDeckJson["Material"]}, {"Contact", pDeckJson["Contact"]},
    {"Neighbor", pDeckJson["Neighbor"]}, {"Particle_Generation", pDeckJson["Particle_Generation"]}});

  std::cout << "\n\nPrinting global input deck json:\n";
  std::cout << inputJson.dump(2) << std::endl;
  // save to file
  std::ofstream f("input.json");
  f << inputJson.dump(2);
  f.close();
}
