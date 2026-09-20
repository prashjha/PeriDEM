/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * Each inp::*Deck lists its fields twice, once in getExampleJson, which writes
 * the block, and once in readFromJson, which reads it. No test compared the
 * two, and they disagreed in three places. BCBaseDeck emitted Region,
 * Time_Function and Spatial_Function as JSON arrays, where readFromJson calls
 * at() and value() on them as objects, and Parameters overwrote Type.
 * TestDeck emitted an array for Test_Name. MaterialDeck wrote Is_Plane_Strain
 * and readFromJson read Is_Plain_Strain.
 *
 * For each deck this writes a block with values other than the defaults, reads
 * it back, and compares the values. A disagreement between the two lists fails
 * here rather than producing a deck that cannot be read.
 */

#include "inp/deckIncludes.h"
#include "geom/geomObjectsUtil.h"
#include "util/io.h"
#include "util/json.h"
#include "util/point.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string &what) {
  ++g_checks;
  if (!ok) {
    ++g_failures;
    std::cerr << "  FAIL: " << what << "\n";
  }
}

void checkClose(double got, double want, const std::string &what,
                double tol = 1.0e-12) {
  check(std::abs(got - want) <= tol * std::max(1.0, std::abs(want)),
        what + " (got " + std::to_string(got) + ", want " +
            std::to_string(want) + ")");
}

/*! The readers call at() and value() on these blocks, so each is an object. */
void checkIsObject(const json &j, const std::string &key) {
  check(j.contains(key), key + " present");
  if (j.contains(key))
    check(j.at(key).is_object(), key + " is an object, not an array");
}

void testModelDeck() {
  std::cout << "ModelDeck\n";
  auto j = inp::ModelDeck::getExampleJson(3, 0.25, 500, "finite_difference",
                                          "central_difference", false, 3,
                                          "Single_Particle", 7);
  inp::ModelDeck d;
  d.readFromJson(j);
  check(d.d_dim == 3, "Dimension round-trips");
  checkClose(d.d_tFinal, 0.25, "Final_Time round-trips");
  check(d.d_Nt == 500, "Time_Steps round-trips");
  check(!d.d_populateElementNodeConnectivity,
        "Populate_ElementNodeConnectivity round-trips");
  check(d.d_quadOrder == 3, "Quad_Approximation_Order round-trips");
  check(d.d_particleSimType == "Single_Particle",
        "Particle_Sim_Type round-trips");
  check(d.d_seed == 7, "Seed round-trips");
  checkClose(d.d_dt, 0.25 / 500., "dt derived from Final_Time / Time_Steps");
}

void testOutputDeck() {
  std::cout << "OutputDeck\n";
  auto j = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", "out/"},
           {"Tags", std::vector<std::string>({"Displacement", "Damage_Z"})},
           {"Output_Interval", 25},
           {"Debug", 1},
           {"Perform_FE_Out", false},
           {"Compress_Type", "zlib"},
           {"Perform_Out", true},
           {"Test_Output_Interval", 5},
           {"Tag_PP", "3"},
           {"PVD_Collection", true}});
  inp::OutputDeck d;
  d.readFromJson(j);
  check(d.d_path == "out/", "Path round-trips");
  check(d.d_dtOut == 25, "Output_Interval round-trips");
  check(d.d_debug == 1, "Debug round-trips");
  check(!d.d_performFEOut, "Perform_FE_Out round-trips");
  check(d.d_dtTestOut == 5, "Test_Output_Interval round-trips");
  check(d.d_tagPPFile == "3", "Tag_PP round-trips");
  check(d.d_pvdCollection, "PVD_Collection round-trips");
  check(d.d_outTags.size() == 2, "Tags round-trip");
}

