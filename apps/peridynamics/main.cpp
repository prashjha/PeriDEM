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

#include "model/dem/demModel.h"
#include "material/materialUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/geomObjects.h"
#include "util/matrix.h"
#include "util/methods.h"
#include "util/point.h"
#include "inp/pdecks/contactDeck.h"
#include "rw/reader.h"
#include "util/function.h"
#include "util/geom.h"
#include "util/methods.h"
#include "util/randomDist.h"
#include "util/parallelUtil.h"
#include "inp/decks/materialDeck.h"
#include "inp/decks/modelDeck.h"
#include "inp/decks/outputDeck.h"
#include "inp/decks/restartDeck.h"
#include "rw/vtkParticleWriter.h"
#include "rw/vtkParticleReader.h"
#include "fe/elemIncludes.h"
#include "fe/meshUtil.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

/*!
 * @brief Namespace to define peridynamics apps that is meant to simulate single particle deformation.
 */
namespace peridynamics {

using util::io::log;

/*!
 * @brief Main model class to simulate peridynamics deformation of single particle
 */
class Model : public model::DEMModel {

public:

  /*!
   * @brief Constructor
   *
   * @param deck Input deck
   */
  explicit Model(inp::Input *deck) : model::DEMModel(deck, "peridynamics::Model") {}

  /*!
   * @brief Initialize model data
   */
  void init() override {
    // init time step
    d_n = 0;
    d_time = 0.;
    if (d_outputDeck_p->d_dtTestOut == 0)
      d_outputDeck_p->d_dtTestOut = d_outputDeck_p->d_dtOut / 10;
    d_infoN = d_outputDeck_p->d_dtOut;

    // debug/information variables
    {
      appendKeyData("debug_once", -1);
      appendKeyData("update_contact_neigh_search_params_init_call_count", 0);
      appendKeyData("tree_compute_time", 0);
      appendKeyData("contact_compute_time", 0);
      appendKeyData("contact_neigh_update_time", 0);
      appendKeyData("peridynamics_neigh_update_time", 0);
      appendKeyData("pd_compute_time", 0);
      appendKeyData("extf_compute_time", 0);
      appendKeyData("integrate_compute_time", 0);
      appendKeyData("pt_cloud_update_time", 0);
      appendKeyData("avg_tree_update_time", 0);
      appendKeyData("avg_contact_neigh_update_time", 0);
      appendKeyData("avg_contact_force_time", 0);
      appendKeyData("avg_peridynamics_force_time", 0);
      appendKeyData("avg_extf_compute_time", 0);
      appendKeyData("pen_dist", 0);
      appendKeyData("max_y", 0);
      appendKeyData("contact_area_radius", 0);
    }

    auto t1 = steady_clock::now();
    auto t2 = steady_clock::now();
    log(d_name + ": Initializing objects.\n");

    // create particles
    log(d_name + ": Creating particles.\n");
    createParticles();

    log(d_name + ": Creating maximum velocity data for particles.\n");
    d_maxVelocityParticlesListTypeAll
            = std::vector<double>(d_particlesListTypeAll.size(), 0.);
    d_maxVelocity = util::methods::max(d_maxVelocityParticlesListTypeAll);

    // setup element-node connectivity data if needed
    log(d_name + ": Setting up element-node connectivity data for strain/stress.\n");
    setupQuadratureData();

    // create search object
    log(d_name + ": Creating neighbor search tree.\n");

    // create tree object
    d_nsearch_p = std::make_unique<NSearch>(d_x, d_outputDeck_p->d_debug);

    // setup tree
    double set_tree_time = d_nsearch_p->setInputCloud();
    log(fmt::format("{}: Tree setup time (ms) = {}. \n", d_name, set_tree_time));

    // create neighborlists
    log(d_name + ": Creating neighborlist for peridynamics.\n");
    t1 = steady_clock::now();
    updatePeridynamicNeighborlist();
    t2 = steady_clock::now();
    appendKeyData("peridynamics_neigh_update_time", util::methods::timeDiff(t1, t2));

    // create peridynamic bonds
    log(d_name + ": Creating peridynamics bonds.\n");
    d_fracture_p = std::make_unique<geometry::Fracture>(&d_x, &d_neighPd);

    // compute quantities in state-based simulations
    log(d_name + ": Compute state-based peridynamic quantities.\n");
    material::computeStateMx(this, true);

    // initialize loading class
    log(d_name + ": Initializing displacement loading object.\n");
    d_uLoading_p =
            std::make_unique<loading::ParticleULoading>(d_pDeck_p->d_dispDeck);
    for (auto &p : d_particlesListTypeAll)
      d_uLoading_p->setFixity(p);

    log(d_name + ": Initializing force loading object.\n");
    d_fLoading_p =
            std::make_unique<loading::ParticleFLoading>(d_pDeck_p->d_forceDeck);

    log(d_name + ": Creating list of nodes on which force is to be computed.\n");
    for (size_t i = 0; i < d_x.size(); i++) {
      const auto &ptId = d_ptId[i];
      const auto &pi = getParticleFromAllList(ptId);
      if (pi->d_computeForce) {
        d_fContCompNodes.push_back(i);
        d_fPdCompNodes.push_back(i);
      }
    }

    // initialize remaining fields (if any)
    d_Z = std::vector<float>(d_x.size(), 0.);

    t2 = steady_clock::now();
    log(fmt::format("{}: Total setup time (ms) = {}. \n",
                    d_name, util::methods::timeDiff(t1, t2)));

    // compute complexity information
    size_t free_dofs = 0;
    for (const auto &f : d_fix) {
      for (size_t dof = 0; dof < 3; dof++)
        if (util::methods::isFree(f, dof))
          free_dofs++;
    }
    log(fmt::format("{}: Computational complexity information \n"
                    "  Total number of particles = {}, number of "
                    "particles = {}, number of walls = {}, \n"
                    "  number of dofs = {}, number of free dofs = {}. \n",
                    d_name, d_particlesListTypeAll.size(),
                    d_particlesListTypeParticle.size(),
                    d_particlesListTypeWall.size(),
                    3 * d_x.size(),
                    free_dofs));
  }

