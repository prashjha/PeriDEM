/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "input.h"
#include "decks/materialDeck.h"
#include "decks/modelDeck.h"
#include "decks/outputDeck.h"
#include "decks/restartDeck.h"
#include "inp/decks/meshDeck.h"
#include "inp/pdecks/contactDeck.h"
#include "inp/pdecks/particleDeck.h"
#include <cmath>
#include <inp/pdecks/contactDeck.h>
#include <iostream>
#include <util/methods.h>
#include <yaml-cpp/yaml.h>

namespace {

inline bool definitelyGreaterThan(const double &a, const double &b) {
  return (a - b) >
         ((std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * 1.0E-5);
}

void rectDataSizeWarning() {
  std::cout << "Warning: We now require rectangle to be specified "
               "by 3 dimensional coordinates of corner points. In "
               "future, this will be strongly preferred.\n";
}

std::string stringVecSerial(const std::vector<std::string> &svec) {
  if (svec.empty())
    return "";

  std::string s = "";
  for (size_t i=0; i<svec.size(); i++) {
    s = s + svec[i];
    if (i < svec.size() - 1)
      s = s + "; ";
  }

  return s;
}

void readGeometry(YAML::Node data,
           std::string data_block_name,
           std::string &name,
           std::vector<double> &params,
           std::pair<std::vector<std::string>, std::vector<std::string>> &complexInfo) {

  if (!data["Type"]) {
    std::cerr << "Error; Can not read geometry type in yaml data in block = "
              << data_block_name << ".\n";
    exit(EXIT_FAILURE);
  }
  name = data["Type"].as<std::string>();

  if (name == "complex") {
    // read vector of types and flags
    if (data["Vec_Type"]) {
      for (auto f: data["Vec_Type"])
        complexInfo.first.push_back(f.as<std::string>());
    } else {
      std::cerr << "Error: To define complex geometry, we require vector of "
                   "types in yaml data block = "
                << data_block_name << "\n";
      exit(EXIT_FAILURE);
    }

    if (data["Vec_Flag"]) {
      for (auto f: data["Vec_Flag"])
        complexInfo.second.push_back(f.as<std::string>());
    } else {
      std::cerr << "Error: To define complex geometry, we require vector of "
                   "flags in yaml data block = "
                << data_block_name << "\n";
      exit(EXIT_FAILURE);
    }
  }

  params.clear();
  if (data["Parameters"]) {
    for (auto f : data["Parameters"])
      params.push_back(f.as<double>());
  } else {
    std::cerr << "Error: Parameters defining geometry is not present in data block = "
              << data_block_name << ".\n";
    exit(EXIT_FAILURE);
  }
}

} // namespace

inp::Input::Input(std::string filename, bool createDefault)
    : d_inputFilename(filename), d_createDefault(createDefault), d_meshDeck_p(nullptr), d_materialDeck_p(nullptr), d_outputDeck_p(nullptr),
      d_modelDeck_p(nullptr) {

  if (d_inputFilename.empty()) {
    if (d_createDefault) {
      std::cout
              << "Input: YAML input filename is empty and createDefault is true. "
                 "Default input decks will be created.\n";
    } else {
      std::cerr << "Error: YAML input filename is empty and createDefault is false. Exiting.\n";
      exit(EXIT_FAILURE);
    }
  }

  // follow the order of reading
  setModelDeck();
  setOutputDeck();

  if (d_modelDeck_p->d_isPeriDEM) {
    setParticleDeck();
    setContactDeck();
  }
  setRestartDeck();

  if (!d_modelDeck_p->d_isPeriDEM)
    setMeshDeck();

  if (!d_modelDeck_p->d_isPeriDEM)
    setMaterialDeck();
}

void createDefaultInputConfiguration() {

}

//
// accessor methods
//
std::shared_ptr<inp::MaterialDeck> inp::Input::getMaterialDeck() { return
                                                                    d_materialDeck_p; }

std::shared_ptr<inp::MeshDeck> inp::Input::getMeshDeck() { return
                                                            d_meshDeck_p; }

std::shared_ptr<inp::ModelDeck> inp::Input::getModelDeck() { return
                                                              d_modelDeck_p; }

std::shared_ptr<inp::OutputDeck> inp::Input::getOutputDeck() { return
                                                                d_outputDeck_p; }

std::shared_ptr<inp::RestartDeck> inp::Input::getRestartDeck() { return
                                                                  d_restartDeck_p; }

std::shared_ptr<inp::ParticleDeck> inp::Input::getParticleDeck() { return
                                                                    d_particleDeck_p; }
std::shared_ptr<inp::ContactDeck> inp::Input::getContactDeck() { return
                                                                  d_contactDeck_p; }

bool inp::Input::isPeriDEM() { return d_modelDeck_p->d_isPeriDEM; }

//
// setter methods
//
void inp::Input::setModelDeck() {
  d_modelDeck_p = std::make_shared<inp::ModelDeck>();

  if (d_inputFilename.empty() and d_createDefault)
    return;

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  // read dimension
  if (config["Model"]["Dimension"])
    d_modelDeck_p->d_dim = config["Model"]["Dimension"].as<size_t>();
  else {
    std::cerr << "Error: Please specify the dimension.\n";
    exit(1);
  }

  // read discretization info
  if (config["Model"]["Discretization_Type"]["Time"])
    d_modelDeck_p->d_timeDiscretization =
        config["Model"]["Discretization_Type"]["Time"].as<std::string>();

  if (config["Model"]["Discretization_Type"]["Spatial"])
    d_modelDeck_p->d_spatialDiscretization =
        config["Model"]["Discretization_Type"]["Spatial"].as<std::string>();


  if (d_modelDeck_p->d_timeDiscretization == "central_difference" or
      d_modelDeck_p->d_timeDiscretization == "velocity_verlet")
    d_modelDeck_p->d_simType = "explicit";

  if (config["Model"]["Populate_ElementNodeConnectivity"])
    d_modelDeck_p->d_populateElementNodeConnectivity = true;

  if (config["Model"]["Quad_Approximation_Order"])
    d_modelDeck_p->d_quadOrder = config["Model"]["Quad_Approximation_Order"].as<size_t>();

  // check if this is peridem simulation
  if (config["Particle"])
    d_modelDeck_p->d_isPeriDEM = true;

  // read horizon and horizon to mesh ratio (if available)
  if (config["Model"]["Horizon"])
    d_modelDeck_p->d_horizon = config["Model"]["Horizon"].as<double>();
  if (config["Model"]["Horizon_h_Ratio"])
    d_modelDeck_p->d_rh = config["Model"]["Horizon_h_Ratio"].as<int>();
  if (config["Model"]["Mesh_Size"])
    d_modelDeck_p->d_h = config["Model"]["Mesh_Size"].as<double>();

  if (!config["Model"]["Horizon"]) {

    // check and issue error if this is not peridem simulation
    if (!d_modelDeck_p->d_isPeriDEM)
      if (!config["Model"]["Horizon_h_Ratio"] or
          !config["Model"]["Mesh_Size"]) {

        std::cerr << "Error: Horizon is not provided. In this case "
                     "Horizon_h_Ratio and Mesh_Size are necessary to compute "
                     "horizon.\n";
        exit(1);
      }

    d_modelDeck_p->d_horizon = d_modelDeck_p->d_h * double(d_modelDeck_p->d_rh);
  }

  if (!config["Model"]["Mesh_Size"])
    if (config["Model"]["Horizon_h_Ratio"])
      d_modelDeck_p->d_h =
          d_modelDeck_p->d_horizon / double(d_modelDeck_p->d_rh);

  // read final time and time step
  if (config["Model"]["Final_Time"])
    d_modelDeck_p->d_tFinal = config["Model"]["Final_Time"].as<double>();
  if (config["Model"]["Time_Steps"])
    d_modelDeck_p->d_Nt = config["Model"]["Time_Steps"].as<int>();

  if (std::abs(d_modelDeck_p->d_tFinal) < 1.0E-10 or d_modelDeck_p->d_Nt <= 0) {
    std::cerr << "Error: Check Final_Time and Time_Steps data.\n";
    exit(1);
  }

  d_modelDeck_p->d_dt = d_modelDeck_p->d_tFinal / d_modelDeck_p->d_Nt;

  // check if this is restart problem
  if (config["Restart"])
    d_modelDeck_p->d_isRestartActive = true;

  // check if this is restart problem
  if (config["Model"]["Seed"])
    d_modelDeck_p->d_seed = config["Model"]["Seed"].as<int>();;
} // setModelDeck

void inp::Input::setParticleDeck() {
  d_particleDeck_p = std::make_shared<inp::ParticleDeck>();

  if (d_inputFilename.empty() and d_createDefault)
    return;

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  // <<<< >>>>
  // <<<< STEP 1 - Read container geometry details >>>>
  // <<<< >>>>
  // read particle container geometry and size
  if (config["Container"]["Geometry"]) {
    auto ce = config["Container"]["Geometry"];

    readGeometry(ce,
            "Container->Geometry",
            d_particleDeck_p->d_contGeomName,
            d_particleDeck_p->d_contGeomParams,
            d_particleDeck_p->d_contGeomComplexInfo);

    util::geometry::createGeomObject(
            d_particleDeck_p->d_contGeomName,
            d_particleDeck_p->d_contGeomParams,
            d_particleDeck_p->d_contGeomComplexInfo.first,
            d_particleDeck_p->d_contGeomComplexInfo.second,
            d_particleDeck_p->d_contGeom_p,
            d_modelDeck_p->d_dim);
    //std::cout << d_particleDeck_p->d_contGeom_p->printStr(0, 0) << std::flush;
  } else {
    std::cerr << "Error: Need particle container information.\n";
    exit(1);
  }

  // <<<< >>>>
  // <<<< STEP 2 - Read Zone data >>>>
  // <<<< >>>>
  // read input file
  if (!config["Zone"]) {
    std::cerr << "Error: Zone information is not provided.\n";
    exit(1);
  }
  auto n = config["Zone"]["Zones"].as<size_t>();
  if (n == 0) {
    std::cerr << "Error: Require at least one zone to define particles.\n";
    exit(1);
  }

  d_particleDeck_p->d_zoneVec.resize(n);
  d_particleDeck_p->d_particleZones.resize(n);
  d_particleDeck_p->d_zoneToParticleORWallDeck.resize(n);

  // read zone and particle information
  for (size_t z = 1; z <= n; z++) {

    std::string read_zone = "Zone_";
    read_zone.append(std::to_string(z));

    auto e = config["Zone"][read_zone];

    // <<<< >>>>
    // <<<< STEP 2.1 - Read this zone information >>>>
    // <<<< >>>>
    auto zone_data = inp::Zone();
    zone_data.d_zoneId = z - 1;
    setZoneData({"Zone", read_zone}, &zone_data);

    // store zone data
    d_particleDeck_p->d_zoneVec[z-1] = zone_data;

    // <<<< >>>>
    // <<<< STEP 2.2 - Read particle information in this zone >>>>
    // <<<< >>>>
    auto particle_data = inp::ParticleZone();
    particle_data.d_zone = zone_data;
    particle_data.d_isWall = false;
    if (e["Is_Wall"])
      particle_data.d_isWall = e["Is_Wall"].as<bool>();

    setParticleData(read_zone, &particle_data);

    // <<<< >>>>
    // <<<< STEP 2.3 - Read mesh and reference particle information >>>>
    // <<<< >>>>
    setZoneMeshDeck({"Mesh", read_zone}, &(particle_data.d_meshDeck));

    // <<<< >>>>
    // <<<< STEP 2.4 - Read material properties of this zone >>>>
    // <<<< >>>>
    setZoneMaterialDeck({"Material", read_zone}, &(particle_data.d_matDeck),
                        z);

    // add to the list
    d_particleDeck_p->d_particleZones[z-1] = particle_data;

    // update zone to particle/wall deck map
    d_particleDeck_p->d_zoneToParticleORWallDeck[z - 1] =
            std::make_pair(particle_data.d_isWall ? "wall" : "particle", z - 1);
  } // read zone and particle information

  // <<<< >>>>
  // <<<< STEP 3 - Read neighbor search data >>>>
  // <<<< >>>>
  if (config["Neighbor"]) {
    auto ne = config["Neighbor"];
    if (ne["Update_Criteria"])
      d_particleDeck_p->d_pNeighDeck.d_updateCriteria =
          ne["Update_Criteria"].as<std::string>();

    if (ne["Search_Factor"])
      d_particleDeck_p->d_pNeighDeck.d_sFactor =
          ne["Search_Factor"].as<double>();

    if (ne["Search_Interval"])
      d_particleDeck_p->d_pNeighDeck.d_neighUpdateInterval =
              ne["Search_Interval"].as<size_t>();

    for (auto p : d_particleDeck_p->d_particleZones) {

      // if particle is actually a wall then we do it differently
      double tol_check = 0.;
      if (p.d_isWall) {

        // check if wall has geometry object and it is not a null object
        if (p.d_geom_p->d_name != "null" and !p.d_geom_p->d_name.empty()) {
          if (p.d_geomParams.empty()) {
            std::cerr
                    << "Error: Cannot set neighbor list data as the geometrical "
                       "parameter for the particle/wall is missing.\n";
            exit(EXIT_FAILURE);
          }
          tol_check = p.d_geomParams[0];
        } else {
          // use container geometry or zone geometry to get the tolerance parameter
          if (p.d_zone.d_geomParams.empty()) {
            std::cerr
                    << "Error: Cannot set neighbor list data as the geometrical "
                       "parameter the zone this wall/particle belongs to is missing.\n";
            exit(EXIT_FAILURE);
          }
          tol_check = p.d_zone.d_geomParams[0];
        } // if else for wall geometry
      } else {
        if (p.d_geomParams.empty()) {
          std::cerr
                  << "Error: Cannot set neighbor list data as the geometrical "
                     "parameter for the particle is missing.\n";
          exit(EXIT_FAILURE);
        }
        tol_check = p.d_geomParams[0];
      }

      if (util::isGreater(tol_check, d_particleDeck_p->d_pNeighDeck.d_sTol))
        d_particleDeck_p->d_pNeighDeck.d_sTol = tol_check;
    }

    // multiply by the search factor
    d_particleDeck_p->d_pNeighDeck.d_sTol *=
        d_particleDeck_p->d_pNeighDeck.d_sFactor;
  } else {
    std::cerr << "Error: Need neighbor list information.\n";
    exit(1);
  }

  // <<<< >>>>
  // <<<< STEP 4 - Read gravity force information if any >>>>
  // <<<< >>>>
  if (config["Force_BC"]) {
    if (config["Force_BC"]["Gravity"]) {

      d_particleDeck_p->d_gravityActive = true;

      auto gf = std::vector<double>(3, 0.);
      size_t gf_c = 0;
      for (auto e : config["Force_BC"]["Gravity"])
        gf[gf_c++] = e.as<double>();

      d_particleDeck_p->d_gravity = util::Point(gf[0], gf[1], gf[2]);
    }
  }

  // <<<< >>>>
  // <<<< STEP 5 - Read force bc and displacement bc >>>>
  // <<<< >>>>
  std::vector<std::string> vtags = {"Displacement_BC", "Force_BC"};

  for (const auto &tag : vtags) {

    // get number of force bc sets
    int nsets = 0;
    if (config[tag]["Sets"])
      nsets = config[tag]["Sets"].as<int>();

    for (size_t s = 1; s <= nsets; s++) {

      // prepare string Set_s to read file
      std::string read_set = "Set_";
      read_set.append(std::to_string(s));
      auto e = config[tag][read_set];

      auto bc_deck = PBCData();

      // selection type
      if (e["Selection_Type"])
        bc_deck.d_selectionType = e["Selection_Type"].as<std::string>();

      // find selection type based on data provided
      if (e["Region"]) {

        bc_deck.d_selectionType = "region";

        if (e["Particle_List"])
          bc_deck.d_selectionType += "_with_include_list";

        if (e["Wall_List"]) {
          std::cerr << "Error: We only accept 'Particle_List'.\n";
          exit(EXIT_FAILURE);
        }

        if (e["Particle_Exclude_List"])
            bc_deck.d_selectionType += "_with_exclude_list";
      } else {

        if (e["Particle_List"])
          bc_deck.d_selectionType = "particle";

        if (e["Wall_List"]) {
          std::cerr << "Error: We only accept 'Particle_List'.\n";
          exit(EXIT_FAILURE);
        }
      }

      // check if region is provided
      if (e["Region"]) {

        if (e["Region"]["Rectangle"]) {

          std::vector<double> locs;
          for (auto f : e["Region"]["Rectangle"])
            locs.push_back(f.as<double>());

          util::geometry::checkParamForGeometry(locs.size(), "rectangle");

          // create rectangle object
          bc_deck.d_regionGeom_p = std::make_shared<util::geometry::Rectangle>(
              util::Point(locs[0], locs[1], locs[2]),
              util::Point(locs[3], locs[4], locs[5]));
        } else if (e["Region"]["Circle"]) {

          std::vector<double> locs;
          for (auto f : e["Region"]["Circle"])
            locs.push_back(f.as<double>());

          util::geometry::checkParamForGeometry(locs.size(), "circle");

          // create rectangle object
          bc_deck.d_regionGeom_p = std::make_shared<util::geometry::Circle>(
              locs[0], util::Point(locs[1], locs[2], locs[3]));
        } else if (e["Region"]["Cuboid"]) {
          std::vector<double> locs;
          for (auto f : e["Region"]["Cuboid"])
            locs.push_back(f.as<double>());

          util::geometry::checkParamForGeometry(locs.size(), "cuboid");

          // create cuboid object
          bc_deck.d_regionGeom_p = std::make_shared<util::geometry::Cuboid>(
              util::Point(locs[0], locs[1], locs[2]),
              util::Point(locs[3], locs[4], locs[5]));
        } else if (e["Region"]["Sphere"]) {

          std::vector<double> locs;
          for (auto f : e["Region"]["Sphere"])
            locs.push_back(f.as<double>());

          if (locs.size() != 4) {
            std::cerr << "Error: Data " << tag << "->Set_" << s
                      << "Region->Sphere is not correct in size. We "
                         "require radius and coordinates (three numbers) of "
                         "center of sphere/circle\n";
            exit(1);
          }

          // create rectangle object
          bc_deck.d_regionGeom_p = std::make_shared<util::geometry::Sphere>(
              locs[0], util::Point(locs[1], locs[2], locs[3]));
        }
      } // if region

      // check if particle list is provided
      if (e["Particle_List"]) {
        for (auto f : e["Particle_List"])
          bc_deck.d_pList.emplace_back(f.as<size_t>());
      }

      // check if wall list is provided
      if (e["Wall_List"]) {
        std::cerr << "Error: We only accept 'Particle_List'.\n";
        exit(EXIT_FAILURE);
      }

      // check if particle exclude list is provided
      if (e["Particle_Exclude_List"]) {
        for (auto f : e["Particle_Exclude_List"])
          bc_deck.d_pNotList.emplace_back(f.as<size_t>());
      }

      // read direction
      for (auto j : e["Direction"])
        bc_deck.d_direction.push_back(j.as<size_t>());

      // read time function type
      if (e["Time_Function"]) {
        bc_deck.d_timeFnType = e["Time_Function"]["Type"].as<std::string>();
        if (e["Time_Function"]["Parameters"])
          for (auto j : e["Time_Function"]["Parameters"])
            bc_deck.d_timeFnParams.push_back(j.as<double>());
      }

      if (e["Spatial_Function"]) {
        bc_deck.d_spatialFnType =
            e["Spatial_Function"]["Type"].as<std::string>();
        if (e["Spatial_Function"]["Parameters"])
          for (auto j : e["Spatial_Function"]["Parameters"])
            bc_deck.d_spatialFnParams.push_back(j.as<double>());
      }

      // for displacement bc, check if this is simple zero displacement
      // condition
      if (tag == "Displacement_BC" && e["Zero_Displacement"])
        bc_deck.d_isDisplacementZero = e["Zero_Displacement"].as<bool>();

      if (!bc_deck.d_regionGeom_p)
        bc_deck.d_regionGeom_p = std::make_shared<util::geometry::GeomObject>
            ("dummy_region");

      // add data
      if (tag == "Displacement_BC")
        d_particleDeck_p->d_dispDeck.push_back(bc_deck);
      else
        d_particleDeck_p->d_forceDeck.push_back(bc_deck);
    } // loading sets
  }

  // <<<< >>>>
  // <<<< STEP 6 - Read initial condition data >>>>
  // <<<< >>>>
  if (config["IC"]) {
    if (config["IC"]["Constant_Velocity"]) {

      d_particleDeck_p->d_icDeck.d_icActive = true;

      auto icv = std::vector<double>(3, 0.);
      size_t icv_c = 0;
      for (auto e : config["IC"]["Constant_Velocity"]["Velocity_Vector"])
        icv[icv_c++] = e.as<double>();

      d_particleDeck_p->d_icDeck.d_icVec = util::Point(icv[0], icv[1], icv[2]);

      // get particle ids
      d_particleDeck_p->d_icDeck.d_pList.clear();
      for (auto e : config["IC"]["Constant_Velocity"]["Particle_List"])
        d_particleDeck_p->d_icDeck.d_pList.push_back(e.as<size_t>());
    }
  }

  // <<<< >>>>
  // <<<< STEP 7 - Read test name (if any) >>>>
  // <<<< >>>>
  if (config["Particle"]["Test_Name"]) {
    d_particleDeck_p->d_testName =
        config["Particle"]["Test_Name"].as<std::string>();

    if (d_particleDeck_p->d_testName == "compressive_test") {
      if (!config["Particle"]["Compressive_Test"]) {
        std::cerr << "Error: For compressive test, we require more "
                     "information such as wall id and wall force direction\n";
        exit(0);
      }

      // check
      if (config["Particle"]["Compressive_Test"]["Wall_Id"]) {
        std::cerr << "Error: We only accept 'Particle_Id' in 'config[Particle][Compressive_Test]'.\n";
        exit(EXIT_FAILURE);
      }
      d_particleDeck_p->d_particleIdCompressiveTest =
              config["Particle"]["Compressive_Test"]["Particle_Id"].as<size_t>();

      if (config["Particle"]["Compressive_Test"]["Wall_Force_Direction"]) {
        std::cerr << "Error: We only accept 'Particle_Force_Direction' in 'config[Particle][Compressive_Test]'.\n";
        exit(EXIT_FAILURE);
      }
      d_particleDeck_p->d_particleForceDirectionCompressiveTest =
          config["Particle"]["Compressive_Test"]["Particle_Force_Direction"]
              .as<size_t>();

    }
  } // test block
} // setParticleDeck

void inp::Input::setContactDeck() {
  d_contactDeck_p = std::make_shared<inp::ContactDeck>();

  if (d_inputFilename.empty() and d_createDefault)
    return;

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  if (d_particleDeck_p == nullptr) {
    std::cerr << "Error: Particle data must be read and populated before "
                 "reading contact data"
              << std::endl;
    exit(1);
  }

  // loop over zones
  size_t n = d_particleDeck_p->d_zoneVec.size();

  d_contactDeck_p->d_data.resize(n);
  for (size_t i = 0; i < n; i++)
    d_contactDeck_p->d_data[i].resize(n);

  for (size_t z = 1; z <= n; z++) {
    for (size_t zz = z; zz <= n; zz++) {
      std::string read_zone = "Zone_" + std::to_string(z) + std::to_string(zz);

      if (d_outputDeck_p->d_debug > 1)
        std::cout << "Processing pair (z, zz) = (" << z << ", " << zz << ").\n";

      auto e = config["Contact"][read_zone];

      // check if we copy the contact data from previous zone pair
      if (e["Copy_Contact_Data"]) {

        // get pair of zone id to copy
        std::vector<size_t> zone_pair;
        for (auto j : e["Copy_Contact_Data"])
          zone_pair.push_back(j.as<size_t>());

        if (zone_pair.size() != 2) {
          std::cerr << "Error: Copy_Contact_Data in Contact deck for zone pair "
                    << read_zone << " is invalid.\n";
          exit(1);
        }

        // check if zone_pair data has been read (if not then issue error)
        //if (zone_pair[0] > z or zone_pair[1] > zz - 1) {
        if (zone_pair[0] > z or (zone_pair[0] ==  z and zone_pair[1] >= zz)) {
          std::cerr << "Error: Can not copy the contact data for zone = " <<
              read_zone << " as the pair of data = ("
                           << zone_pair[0] << ", " << zone_pair[1]
                           << ") required to copy have not "
                           "been read.\n";
        }

        // create a copy of contact data
        inp::ContactPairDeck cd =
            d_contactDeck_p->d_data[zone_pair[0] - 1][zone_pair[1] - 1];

        if (d_outputDeck_p->d_debug > 1) {
          std::cout << "Processing pair (z, zz) = (" << z << ", " << zz << "). "
                    << "Copying contact data from (n, m) = (" << zone_pair[0]
                    << ", " << zone_pair[1] << ").\n";
          std::cout << cd.printStr();
        }

        // copy to the data
        d_contactDeck_p->d_data[z - 1][zz - 1] = cd;
        d_contactDeck_p->d_data[zz - 1][z - 1] = cd;
      } else {

        auto cd = inp::ContactPairDeck();
        //if (z == zz) {
          if (e["Contact_Radius_Factor"]) {
            cd.d_contactR = e["Contact_Radius_Factor"].as<double>();
            cd.d_computeContactR = true;
          } else if (e["Contact_Radius"]) {
            cd.d_contactR = e["Contact_Radius"].as<double>();
            cd.d_computeContactR = false;
          }
        //}

        if (e["Damping_On"])
          cd.d_dampingOn = e["Damping_On"].as<bool>();

        if (e["Friction_On"])
          cd.d_frictionOn = e["Friction_On"].as<bool>();

        if (!e["Kn"]) {

          bool v_max_found = false;
          bool delta_max_found = false;

          if (e["V_Max"])
            v_max_found = true;
          if (e["Delta_Max"])
            delta_max_found = true;

          if (v_max_found and delta_max_found) {
            cd.d_vMax = e["V_Max"].as<double>();
            cd.d_deltaMax = e["Delta_Max"].as<double>();
          }

          if (v_max_found and !delta_max_found) {
            cd.d_vMax = e["V_Max"].as<double>();
            cd.d_deltaMax = 1.;
          }

          if (!v_max_found) {
            std::cerr << "Error: Need V_Max parameter for contact force. Either"
                         " provide V_Max (and/or Delta_Max) or provide Kn.\n"
                      << "Processing pair (z, zz) = (" << z << ", " << zz << ").\n"
                      << "Contact deck info\n"
                      << cd.printStr();
            exit(1);
          }
        } else {

          cd.d_Kn = e["Kn"].as<double>();

          cd.d_deltaMax = 1.;
          cd.d_vMax = std::sqrt(cd.d_Kn);
        }

        if (cd.d_dampingOn) {
          if (e["Epsilon"])
            cd.d_eps = e["Epsilon"].as<double>();
          else {
            std::cerr << "Error: Epsilon needed" << std::endl;
            exit(1);
          }
        } else {
          cd.d_eps = 1.;
        }

        if (cd.d_dampingOn && util::isLess(cd.d_betanFactor, 1.0e-8))
          cd.d_dampingOn = false;

        if (cd.d_frictionOn) {
          if (e["Friction_Coeff"])
            cd.d_mu = e["Friction_Coeff"].as<double>();
          else {
            std::cerr << "Error: Friction_Coeff needed" << std::endl;
            exit(1);
          }
        } else {
          cd.d_mu = 0.;
        }

        if (e["Kn_Factor"])
          cd.d_KnFactor = e["Kn_Factor"].as<double>();
        if (e["Beta_n_Factor"])
          cd.d_betanFactor = e["Beta_n_Factor"].as<double>();

        if (!cd.d_dampingOn)
          cd.d_betanFactor = 0.;

        // add contact data to list (symmetric list)
        d_contactDeck_p->d_data[z - 1][zz - 1] = cd;
        d_contactDeck_p->d_data[zz - 1][z - 1] = cd;
      }
    }
  }
} // setContactDeck

void inp::Input::setRestartDeck() {
  d_restartDeck_p = std::make_shared<inp::RestartDeck>();
  if (d_inputFilename.empty() and d_createDefault)
    return;

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  // read restart file
  if (config["Restart"]) {
    if (config["Restart"]["File"])
      d_restartDeck_p->d_file = config["Restart"]["File"].as<std::string>();
    else {
      std::cerr << "Error: Please specify the file for restart.\n";
      exit(1);
    }

    // read time step from which to begin
    if (config["Restart"]["Step"])
      d_restartDeck_p->d_step = config["Restart"]["Step"].as<size_t>();
    else {
      std::cerr << "Error: Please specify the time step from which to restart "
                   "the simulation.\n";
      exit(1);
    }

    if (config["Restart"]["Change_Reference_Free_Dofs"])
      d_restartDeck_p->d_changeRefFreeDofs =
          config["Restart"]["Change_Reference_Free_Dofs"].as<bool>();
  }
} // setRestartDeck

void inp::Input::setMeshDeck() {
  d_meshDeck_p = std::make_shared<inp::MeshDeck>();
  if (d_inputFilename.empty() and d_createDefault)
    return;

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  // read dimension
  if (config["Model"]["Dimension"])
    d_meshDeck_p->d_dim = config["Model"]["Dimension"].as<size_t>();

  // read spatial discretization type
  if (config["Model"]["Discretization_Type"]["Spatial"])
    d_meshDeck_p->d_spatialDiscretization =
        config["Model"]["Discretization_Type"]["Spatial"].as<std::string>();

  if (config["Model"]["Populate_ElementNodeConnectivity"])
    d_meshDeck_p->d_populateElementNodeConnectivity = true;

  if (config["Model"]["Quad_Approximation_Order"])
    d_meshDeck_p->d_quadOrder = config["Model"]["Quad_Approximation_Order"].as<size_t>();

  // read mesh filename
  if (config["Mesh"]["File"])
    d_meshDeck_p->d_filename = config["Mesh"]["File"].as<std::string>();
  else {

    std::cerr << "Error: Please specify mesh filename.\n";
    exit(1);
  }

  if (d_modelDeck_p->d_h < 1.0E-12)
    d_meshDeck_p->d_computeMeshSize = true;
  else
    d_meshDeck_p->d_h = d_modelDeck_p->d_h;
} // setMeshDeck

void inp::Input::setMaterialDeck() {
  d_materialDeck_p = std::make_shared<inp::MaterialDeck>();
  if (d_inputFilename.empty() and d_createDefault)
    return;

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  auto e = config["Material"];
  if (e["Is_Plane_Strain"])
    d_materialDeck_p->d_isPlaneStrain = e["Is_Plane_Strain"].as<bool>();
  if (e["Type"])
    d_materialDeck_p->d_materialType = e["Type"].as<std::string>();
  else {
    std::cerr << "Error: Please specify the material type.\n";
    exit(1);
  }
  if (e["Compute_From_Classical"])
    d_materialDeck_p->d_computeParamsFromElastic =
        e["Compute_From_Classical"].as<bool>();
  if (e["E"])
    d_materialDeck_p->d_matData.d_E = e["E"].as<double>();
  if (e["G"])
    d_materialDeck_p->d_matData.d_G = e["G"].as<double>();
  if (e["K"])
    d_materialDeck_p->d_matData.d_K = e["K"].as<double>();
  if (e["Lambda"])
    d_materialDeck_p->d_matData.d_lambda = e["Lambda"].as<double>();
  if (e["Mu"])
    d_materialDeck_p->d_matData.d_mu = e["Mu"].as<double>();
  if (e["Poisson_Ratio"])
    d_materialDeck_p->d_matData.d_nu = e["Poisson_Ratio"].as<double>();
  if (e["Gc"])
    d_materialDeck_p->d_matData.d_Gc = e["Gc"].as<double>();
  if (e["KIc"])
    d_materialDeck_p->d_matData.d_KIc = e["KIc"].as<double>();

  // read pairwise (bond) potential
  if (e["Bond_Potential"]) {
    auto f = e["Bond_Potential"];
    if (f["Type"])
      d_materialDeck_p->d_bondPotentialType = f["Type"].as<size_t>();
    if (f["Check_Sc_Factor"])
      d_materialDeck_p->d_checkScFactor = f["Check_Sc_Factor"].as<double>();
    if (f["Irreversible_Bond_Fracture"])
      d_materialDeck_p->d_irreversibleBondBreak =
          f["Irreversible_Bond_Fracture"].as<bool>();
    if (f["Parameters"])
      for (auto j : f["Parameters"])
        d_materialDeck_p->d_bondPotentialParams.push_back(j.as<double>());
  }

  // read hydrostatic (state) potential
  if (e["State_Potential"]) {
    auto f = e["State_Potential"];
    if (f["Type"])
      d_materialDeck_p->d_statePotentialType = f["Type"].as<size_t>();
    if (f["Contribution_From_Broken_Bond"])
      d_materialDeck_p->d_stateContributionFromBrokenBond =
          f["Contribution_From_Broken_Bond"].as<bool>();
    if (f["Parameters"])
      for (auto j : f["Parameters"])
        d_materialDeck_p->d_statePotentialParams.push_back(j.as<double>());
  }

  // read influence function
  if (e["Influence_Function"]) {
    auto f = e["Influence_Function"];
    if (f["Type"])
      d_materialDeck_p->d_influenceFnType = f["Type"].as<size_t>();
    if (f["Parameters"])
      for (auto j : f["Parameters"])
        d_materialDeck_p->d_influenceFnParams.push_back(j.as<double>());
  }

  // read density
  if (e["Density"])
    d_materialDeck_p->d_density = e["Density"].as<double>();
  else {
    std::cerr << "Error: Please specify the density of the material.\n";
    exit(1);
  }
} // setMaterialDeck

void inp::Input::setOutputDeck() {
  d_outputDeck_p = std::make_shared<inp::OutputDeck>();
  if (d_inputFilename.empty() and d_createDefault)
    return;

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  if (config["Output"]) {
    auto e = config["Output"];
    if (e["File_Format"])
      d_outputDeck_p->d_outFormat = e["File_Format"].as<std::string>();
    if (e["Path"])
      d_outputDeck_p->d_path = e["Path"].as<std::string>();
    if (e["Tags"])
      for (auto f : e["Tags"])
        d_outputDeck_p->d_outTags.push_back(f.as<std::string>());

    if (e["Output_Interval"])
      d_outputDeck_p->d_dtOut = e["Output_Interval"].as<size_t>();
    d_outputDeck_p->d_dtOutOld = d_outputDeck_p->d_dtOut;
    d_outputDeck_p->d_dtOutCriteria = d_outputDeck_p->d_dtOut;
    if (e["Debug"])
      d_outputDeck_p->d_debug = e["Debug"].as<size_t>();
    if (e["Perform_FE_Out"])
      d_outputDeck_p->d_performFEOut = e["Perform_FE_Out"].as<bool>();
    if (e["Compress_Type"])
      d_outputDeck_p->d_compressType = e["Compress_Type"].as<std::string>();
    if (e["Output_Criteria"]) {
      if (e["Output_Criteria"]["Type"])
        d_outputDeck_p->d_outCriteria =
            e["Output_Criteria"]["Type"].as<std::string>();
      if (e["Output_Criteria"]["New_Interval"])
        d_outputDeck_p->d_dtOutCriteria =
            e["Output_Criteria"]["New_Interval"].as<size_t>();
      if (e["Output_Criteria"]["Parameters"]) {
        auto ps = e["Output_Criteria"]["Parameters"];
        for (auto p : ps)
          d_outputDeck_p->d_outCriteriaParams.emplace_back(p.as<double>());
      }
    }

    if (e["Perform_Out"])
      d_outputDeck_p->d_performOut = e["Perform_Out"].as<bool>();
    if (e["Test_Output_Interval"])
      d_outputDeck_p->d_dtTestOut = e["Test_Output_Interval"].as<size_t>();
    if (e["Tag_PP"])
      d_outputDeck_p->d_tagPPFile = e["Tag_PP"].as<std::string>();
  }
} // setOutputDeck

void inp::Input::setZoneMaterialDeck(std::vector<std::string> s_config,
                                     inp::MaterialDeck *m_deck, size_t zone_id) {

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  auto e = config[s_config[0]];
  if (s_config.size() > 1) {
    if (s_config.size() == 2)
      e = config[s_config[0]][s_config[1]];
    else if (s_config.size() == 3)
      e = config[s_config[0]][s_config[1]][s_config[2]];
  }

  if (!e) {
    std::cerr << "Error: Can not read material properties for given zone.\n";
    exit(1);
  }

  // check if we copy the material data from previous material zone deck
  if (e["Copy_Material_Data"]) {

    // get the zone id from which we need to read the material data
    auto copy_zone_id = e["Copy_Material_Data"].as<size_t>();

    if (copy_zone_id >= zone_id) {
      std::cerr << "Error: Invalid zone id for Material deck corresponding to"
                   " zone = " << zone_id << ".\n";
      exit(1);
    }

    s_config[1] = "Zone_" + std::to_string(copy_zone_id);

    // make sure the copy zone does not have copy tag
    if (config[s_config[0]][s_config[1]]["Copy_Material_Data"]) {
      std::cerr << "Error: Tag " << s_config[0] << "->" << s_config[1]
                << " should not have tag Copy_Material_Data\n";
      exit(1);
    }

    setZoneMaterialDeck(s_config, m_deck, copy_zone_id);
    return;
  }

  if (e["Is_Plane_Strain"])
    m_deck->d_isPlaneStrain = e["Is_Plane_Strain"].as<bool>();
  if (e["Type"])
    m_deck->d_materialType = e["Type"].as<std::string>();
  else {
    std::cerr << "Error: Please specify the material type.\n";
    exit(1);
  }
  if (e["Compute_From_Classical"])
    m_deck->d_computeParamsFromElastic = e["Compute_From_Classical"].as<bool>();
  if (e["E"])
    m_deck->d_matData.d_E = e["E"].as<double>();
  if (e["G"])
    m_deck->d_matData.d_G = e["G"].as<double>();
  if (e["K"])
    m_deck->d_matData.d_K = e["K"].as<double>();
  if (e["Lambda"])
    m_deck->d_matData.d_lambda = e["Lambda"].as<double>();
  if (e["Mu"])
    m_deck->d_matData.d_mu = e["Mu"].as<double>();
  if (e["Poisson_Ratio"])
    m_deck->d_matData.d_nu = e["Poisson_Ratio"].as<double>();
  if (e["Gc"])
    m_deck->d_matData.d_Gc = e["Gc"].as<double>();
  if (e["KIc"])
    m_deck->d_matData.d_KIc = e["KIc"].as<double>();

  // read pairwise (bond) potential
  if (e["Bond_Potential"]) {
    auto f = e["Bond_Potential"];
    if (f["Type"])
      m_deck->d_bondPotentialType = f["Type"].as<size_t>();
    if (f["Check_Sc_Factor"])
      m_deck->d_checkScFactor = f["Check_Sc_Factor"].as<double>();
    if (f["Irreversible_Bond_Fracture"])
      m_deck->d_irreversibleBondBreak =
          f["Irreversible_Bond_Fracture"].as<bool>();
    if (f["Parameters"])
      for (auto j : f["Parameters"])
        m_deck->d_bondPotentialParams.push_back(j.as<double>());
  }

  // read hydrostatic (state) potential
  if (e["State_Potential"]) {
    auto f = e["State_Potential"];
    if (f["Type"])
      m_deck->d_statePotentialType = f["Type"].as<size_t>();
    if (f["Contribution_From_Broken_Bond"])
      m_deck->d_stateContributionFromBrokenBond =
          f["Contribution_From_Broken_Bond"].as<bool>();
    if (f["Parameters"])
      for (auto j : f["Parameters"])
        m_deck->d_statePotentialParams.push_back(j.as<double>());
  }

  // read influence function
  if (e["Influence_Function"]) {
    auto f = e["Influence_Function"];
    if (f["Type"])
      m_deck->d_influenceFnType = f["Type"].as<size_t>();
    if (f["Parameters"])
      for (auto j : f["Parameters"])
        m_deck->d_influenceFnParams.push_back(j.as<double>());
  }

  // read density
  if (e["Density"])
    m_deck->d_density = e["Density"].as<double>();
  else {
    std::cerr << "Error: Please specify the density of the material.\n";
    exit(1);
  }

  // read horizon
  if (e["Horizon"])
    m_deck->d_horizon = e["Horizon"].as<double>();

  // read horizon to mesh ratio
  if (e["Horizon_Mesh_Ratio"])
    m_deck->d_horizonMeshRatio = e["Horizon_Mesh_Ratio"].as<double>();

  // std::cout << m_deck->printStr(0, 0) << "\n";
} // setMaterialDeck

void inp::Input::setZoneMeshDeck(std::vector<std::string> s_config,
                                 inp::MeshDeck *mesh_deck) {

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  auto e = config[s_config[0]];
  if (s_config.size() > 1) {
    if (s_config.size() == 2)
      e = config[s_config[0]][s_config[1]];
    else if (s_config.size() == 3)
      e = config[s_config[0]][s_config[1]][s_config[2]];
  }

  if (!e) {
    std::cerr << "Error: Can not read mesh properties for given zone.\n";
    exit(1);
  }

  // read dimension
  mesh_deck->d_dim = config["Model"]["Dimension"].as<size_t>();

  // read spatial discretization type
  if (config["Model"]["Discretization_Type"]["Spatial"])
    mesh_deck->d_spatialDiscretization =
        config["Model"]["Discretization_Type"]["Spatial"].as<std::string>();

  // read mesh filename
  if (e["File"])
    mesh_deck->d_filename = e["File"].as<std::string>();
  else {
    std::cerr << "Error: Please specify mesh filename.\n";
    exit(1);
  }

  if (e["Mesh_Size"]) {
    mesh_deck->d_h = e["Mesh_Size"].as<double>();
    mesh_deck->d_computeMeshSize = false;
  } else
    mesh_deck->d_computeMeshSize = true;

  //mesh_deck->print();
}

void inp::Input::setZoneData(std::vector<std::string> s_config,
                             inp::Zone *zone_data) {

  YAML::Node config = YAML::LoadFile(d_inputFilename);
  auto e = config[s_config[0]];
  if (s_config.size() > 1)
    e = config[s_config[0]][s_config[1]];
  if (s_config.size() > 2)
    e = config[s_config[0]][s_config[1]][s_config[2]];

  if (!e) {
    std::cerr << "Error: Can not read zone properties for given zone.\n";
    exit(1);
  }

  // read zone information
  if (e["Type"]) {
    readGeometry(e, stringVecSerial(s_config),
                 zone_data->d_geomName,
                 zone_data->d_geomParams,
                 zone_data->d_geomComplexInfo);

    util::geometry::createGeomObject(
            zone_data->d_geomName,
            zone_data->d_geomParams,
            zone_data->d_geomComplexInfo.first,
            zone_data->d_geomComplexInfo.second,
            zone_data->d_geom_p,
            d_modelDeck_p->d_dim,
            false);
  } else {
    // assign container geometry
    this->d_particleDeck_p->copyContainerGeometry(
            zone_data->d_geomName,
            zone_data->d_geomParams,
            zone_data->d_geomComplexInfo,
            zone_data->d_geom_p);
  }
}

void inp::Input::setParticleData(std::string string_zone,
                                 inp::ParticleZone *particle_data) {

  YAML::Node config = YAML::LoadFile(d_inputFilename);

  auto pe = config["Particle"][string_zone];

  if (particle_data->d_isWall) {
    if (!pe) {
      auto wall_pe = config["Particle"][string_zone];
      if (!wall_pe) {
        std::cerr << "Error: For this particle/wall in zone = " << string_zone
                  << " , neither config['Particle'] "
                     "nor config['Wall'] exist in input file.\n";
        exit(EXIT_FAILURE);
      } else {
        pe = wall_pe;
        std::cout << "Warning: In future, for wall, separate config['Wall'] "
                     "will not be used.\n";
      }
    }
  }

  // <<<< >>>>
  // <<<< STEP 1 - Read dof computation information >>>>
  // <<<< >>>>
  particle_data->d_allDofsConstrained = false;
  if (pe["All_Dofs_Constrained"])
    particle_data->d_allDofsConstrained = pe["All_Dofs_Constrained"].as<bool>();

  particle_data->d_createParticleUsingParticleZoneGeomObject = false;
  if (pe["Create_Particle_Using_ParticleZone_GeomObject"])
    particle_data->d_createParticleUsingParticleZoneGeomObject
      = pe["Create_Particle_Using_ParticleZone_GeomObject"].as<bool>();

  // <<<< >>>>
  // <<<< STEP 2 - Read geometry type and create geometry >>>>
  // <<<< >>>>
  if (pe) {

    readGeometry(pe, "Particle->" + string_zone,
                 particle_data->d_geomName,
                 particle_data->d_geomParams,
                 particle_data->d_geomComplexInfo);

    // create particle
    util::geometry::createGeomObject(particle_data->d_geomName,
                                     particle_data->d_geomParams,
                                     particle_data->d_geomComplexInfo.first,
                                     particle_data->d_geomComplexInfo.second,
                                     particle_data->d_geom_p,
                                     d_modelDeck_p->d_dim);

    // read test name (if any)
    if (pe["Near_Bd_Nodes_Tol"])
      particle_data->d_nearBdNodesTol =
              pe["Near_Bd_Nodes_Tol"].as<double>();
  }
  else {
    if (particle_data->d_createParticleUsingParticleZoneGeomObject) {
      // create geometry from container as we do not really need geometry object
      // for this particle created through
      this->d_particleDeck_p->copyContainerGeometry(
              particle_data->d_geomName,
              particle_data->d_geomParams,
              particle_data->d_geomComplexInfo,
              particle_data->d_geom_p);
    }
    else {
      std::cerr << "Error: Particle/wall details for zone = " << string_zone
                << "is not provided.\n";
      exit(1);
    }
  }

  // <<<< >>>>
  // <<<< STEP 3 - Read reference particle geometry type and create reference geometry >>>>
  // <<<< >>>>
  auto re = config["Mesh"][string_zone]["Reference_Particle"];
  std::string refGeomType;
  if (re) {

    readGeometry(re, "Particle->" + string_zone + "->Reference_Particle",
                 particle_data->d_refParticleGeomName,
                 particle_data->d_refParticleGeomParams,
                 particle_data->d_refParticleGeomComplexInfo);

    util::geometry::createGeomObject(refGeomType, particle_data->d_refParticleGeomParams,
                                     particle_data->d_refParticleGeomComplexInfo.first,
                                     particle_data->d_refParticleGeomComplexInfo.second,
                                     particle_data->d_refParticleGeom_p,
                                     d_modelDeck_p->d_dim);

  } else {
    // use the general particle data for the reference particle as well
    bool copy_from_ref = false;
    particle_data->copyContainerGeometry(
            particle_data->d_refParticleGeomName,
            particle_data->d_refParticleGeomParams,
            particle_data->d_refParticleGeomComplexInfo,
            particle_data->d_refParticleGeom_p,
            copy_from_ref);
  }

  // <<<< >>>>
  // <<<< STEP 4 - Read particle generation method >>>>
  // <<<< >>>>
  if (!particle_data->d_createParticleUsingParticleZoneGeomObject) {
    if (config["Particle_Generation"]["From_File"]) {

      particle_data->d_genMethod = "From_File";
      particle_data->d_particleFile =
              config["Particle_Generation"]["From_File"].as<std::string>();

      if (config["Particle_Generation"]["File_Data_Type"]) {
        particle_data->d_particleFileDataType =
                config["Particle_Generation"]["File_Data_Type"].as<std::string>();
      } else {
        particle_data->d_particleFileDataType = "loc_rad";
      }
    } else {
      std::cerr << "Error: Particle_Generation->From_File block is not provided"
                   ". Can not generate particles for dem simulation. "
                   "Terminating the simulation.\n";
      exit(1);
    }
  }
  else {
    particle_data->d_genMethod = "Create_Particle_Using_ParticleZone_GeomObject";
  }
}