void testMaterialDeck() {
  std::cout << "MaterialDeck\n";
  // Plane strain on: the writer key and the reader key must agree.
  auto j = inp::MaterialDeck::getExampleJson("PMBBond", true, 6.0e-4, 0, 1200.,
                                             2.16e7, 1.296e7, 50., true, 1,
                                             3.24e7);
  inp::MaterialDeck d;
  d.readFromJson(j);
  check(d.d_materialType == "PMBBond", "Type round-trips");
  check(d.d_isPlaneStrain, "Is_Plane_Strain round-trips (writer/reader key)");
  checkClose(d.d_horizon, 6.0e-4, "Horizon round-trips");
  checkClose(d.d_density, 1200., "Density round-trips");
  check(d.d_influenceFnType == 1, "Influence_Function.Type round-trips");

  // Plane strain off must also survive.
  auto j2 = inp::MaterialDeck::getExampleJson("PDState", false, 6.0e-4, 0,
                                              1200., 2.16e7, 1.296e7, 50.,
                                              true, 0);
  inp::MaterialDeck d2;
  d2.readFromJson(j2);
  check(!d2.d_isPlaneStrain, "Is_Plane_Strain false round-trips");
}

void testMeshDeck() {
  std::cout << "MeshDeck\n";
  // A mesh size with no filename requests generation.
  auto j = inp::MeshDeck::getExampleJson(json{{"Mesh_Size", 1.0e-4}});
  inp::MeshDeck d;
  d.readFromJson(j);
  check(d.d_createMesh, "bare Mesh_Size still means CreateMesh");
  checkClose(d.d_hMeshing, 1.0e-4, "Mesh_Size round-trips");

  // File only.
  auto j2 = inp::MeshDeck::getExampleJson(json{{"File", "mesh.msh"}});
  inp::MeshDeck d2;
  d2.readFromJson(j2);
  check(d2.d_filename == "mesh.msh", "File round-trips");
  check(!d2.d_createMesh, "a plain File does not request mesh creation");

  // File plus generation, which is what the examples need.
  auto j3 = inp::MeshDeck::getExampleJson(
      json{{"File", "mesh.msh"},
           {"Mesh_Size", 2.0e-4},
           {"Create_Mesh", true},
           {"Info", "uniform"},
           {"Write_Mesh_File", false},
           {"Void_Regions", {{0., 0., 0., 1., 1., 1.}}}});
  inp::MeshDeck d3;
  d3.readFromJson(j3);
  check(d3.d_filename == "mesh.msh", "File round-trips with CreateMesh");
  check(d3.d_createMesh, "CreateMesh.Flag round-trips");
  check(d3.d_createMeshInfo == "uniform", "CreateMesh.Info round-trips");
  check(!d3.d_writeMeshFile, "Write_Mesh_File round-trips");
  checkClose(d3.d_hMeshing, 2.0e-4, "CreateMesh.Mesh_Size round-trips");
  check(d3.d_voidRegions.size() == 1, "Void_Regions round-trip");
}

void testContactPairDeck() {
  std::cout << "ContactPairDeck\n";
  auto j = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius_Factor", 0.95},
               {"Kn", 1.0e12},
               {"Damping_On", true},
               {"Epsilon", 0.9},
               {"Friction_On", true},
               {"Friction_Coeff", 0.5},
               {"Kn_Factor", 2.0},
               {"Beta_n_Factor", 100.},
               {"K", 2.16e7}});
  inp::ContactPairDeck d;
  d.readFromJson(j);
  check(d.d_computeContactR, "Contact_Radius_Factor round-trips");
  checkClose(d.d_contactR, 0.95, "contact radius factor value");
  checkClose(d.d_Kn, 1.0e12, "Kn round-trips");
  checkClose(d.d_eps, 0.9, "Epsilon round-trips");
  check(d.d_frictionOn, "Friction_On round-trips");
  checkClose(d.d_mu, 0.5, "Friction_Coeff round-trips");
  checkClose(d.d_KnFactor, 2.0, "Kn_Factor round-trips");
  checkClose(d.d_betanFactor, 100., "Beta_n_Factor round-trips");

  // Damping off must zero the damping factor, not carry it.
  auto j2 = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius_Factor", 0.95},
               {"Kn", 1.0e12},
               {"Damping_On", false},
               {"Epsilon", 0.9},
               {"Friction_On", false},
               {"Friction_Coeff", 0.},
               {"Kn_Factor", 1.},
               {"Beta_n_Factor", 100.},
               {"K", 2.16e7}});
  inp::ContactPairDeck d2;
  d2.readFromJson(j2);
  check(!d2.d_dampingOn, "Damping_On false round-trips");
  checkClose(d2.d_betanFactor, 0., "Beta_n_Factor zeroed when damping is off");
}

