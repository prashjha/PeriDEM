/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "periDEMModel.h"

#include "contact/contact.h"
#include "pd/pdForce.h"
#include "nsearch/neighborPolicy.h"
#include "time_int/integrator.h"
#include "rw/particleOutput.h"
#include "postprocess/postprocess.h"

#include <algorithm>
#include <stdexcept>

// utils
#include "particle/baseParticle.h"
#include "material/materialUtil.h"
#include "util/function.h"
#include "geom/geomIncludes.h"
#include "util/matrix.h"
#include "util/vecMethods.h"
#include "util/point.h"
#include "inp/input.h"
#include "rw/reader.h"
#include "util/function.h"
#include "util/randomDist.h"
#include "util/parallelUtil.h"
#include "rw/vtkParticleWriter.h"
#include "rw/pvdCollectionWriter.h"
#include "rw/vtkParticleReader.h"
#include "mesh_gen/meshGenerator.h"
#include "loading/particleIC.h"
#include "util/io.h"
#include "particle/createParticles.h"
#include "particle/particleMpi.h"
#include "pd/pdMpi.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <random>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>


PeriDEMModel::PeriDEMModel(std::shared_ptr<inp::Input> & deck, std::string modelName)
  : data::ModelData(deck) {

  d_name = std::move(modelName);

  // Ensure output directory exists (Path from input; relative to cwd).
  {
    namespace fs = std::filesystem;
    fs::path out(d_outputDeck_p->d_path);
    if (!out.empty())
      fs::create_directories(out);
  }

  // initialize logger
  util::io::initLogger(d_outputDeck_p->d_debug,
                       d_outputDeck_p->d_path + "log.txt");
  if (!d_postprocess_p)
    d_postprocess_p = std::make_unique<postprocess::Postprocess>();
}

void PeriDEMModel::log(std::ostringstream &oss, int priority, bool check_condition, int override_priority,
                          bool screen_out) {
  util::io::log(priority, oss, check_condition, override_priority, screen_out);
}

void PeriDEMModel::log(const std::string &str, int priority, bool check_condition, int override_priority,
                          bool screen_out) {
  util::io::log(priority, str, check_condition, override_priority, screen_out);
}

void PeriDEMModel::run(std::shared_ptr<inp::Input> & deck) {

  // initialize data
  init();

  // check for restart
  if (d_modelDeck_p->d_isRestartActive)
    restart(deck);

  // integrate in time
  integrate();

  // close
  close();
}

void PeriDEMModel::restart(std::shared_ptr<inp::Input> & deck) {

  log(d_name + ": Restarting the simulation\n");

  // set time step to step specified in restart deck
  d_n = d_restartDeck_p->d_step;
  d_time = double(d_n) * d_modelDeck_p->d_dt;
  log(std::format("  Restart step = {}, time = {:.6f} \n", d_n, d_time));

  // get backup of reference configuration
  std::vector<util::Point> x_ref(d_x.size(), util::Point());
  for (auto &x : d_x)
    x_ref.push_back(x);

  // read displacement and velocity from restart file
  log("  Reading data from restart file = " + d_restartDeck_p->d_file + " \n");
  auto reader = rw::reader::VtkParticleReader(d_restartDeck_p->d_file);
  reader.readNodes(this);
}

void PeriDEMModel::close() {
  if (d_postprocess_p)
    d_postprocess_p->close(*this);
}