  /*!
   * @brief Create single particle with given information in the input deck
   */
  void createParticles() override {
    d_particlesListTypeParticle.resize(0);
    d_particlesListTypeAll.resize(0);
    d_particlesListTypeWall.resize(0);
    d_referenceParticles.clear();

    // loop over all particle zones
    for (size_t z = 0; z < d_pDeck_p->d_particleZones.size(); z++) {

      // does this particle zone correspond to particle or wall
      bool is_wall = d_pDeck_p->d_particleZones[z].d_isWall;
      std::string particle_type = d_pDeck_p->d_zoneToParticleORWallDeck[z].first;
      if (is_wall and particle_type != "wall") {
        std::cerr << fmt::format("Error: String d_zoneToParticleORWallDeck[z].first for zone z = {} "
                                 "should be 'wall'.\n", z);
        exit(EXIT_FAILURE);
      }
      if (!is_wall and particle_type != "particle") {
        std::cerr << fmt::format("Error: String d_zoneToParticleORWallDeck[z].first for zone z = {} "
                                 "should be 'particle'.\n", z);
        exit(EXIT_FAILURE);
      }

      // get current size of particles data
      auto psize = d_particlesListTypeAll.size();

      // get particle zone
      auto &pz = d_pDeck_p->d_particleZones[z];

      // get zone id
      auto z_id = pz.d_zone.d_zoneId;
      if (z_id != z) {
        std::cerr << fmt::format("Error: d_zoneId = {} in ParticleZone for "
                                 "z = {} should be equal to z = {}.\n",
                                 z_id, z, z);
        exit(EXIT_FAILURE);
      }

      // read mesh data
      log(d_name + ": Creating mesh for reference particle in zone = " +
          std::to_string(z_id) + "\n");
      auto mesh = std::make_shared<fe::Mesh>(&pz.d_meshDeck);

      // create the reference particle
      log(d_name + ": Creating reference particle in zone = " +
          std::to_string(z_id) + "\n");

      // get representative particle for this zone
      std::shared_ptr<util::geometry::GeomObject> rep_geom_p;
      std::vector<double> rep_geom_params;

      if (pz.d_geom_p->d_name == "null") {
        // create geometry based on mesh bounding box
        auto mesh_bbox = mesh->getBoundingBox();
        for (auto a : mesh_bbox.first)
          rep_geom_params.push_back(a);
        for (auto a : mesh_bbox.second)
          rep_geom_params.push_back(a);

        std::string rep_geom_name = "rectangle";
        if (d_modelDeck_p->d_dim == 3)
          rep_geom_name = "cuboid";

        util::geometry::createGeomObject(rep_geom_name,
                                         rep_geom_params,
                                         pz.d_geomComplexInfo.first,
                                         pz.d_geomComplexInfo.second,
                                         rep_geom_p,
                                         d_modelDeck_p->d_dim);
      }
      else {
        rep_geom_p = pz.d_geom_p;
        rep_geom_params = pz.d_geomParams;
      }

      auto ref_p = std::make_shared<particle::RefParticle>(
              d_referenceParticles.size(),
              static_cast<std::shared_ptr<ModelData>>(this),
              rep_geom_p,
              mesh);

      d_referenceParticles.emplace_back(ref_p);

      // create particle
      log(d_name + ": Creating particles in zone = " +
          std::to_string(z_id) + "\n");

      // ref_p has geometry and mesh which will be used in creating this particle
      // we need to create identity transform
      auto p_transform = particle::ParticleTransform();

      // create particle
      auto p = new particle::BaseParticle(
              pz.d_isWall ? "wall" : "particle",
              d_particlesListTypeAll.size(),
              pz.d_isWall ? d_particlesListTypeWall.size() : d_particlesListTypeParticle.size(),
              z_id,
              ref_p->getDimension(),
              pz.d_particleDescription,
              pz.d_isWall,
              pz.d_allDofsConstrained,
              ref_p->getNumNodes(),
              0.,
              static_cast<std::shared_ptr<ModelData>>(this),
              ref_p,
              ref_p->getGeomP(),
              p_transform,
              ref_p->getMeshP(),
              pz.d_matDeck,
              true);

      // push p to list
      if (pz.d_isWall)
        d_particlesListTypeWall.push_back(p);
      else
        d_particlesListTypeParticle.push_back(p);

      d_particlesListTypeAll.push_back(p);

      // get new size of data
      auto psize_new = d_particlesListTypeAll.size();

      // store this in zone-info
      d_zInfo.emplace_back(std::vector<size_t>{psize, psize_new, z_id});
    }
  }