void testContactDeck() {
  std::cout << "ContactDeck\n";
  auto j = inp::ContactDeck::getExampleJson(json{{"Sets", 2}});
  check(j.at("Sets") == 2, "Sets count");
  for (const auto &key : {"Set_1_1", "Set_1_2", "Set_2_2"})
    check(j.contains(key), std::string("pair ") + key + " present");
  check(j.contains("Damping_Law"), "Damping_Law present");
  check(j.contains("Correct_Volume"), "Correct_Volume present");
}

void testNeighborAndGenDecks() {
  std::cout << "PNeighborDeck / PGenDeck\n";
  auto j = inp::PNeighborDeck::getExampleJson(
          json{{"Update_Criteria", "simple_all"},
               {"Search_Factor", 8.},
               {"Search_Interval", 5},
               {"Near_Bd_Nodes_Tol", 0.25}});
  inp::PNeighborDeck d;
  d.readFromJson(j);
  check(d.d_updateCriteria == "simple_all", "Update_Criteria round-trips");
  checkClose(d.d_sFactor, 8., "Search_Factor round-trips");
  check(d.d_neighUpdateInterval == 5, "Search_Interval round-trips");
  checkClose(d.d_nearBdNodesTol, 0.25, "Near_Bd_Nodes_Tol round-trips");

  auto jg = inp::PGenDeck::getExampleJson(json{{"Method", "From_File"}});
  check(jg.at("Method") == "From_File", "Method present");
}

void testTestDeck() {
  std::cout << "TestDeck\n";
  // An empty name yields an empty block, which the reader ignores.
  auto empty = inp::TestDeck::getExampleJson(json{{"Test_Name", ""}});
  check(empty.empty(), "an unnamed test yields an empty block");

  // A plain test name. This used to be emitted as a JSON array.
  auto j = inp::TestDeck::getExampleJson(json{{"Test_Name", "two_particle"}});
  check(j.is_object(), "Test block is an object, not an array");
  inp::TestDeck d;
  d.readFromJson(j);
  check(d.d_testName == "two_particle", "Test_Name round-trips");

  // readFromJson requires the sub-block for both spellings of the name.
  for (const std::string name : {"Compressive_Test", "compressive_test"}) {
    auto jc = inp::TestDeck::getExampleJson(
        json{{"Test_Name", name},
             {"Compressive_Test",
              json{{"Wall_Id", 13}, {"Wall_Force_Direction", 2}}}});
    checkIsObject(jc, "Compressive_Test");
    inp::TestDeck dc;
    dc.readFromJson(jc);
    check(dc.d_testName == name, name + ": Test_Name round-trips");
    check(dc.d_particleIdCompressiveTest == 13, name + ": Wall_Id round-trips");
    check(dc.d_particleForceDirectionCompressiveTest == 2,
          name + ": Wall_Force_Direction round-trips");
  }
}