void PeriDEMModel::init() {

  // init time step
  d_n = 0;
  d_time = 0.;
  if (d_outputDeck_p->d_dtOut < 1)
    throw std::runtime_error(
        "Output_Interval must be >= 1 (use 1 to write every step).");
  if (d_outputDeck_p->d_dtOutCriteria < 1)
    d_outputDeck_p->d_dtOutCriteria = d_outputDeck_p->d_dtOut;
  if (d_outputDeck_p->d_dtOutOld < 1)
    d_outputDeck_p->d_dtOutOld = d_outputDeck_p->d_dtOut;
  if (d_outputDeck_p->d_dtTestOut == 0)
    d_outputDeck_p->d_dtTestOut = std::max<size_t>(1, d_outputDeck_p->d_dtOut / 10);
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
  particle::createParticles(*this);
  particle::assignMpiOwners(*this);

  log(d_name + ": Creating maximum velocity data for particles.\n");
  d_maxVelocityParticlesListTypeAll
          = std::vector<double>(d_particlesListTypeAll.size(), 0.);
  d_maxVelocity = util::methods::max(d_maxVelocityParticlesListTypeAll);

  // setup contact
  if (d_input_p->isMultiParticle()) {
    if (!d_contact_p)
      d_contact_p = std::make_unique<contact::Contact>();
    log(d_name + ": Setting up contact.\n");
    d_contact_p->setup(*this);
  }

  // setup element-node connectivity data if needed
  log(d_name + ": Setting up element-node connectivity data for strain/stress.\n");
  data::setupQuadratureData(*this);

  // create search object
  log(d_name + ": Creating neighbor search tree.\n");

  // create tree object
  d_nsearch_p = std::make_unique<NSearch>(d_x, d_outputDeck_p->d_debug);

  // setup tree
  double set_tree_time = d_nsearch_p->setInputCloud();
  log(std::format("{}: Tree setup time (ms) = {}. \n", d_name, set_tree_time));

  // create neighborlists
  log(d_name + ": Creating neighborlist for peridynamics.\n");
  t1 = steady_clock::now();
  nsearch::updatePeridynamicNeighborlist(*this);
  t2 = steady_clock::now();
  appendKeyData("peridynamics_neigh_update_time", util::methods::timeDiff(t1, t2));

  if (d_input_p->isMultiParticle()) {
    log(d_name + ": Creating neighborlist for contact.\n");
    d_contNeighUpdateInterval = d_particleDeck_p->d_pNeighDeck.d_neighUpdateInterval;
    d_contNeighSearchRadius = d_particleDeck_p->d_pNeighDeck.d_sFactor * d_maxContactR;
    t1 = steady_clock::now();
    d_contact_p->updateNeighborlist(*this);
    t2 = steady_clock::now();
    appendKeyData("contact_neigh_update_time", util::methods::timeDiff(t1, t2));
  }

  // create peridynamic bonds
  log(d_name + ": Creating peridynamics bonds.\n");
  d_fracture_p = std::make_unique<geometry::Fracture>(&d_x, &d_neighPd);

  // compute quantities in state-based simulations
  log(d_name + ": Compute state-based peridynamic quantities.\n");
  material::computeStateMx(this, true);

  // initialize loading class
  log(d_name + ": Initializing displacement loading object.\n");
  d_uLoading_p =
      std::make_unique<loading::ParticleULoading>(d_bcDeck_p->d_dispDeck);
  for (auto &p : d_particlesListTypeAll)
    d_uLoading_p->setFixity(p);

  log(d_name + ": Initializing force loading object.\n");
  d_fLoading_p =
      std::make_unique<loading::ParticleFLoading>(d_bcDeck_p->d_forceDeck);

  // if all dofs of particle is fixed, then mark it so that we do not
  // compute force
  // MAYBE NOT as we may be interested in reaction forces
  //  for (auto &p : d_particlesListTypeAll)
  //    p->checkFixityForForce(); // TODO implement

  // if this is a two-particle test, we set the force calculation off in
  // first particle
  if (d_testDeck_p->d_testName == "two_particle") {
    d_particlesListTypeAll[0]->d_computeForce = false;
  }

  log(std::format("{}: Total particles = {}. \n",
                  d_name, d_particlesListTypeAll.size()));

  for (const auto &p : d_particlesListTypeAll)
    if (!p->d_computeForce)
      log(std::format("{}: Force OFF in Particle i = {}. \n", d_name, p->getId()));

  // Single-particle DOF-MPI: Metis node owners + PD ghost plan (T10).
  pd::setupDofPartition(*this);

  log(d_name + ": Creating list of nodes on which force is to be computed.\n");
  // Selection is per particle or wall: a node enters d_fCompNodes if force is
  // computed on any node of its body. Selecting individual nodes is not
  // implemented.
  const int mpi_rank = util::parallel::mpiRank();
  for (size_t i = 0; i < d_x.size(); i++) {
    const auto &ptId = d_ptId[i];
    const auto &pi = getParticleFromAllList(ptId);
    bool node_owned = false;
    if (d_pdDofMpi)
      node_owned = (static_cast<int>(d_pdNodePartition[i]) == mpi_rank);
    else if (pi->isWall())
      // Walls are replicated; assemble wall contact/reaction on rank 0 only.
      node_owned = (mpi_rank == 0);
    else
      node_owned = particle::isLocallyOwned(*pi);
    if (pi->d_computeForce && node_owned) {
      d_fContCompNodes.push_back(i);
      // Walls keep contact (and reaction) but not peridynamic force. Treating a
      // container as a PD body on a thin/boolean mesh makes Damage_Z explode
      // and the neighbor search then allocates until the process is OOM-killed.
      if (!pi->isWall())
        d_fPdCompNodes.push_back(i);
    }
  }

  // initialize remaining fields (if any)
  d_Z = std::vector<float>(d_x.size(), 0.);
  // Damage: volume-weighted phi (Silling/Trask) and broken-bond count
  // fraction (Bhattacharya & Lipton).
  d_phi = std::vector<float>(d_x.size(), 0.);
  d_phiBond = std::vector<float>(d_x.size(), 0.);

  t2 = steady_clock::now();
  log(std::format("{}: Total setup time (ms) = {}. \n",
                  d_name, util::methods::timeDiff(t1, t2)));

  // compute complexity information
  size_t free_dofs = 0;
  for (const auto &f : d_fix) {
    for (size_t dof = 0; dof < 3; dof++)
      if (util::methods::isFree(f, dof))
        free_dofs++;
  }
  log(std::format("{}: Computational complexity information \n"
                  "  Total number of particles = {}, number of "
                  "particles = {}, number of walls = {}, \n"
                  "  number of dofs = {}, number of free dofs = {}. \n",
                  d_name, d_particlesListTypeAll.size(),
                  d_particlesListTypeParticle.size(),
                  d_particlesListTypeWall.size(),
                  3 * d_x.size(),
                  free_dofs));
}