  /*!
   * @brief Compute forces
   */
  void computeForces() override {

    bool dbg_condition = d_n % d_infoN == 0;

    log("  Compute forces \n", 2, dbg_condition, 3);

    // reset force
    auto t1 = steady_clock::now();
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
            (std::size_t) 0, d_x.size(), (std::size_t) 1,
            [this](std::size_t i) { this->d_f[i] = util::Point(); }
    ); // for_each

    executor.run(taskflow).get();
    auto force_reset_time = util::methods::timeDiff(t1, steady_clock::now());

    // compute peridynamic forces
    t1 = steady_clock::now();
    computePeridynamicForces();
    auto pd_time = util::methods::timeDiff(t1, steady_clock::now());
    appendKeyData("pd_compute_time", pd_time);
    appendKeyData("avg_peridynamics_force_time", pd_time/d_infoN);

    // Compute external forces
    t1 = steady_clock::now();
    computeExternalForces();
    auto extf_time = util::methods::timeDiff(t1, steady_clock::now());
    appendKeyData("extf_compute_time", extf_time);
    appendKeyData("avg_extf_compute_time", extf_time/d_infoN);

    // output avg time info
    if (dbg_condition) {
      log(fmt::format("    Avg time (ms): \n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n",
                      "peridynamics force", size_t(getKeyData("avg_peridynamics_force_time")),
                      "external force", size_t(getKeyData("avg_extf_compute_time")/d_infoN)),
          2, dbg_condition, 3);

      appendKeyData("avg_peridynamics_force_time", 0.);
      appendKeyData("avg_extf_compute_time", 0.);
    }

    log(fmt::format("    {:50s} = {:8d} \n",
                    "Force reset time (ms)",
                    size_t(force_reset_time)
        ),
        2, dbg_condition, 3);

    log(fmt::format("    {:50s} = {:8d} \n",
                    "Peridynamics force time (ms)",
                    size_t(pd_time)
        ),
        2, dbg_condition, 3);

    log(fmt::format("    {:50s} = {:8d} \n",
                    "External force time (ms)",
                    size_t(extf_time)
        ),
        2, dbg_condition, 3);
  }

};
} // namespace peridynamics

int main(int argc, char *argv[]) {

  // print program version
  std::cout << "Peridynamics (PeriDEM)"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl << std::flush;

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h")) {
    // print help
    std::cout << "Syntax to run the app: ./Peridynamics -i <input file> -nThreads <number of threads>";
    std::cout << "Example: ./Peridynamics -i input.yaml -nThreads 2";
  }

  // read input arguments
  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = 2;
    util::io::print(fmt::format("Running Peridynamics with number of threads = {}\n", nThreads));
  }
  // set number of threads
  util::parallel::initNThreads(nThreads);
  util::io::print(fmt::format("Number of threads = {}\n", util::parallel::getNThreads()));

  std::string filename;
  if (input.cmdOptionExists("-i"))
    filename = input.getCmdOption("-i");
  else {
    filename = "./example/input_0.yaml";
    util::io::print(fmt::format("Running Peridynamics with example input file = {}\n", filename));
  }

  // current time
  auto begin = steady_clock::now();

  // create deck
  auto *deck = new inp::Input(filename);

  // check which model to run
  if (deck->isPeriDEM()) {
    // ensure two variables in the deck are set
    deck->getModelDeck()->d_populateElementNodeConnectivity = true;

    // simulate model
    peridynamics::Model dem(deck);
    dem.run(deck);
  }

  // get time elapsed
  auto end = steady_clock::now();

  std::cout << "Total simulation time (s) = " 
            << util::methods::timeDiff(begin, end, "seconds") 
            << std::endl;
}