void testBCBaseDeck() {
  std::cout << "BCBaseDeck\n";
  geom::GeomData region;
  region.d_geomName = "rectangle";
  region.d_geomParams = {0., 0., 0., 1.0e-3, 1.0e-3, 0.};
  geom::createGeomObject(region);

  // Region plus both function blocks: all three used to come out as arrays.
  auto j = inp::BCBaseDeck::getExampleJson(
      json{{"Type", "Displacement_BC"},
           {"Particle_List", {0, 1}},
           {"Particle_Exclude_List", {2}},
           {"Direction", {1, 2}},
           {"Zero_Displacement", true},
           {"Time_Function",
            json{{"Type", "linear"},
                 {"Parameters", std::vector<double>{0.5}}}},
           {"Spatial_Function",
            json{{"Type", "constant"},
                 {"Parameters", std::vector<double>{}}}}},
      &region);
  checkIsObject(j, "Region");
  if (j.contains("Region"))
    checkIsObject(j.at("Region"), "Geometry");
  checkIsObject(j, "Time_Function");
  checkIsObject(j, "Spatial_Function");
  check(j.at("Time_Function").contains("Type"),
        "Time_Function.Type survives alongside Parameters");
  check(j.at("Time_Function").contains("Parameters"),
        "Time_Function.Parameters present");

  inp::BCBaseDeck d;
  d.readFromJson(j, "Displacement_BC");
  check(d.d_isRegionActive, "Region is picked up by the reader");
  check(d.d_regionGeomData.d_geomName == "rectangle",
        "Region geometry round-trips");
  check(d.d_timeFnType == "linear", "Time_Function.Type round-trips");
  check(d.d_timeFnParams.size() == 1, "Time_Function.Parameters round-trip");
  check(d.d_spatialFnType == "constant", "Spatial_Function.Type round-trips");
  check(d.d_pList.size() == 2, "Particle_List round-trips");
  check(d.d_pNotList.size() == 1, "Particle_Exclude_List round-trips");
  check(d.d_direction.size() == 2, "Direction round-trips");
  check(d.d_isDisplacementZero, "Zero_Displacement round-trips");

  // Initial condition shape.
  auto jic = inp::BCBaseDeck::getExampleJson(
      json{{"Type", "IC"},
           {"Particle_List", {1}},
           {"IC_Type", "Constant_Velocity"},
           {"IC_Vector", {0., -2.5, 0.}}});
  checkIsObject(jic, "Constant_Velocity");
  inp::BCBaseDeck dic;
  dic.readFromJson(jic, "IC");
  check(dic.d_icType == "Constant_Velocity", "IC type round-trips");
  checkClose(dic.d_icVec.d_y, -2.5, "IC velocity round-trips");
}

void testBCDeck() {
  std::cout << "BCDeck\n";
  auto j = inp::BCDeck::getExampleJson(
      json{{"Displacement_BC_Sets", 1},
           {"IC_Sets", 1},
           {"Gravity", util::Point(0., -10., 0.).toVec()}});
  checkIsObject(j, "Force_BC");
  check(j.at("Force_BC").contains("Gravity"), "Gravity present");
  checkIsObject(j, "Displacement_BC");
  checkIsObject(j, "IC");
}

} // namespace

namespace {

/*! A malformed block makes the reader throw. Recording that as a failed check
 *  rather than letting it propagate lets the remaining decks run, so one
 *  report names every deck that disagrees. */
void run(void (*fn)(), const std::string &name) {
  try {
    fn();
  } catch (const std::exception &e) {
    ++g_checks;
    ++g_failures;
    std::cerr << "  FAIL: " << name << " threw: " << e.what() << "\n";
  }
}

} // namespace

int main() {
  std::cout << "Deck factory round-trip\n"
            << "-----------------------\n";
  run(testModelDeck, "ModelDeck");
  run(testOutputDeck, "OutputDeck");
  run(testMaterialDeck, "MaterialDeck");
  run(testMeshDeck, "MeshDeck");
  run(testContactPairDeck, "ContactPairDeck");
  run(testContactDeck, "ContactDeck");
  run(testNeighborAndGenDecks, "PNeighborDeck/PGenDeck");
  run(testTestDeck, "TestDeck");
  run(testBCBaseDeck, "BCBaseDeck");
  run(testBCDeck, "BCDeck");

  std::cout << "-----------------------\n"
            << g_checks - g_failures << " / " << g_checks << " checks passed\n";
  if (g_failures > 0) {
    std::cerr << g_failures << " check(s) failed\n";
    return 1;
  }
  return 0;
}