void PeriDEMModel::integrate() {
  time_int::Integrator().integrate(*this);
}

void PeriDEMModel::integrateStep() {
  time_int::Integrator().step(*this);
}

void PeriDEMModel::computeForces() {

  bool dbg_condition = d_n % d_infoN == 0;

  log("  Compute forces \n", 2, dbg_condition, 3);

  // Refresh ghost grain kinematics from owners before contact / damping.
  {
    const auto t_ex0 = steady_clock::now();
    particle::exchangeGhostKinematics(*this);
    appendKeyData("mpi_exchange_wall_time",
                  util::methods::timeDiff(t_ex0, steady_clock::now()));
  }

  // Nodal DOF halo for PD neighbor reads (Single_Particle MPI).
  pd::exchangeGhostDisplacement(*this);

  // reset force
  auto t1 = steady_clock::now();
  float force_reset_time = 0;
  {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, d_x.size(), (std::size_t) 1,
        [this](std::size_t i) { this->d_f[i] = util::Point(); }
    ); // for_each

    executor.run(taskflow).get();
    force_reset_time = util::methods::timeDiff(t1, steady_clock::now());
  }

  // compute peridynamic forces
  t1 = steady_clock::now();
  pd::computeForces(*this);
  auto pd_time = util::methods::timeDiff(t1, steady_clock::now());
  appendKeyData("pd_compute_time", pd_time);
  appendKeyData("avg_peridynamics_force_time", pd_time/d_infoN);

  float current_contact_neigh_update_time = 0;
  float contact_time = 0;
  if (d_input_p->isMultiParticle() && d_contact_p) {
    // update contact neighborlist
    t1 = steady_clock::now();
    d_contact_p->updateNeighborlist(*this);
    current_contact_neigh_update_time = util::methods::timeDiff(t1,
                                                                     steady_clock::now());
    appendKeyData("contact_neigh_update_time",
                  current_contact_neigh_update_time);
    appendKeyData("avg_contact_neigh_update_time",
                  current_contact_neigh_update_time / d_infoN);

    // compute contact forces between particles
    t1 = steady_clock::now();
    d_contact_p->computeForces(*this);
    contact_time = util::methods::timeDiff(t1, steady_clock::now());
    appendKeyData("contact_compute_time", contact_time);
    appendKeyData("avg_contact_force_time", contact_time / d_infoN);
  }

  // Compute external forces
  t1 = steady_clock::now();
  computeExternalForces();
  auto extf_time = util::methods::timeDiff(t1, steady_clock::now());
  appendKeyData("extf_compute_time", extf_time);
  appendKeyData("avg_extf_compute_time", extf_time/d_infoN);

  // output avg time info
  if (dbg_condition) {
    if (d_input_p->isMultiParticle()) {
      log(std::format("    Avg time (ms): \n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n",
                      "tree update", size_t(getKeyData("avg_tree_update_time")),
                      "contact neigh update",
                      size_t(getKeyData("avg_contact_neigh_update_time")),
                      "contact force",
                      size_t(getKeyData("avg_contact_force_time")),
                      "total contact", size_t(getKeyData("avg_tree_update_time")
                                              + getKeyData(
                          "avg_contact_neigh_update_time")
                                              + getKeyData(
                          "avg_contact_force_time")),
                      "peridynamics force",
                      size_t(getKeyData("avg_peridynamics_force_time")),
                      "external force",
                      size_t(getKeyData("avg_extf_compute_time") / d_infoN)),
          2, dbg_condition, 3);

      appendKeyData("avg_tree_update_time", 0.);
      appendKeyData("avg_contact_neigh_update_time", 0.);
      appendKeyData("avg_contact_force_time", 0.);
      appendKeyData("avg_peridynamics_force_time", 0.);
      appendKeyData("avg_extf_compute_time", 0.);
    }
    else {
      log(std::format("    Avg time (ms): \n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n",
                      "peridynamics force", size_t(getKeyData("avg_peridynamics_force_time")),
                      "external force", size_t(getKeyData("avg_extf_compute_time")/d_infoN)),
          2, dbg_condition, 3);

      appendKeyData("avg_peridynamics_force_time", 0.);
      appendKeyData("avg_extf_compute_time", 0.);
    }
  }

  log(std::format("    {:50s} = {:8d} \n",
                  "Force reset time (ms)",
                  size_t(force_reset_time)
      ),
      2, dbg_condition, 3);

  log(std::format("    {:50s} = {:8d} \n",
                  "External force time (ms)",
                  size_t(extf_time)
      ),
      2, dbg_condition, 3);

  log(std::format("    {:50s} = {:8d} \n",
                  "Peridynamics force time (ms)",
                  size_t(pd_time)
      ),
      2, dbg_condition, 3);

  if (d_input_p->isMultiParticle()) {

    log(std::format("    {:50s} = {:8d} \n",
                    "Point cloud update time (ms)",
                    size_t(getKeyData("pt_cloud_update_time"))
        ),
        2, dbg_condition, 3);

    log(std::format("    {:50s} = {:8d} \n",
                    "Contact neighborlist update time (ms)",
                    size_t(current_contact_neigh_update_time)
        ),
        2, dbg_condition, 3);

    log(std::format("    {:50s} = {:8d} \n",
                    "Contact force time (ms)",
                    size_t(contact_time)
        ),
        2, dbg_condition, 3);
  }

}

void PeriDEMModel::computeExternalForces() {
  log("    Computing external force \n", 3);

  auto gravity = d_bcDeck_p->d_gravity;

  if (gravity.length() > 1.0E-8) {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    // Only owned force nodes (walls on rank 0 for particle-MPI; Metis owners
    // for DOF-MPI). Applying gravity on every rank then Allreducing reaction
    // would multiply wall weight by mpiSize.
    const auto &nodes = d_fContCompNodes;
    taskflow.for_each_index((std::size_t) 0, nodes.size(), (std::size_t)1,
                            [this, gravity, &nodes](std::size_t II) {
                              const size_t i = nodes[II];
                              this->d_f[i] += this->getDensity(i) * gravity;
                            });

    executor.run(taskflow).get();
  }

  for (auto &p : d_particlesListTypeAll) {
    if (p->isWall()) {
      if (util::parallel::mpiRank() == 0)
        d_fLoading_p->apply(d_time, p);
    } else if (d_pdDofMpi) {
      // Nodal ownership: apply on every rank; loading loops all nodes of p,
      // but only owned force nodes are integrated.
      d_fLoading_p->apply(d_time, p);
    } else if (particle::isLocallyOwned(*p)) {
      d_fLoading_p->apply(d_time, p);
    }
  }
}

void PeriDEMModel::applyDisplacementBC() {
  log("    Computing external displacement bc \n", 3);
  for (auto &p : d_particlesListTypeAll)
    d_uLoading_p->apply(d_time, p); // applied in parallel
}

void PeriDEMModel::applyInitialCondition() {
  log("Applying initial condition \n", 3);
  for (auto &p : d_particlesListTypeAll)
    loading::applyIC(p, d_bcDeck_p->d_icDeck); // applied in parallel
}

void PeriDEMModel::output() {
  if (currentStep() == 0)
    log(std::format("{}: Output step = {}, time = {:.6f} \n", d_name, d_n, d_time),
        2);
  rw::writeOutput(*this);
}

std::string PeriDEMModel::ppTwoParticleTest() {
  if (!d_postprocess_p)
    return "";
  return d_postprocess_p->twoParticle(*this);
}

void PeriDEMModel::checkStop() {
  if (d_testDeck_p->d_testName == "two_particle")
    log(ppTwoParticleTest(), 2, d_n % d_infoN == 0, 3);
  else if (d_testDeck_p->d_testName == "compressive_test")
    log(ppCompressiveTest(), 2, d_n % d_infoN == 0, 3);
  if (d_postprocess_p)
    d_postprocess_p->checkStop(*this);
}

std::string PeriDEMModel::ppCompressiveTest() {
  if (!d_postprocess_p)
    return "";
  return d_postprocess_p->compressive(*this);
}