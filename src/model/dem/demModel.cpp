/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "demModel.h"

// utils
#include "particle/baseParticle.h"
#include "material/materialUtil.h"
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
#include <random>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>


model::DEMModel::DEMModel(inp::Input *deck, std::string modelName)
  : ModelData(deck),
    d_name(modelName) {

  // initialize logger
  util::io::initLogger(d_outputDeck_p->d_debug,
                       d_outputDeck_p->d_path + "log.txt");
}

void model::DEMModel::log(std::ostringstream &oss, int priority, bool check_condition, int override_priority,
                          bool screen_out) {
  int op = override_priority == -1 ? priority : override_priority;
  //if (d_outputDeck_p->d_debug > priority)
  if ((check_condition and d_outputDeck_p->d_debug > priority) or d_outputDeck_p->d_debug > op)
    util::io::log(oss, screen_out);
}

void model::DEMModel::log(const std::string &str, int priority, bool check_condition, int override_priority,
                          bool screen_out) {
  int op = override_priority == -1 ? priority : override_priority;  
  if ((check_condition and d_outputDeck_p->d_debug > priority) or d_outputDeck_p->d_debug > op)
    util::io::log(str, screen_out);
}

void model::DEMModel::run(inp::Input *deck) {

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

void model::DEMModel::restart(inp::Input *deck) {

  log(d_name + ": Restarting the simulation\n");

  // set time step to step specified in restart deck
  d_n = d_restartDeck_p->d_step;
  d_time = double(d_n) * d_modelDeck_p->d_dt;
  log(fmt::format("  Restart step = {}, time = {:.6f} \n", d_n, d_time));

  // get backup of reference configuration
  std::vector<util::Point> x_ref(d_x.size(), util::Point());
  for (auto &x : d_x)
    x_ref.push_back(x);

  // read displacement and velocity from restart file
  log("  Reading data from restart file = " + d_restartDeck_p->d_file + " \n");
  auto reader = rw::reader::VtkParticleReader(d_restartDeck_p->d_file);
  reader.readNodes(this);
}

void model::DEMModel::close() {
  if (d_ppFile.is_open())
    d_ppFile.close();
}

void model::DEMModel::init() {

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

  // setup contact
  log(d_name + ": Setting up contact.\n");
  setupContact();

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

  log(d_name + ": Creating neighborlist for contact.\n");
  d_contNeighUpdateInterval = d_pDeck_p->d_pNeighDeck.d_neighUpdateInterval;
  d_contNeighSearchRadius = d_pDeck_p->d_pNeighDeck.d_sFactor * d_maxContactR;
  t1 = steady_clock::now();
  updateContactNeighborlist();
  t2 = steady_clock::now();
  appendKeyData("contact_neigh_update_time", util::methods::timeDiff(t1, t2));

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

  // if all dofs of particle is fixed, then mark it so that we do not
  // compute force
  // MAYBE NOT as we may be interested in reaction forces
  //  for (auto &p : d_particlesListTypeAll)
  //    p->checkFixityForForce(); // TODO implement

  // if this is a two-particle test, we set the force calculation off in
  // first particle
  if (d_pDeck_p->d_testName == "two_particle") {
    d_particlesListTypeAll[0]->d_computeForce = false;
  }

  log(fmt::format("{}: Total particles = {}. \n",
                  d_name, d_particlesListTypeAll.size()));

  for (const auto &p : d_particlesListTypeAll)
    if (!p->d_computeForce)
      log(fmt::format("{}: Force OFF in Particle i = {}. \n", d_name, p->getId()));

  log(d_name + ": Creating list of nodes on which force is to be computed.\n");
  // TODO for now we simply look at particle/wall and check if we compute
  //  force on any of its node. Later, one can have control on individual
  //  nodes of particle/wall and remove from d_fCompNodes if no force is to
  //  be computed on them
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

void model::DEMModel::integrate() {

  // perform output at the beginning
  if (d_n == 0 && d_outputDeck_p->d_performOut) {
    log(fmt::format("{}: Output step = {}, time = {:.6f} \n", d_name, d_n, d_time),
        2);
    output();
  }

  // apply initial condition
  if (d_n == 0)
    applyInitialCondition();

  // apply loading
  computeExternalDisplacementBC();
  computeForces();

  for (size_t i = d_n; i < d_modelDeck_p->d_Nt; i++) {

    log(fmt::format("{}: Time step: {}, time: {:8.6f}, steps completed = {}%\n",
                    d_name,
                    i,
                    d_time,
                    float(i) * 100. / d_modelDeck_p->d_Nt),
        2, d_n % d_infoN == 0, 3);
    
    auto t1 = steady_clock::now();
    log("Integrating\n", false, 0, 3);
    integrateStep();
    double integrate_time =
        util::methods::timeDiff(t1, steady_clock::now());

    appendKeyData("integrate_compute_time", integrate_time, true);

    log(fmt::format("  Integration time (ms) = {}\n", integrate_time), 2, d_n % d_infoN == 0, 3);

    if (d_pDeck_p->d_testName == "two_particle") {

      // compute location of maximum shear stress and also compute
      // penetration length
      auto msg = ppTwoParticleTest();
      log(msg, 2, d_n % d_infoN == 0, 3);
    } else if (d_pDeck_p->d_testName == "compressive_test") {
      auto msg = ppCompressiveTest();
      log(msg, 2, d_n % d_infoN == 0, 3);
    }

    // handle general output
    if ((d_n % d_outputDeck_p->d_dtOut == 0) &&
        (d_n >= d_outputDeck_p->d_dtOut) && d_outputDeck_p->d_performOut) {
      output();
    }

    // check for stop
    checkStop();

  } // loop over time steps

  log(fmt::format(
          "{}: Total compute time information (s) \n"
          "  {:22s} = {:8.2f} \n"
          "  {:22s} = {:8.2f} \n"
          "  {:22s} = {:8.2f} \n"
          "  {:22s} = {:8.2f} \n"
          "  {:22s} = {:8.2f} \n",
          d_name,
          "Time integration", getKeyData("integrate_compute_time") * 1.e-6,
          "Peridynamics force", getKeyData("pd_compute_time") * 1.e-6,
          "Contact force", getKeyData("contact_compute_time") * 1.e-6,
          "Search tree update", getKeyData("tree_compute_time") * 1.e-6,
          "External force", getKeyData("extf_compute_time") * 1.e-6)
          );
}

void model::DEMModel::integrateStep() {
  if (d_modelDeck_p->d_timeDiscretization == "central_difference")
    integrateCD();
  else if (d_modelDeck_p->d_timeDiscretization == "velocity_verlet")
    integrateVerlet();
}

void model::DEMModel::integrateCD() {

  // update velocity and displacement
  d_currentDt = d_modelDeck_p->d_dt;
  const auto dim = d_modelDeck_p->d_dim;

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  // update current position, displacement, and velocity of nodes
  taskflow.for_each_index(
    (std::size_t) 0, d_fPdCompNodes.size(), (std::size_t) 1,
      [this, dim](std::size_t II) {
        auto i = this->d_fPdCompNodes[II];

        const auto rho = this->getDensity(i);
        const auto &fix = this->d_fix[i];

        for (int dof = 0; dof < dim; dof++) {
          if (util::methods::isFree(fix, dof)) {
            this->d_v[i][dof] += (this->d_currentDt / rho) * this->d_f[i][dof];
            this->d_u[i][dof] += this->d_currentDt * this->d_v[i][dof];
            this->d_x[i][dof] += this->d_currentDt * this->d_v[i][dof];
          }
        }

        this->d_vMag[i] = this->d_v[i].length();
      } // loop over nodes
  ); // for_each

  executor.run(taskflow).get();

  // advance time
  d_n++;
  d_time += d_currentDt;

  // update displacement bc
  computeExternalDisplacementBC();

  // compute force
  computeForces();
}

void model::DEMModel::integrateVerlet() {

  // update velocity and displacement
  d_currentDt = d_modelDeck_p->d_dt;
  const auto dim = d_modelDeck_p->d_dim;

  // update current position, displacement, and velocity of nodes
  {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, d_fPdCompNodes.size(), (std::size_t) 1,
        [this, dim](std::size_t II) {
          auto i = this->d_fPdCompNodes[II];

          const auto rho = this->getDensity(i);
          const auto &fix = this->d_fix[i];

          for (int dof = 0; dof < dim; dof++) {
            if (util::methods::isFree(fix, dof)) {
              this->d_v[i][dof] += 0.5 * (this->d_currentDt / rho) * this->d_f[i][dof];
              this->d_u[i][dof] += this->d_currentDt * this->d_v[i][dof];
              this->d_x[i][dof] += this->d_currentDt * this->d_v[i][dof];
            }

            this->d_vMag[i] = this->d_v[i].length();
          }
        } // loop over nodes
    ); // for_each

    executor.run(taskflow).get();
  }

  // advance time
  d_n++;
  d_time += d_currentDt;

  // update displacement bc
  computeExternalDisplacementBC();

  // compute force
  computeForces();

  // update velocity of nodes
  {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, d_fPdCompNodes.size(), (std::size_t) 1,
      [this, dim](std::size_t II) {
        auto i = this->d_fPdCompNodes[II];

        const auto rho = this->getDensity(i);
        const auto &fix = this->d_fix[i];
        for (int dof = 0; dof < dim; dof++) {
          if (util::methods::isFree(fix, dof)) {
            this->d_v[i][dof] += 0.5 * (this->d_currentDt / rho) * this->d_f[i][dof];
          }

          this->d_vMag[i] = this->d_v[i].length();
        }
      } // loop over nodes
    ); // for_each

    executor.run(taskflow).get();
  }
}

void model::DEMModel::computeForces() {

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

  // update contact neighborlist
  t1 = steady_clock::now();
  updateContactNeighborlist();
  auto current_contact_neigh_update_time = util::methods::timeDiff(t1, steady_clock::now());
  appendKeyData("contact_neigh_update_time", current_contact_neigh_update_time);
  appendKeyData("avg_contact_neigh_update_time", current_contact_neigh_update_time/d_infoN);

  // compute contact forces between particles
  t1 = steady_clock::now();
  computeContactForces();
  auto contact_time = util::methods::timeDiff(t1, steady_clock::now());
  appendKeyData("contact_compute_time", contact_time);
  appendKeyData("avg_contact_force_time", contact_time/d_infoN);

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
                    "      {:48s} = {:8d}\n"
                    "      {:48s} = {:8d}\n"
                    "      {:48s} = {:8d}\n"
                    "      {:48s} = {:8d}\n"
                    "      {:48s} = {:8d}\n",
                    "tree update", size_t(getKeyData("avg_tree_update_time")),
                    "contact neigh update", size_t(getKeyData("avg_contact_neigh_update_time")),
                    "contact force", size_t(getKeyData("avg_contact_force_time")),
                    "total contact", size_t(getKeyData("avg_tree_update_time")
                          + getKeyData("avg_contact_neigh_update_time")
                          + getKeyData("avg_contact_force_time")),
                    "peridynamics force", size_t(getKeyData("avg_peridynamics_force_time")),
                    "external force", size_t(getKeyData("avg_extf_compute_time")/d_infoN)),
        2, dbg_condition, 3);

    appendKeyData("avg_tree_update_time", 0.);
    appendKeyData("avg_contact_neigh_update_time", 0.);
    appendKeyData("avg_contact_force_time", 0.);
    appendKeyData("avg_peridynamics_force_time", 0.);
    appendKeyData("avg_extf_compute_time", 0.);
  }

  log(fmt::format("    {:50s} = {:8d} \n",
                  "Point cloud update time (ms)",
                  size_t(getKeyData("pt_cloud_update_time"))
      ),
      2, dbg_condition, 3);

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
                  "Contact neighborlist update time (ms)",
                  size_t(current_contact_neigh_update_time)
      ),
      2, dbg_condition, 3);

  log(fmt::format("    {:50s} = {:8d} \n",
                  "Contact force time (ms)",
                  size_t(contact_time)
      ),
      2, dbg_condition, 3);

  log(fmt::format("    {:50s} = {:8d} \n",
                  "External force time (ms)",
                  size_t(extf_time)
      ),
      2, dbg_condition, 3);

}

void model::DEMModel::computePeridynamicForces() {

  log("    Computing peridynamic force \n", 3);

  const auto dim = d_modelDeck_p->d_dim;
  const bool is_state = d_particlesListTypeAll[0]->getMaterial()->isStateActive();

  // compute state-based helper quantities
  if (is_state) {

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, d_fPdCompNodes.size(), (std::size_t) 1, [this](std::size_t II) {
        auto i = this->d_fPdCompNodes[II];

        const auto rho = this->getDensity(i);
        const auto &fix = this->d_fix[i];
        const auto &ptId = this->getPtId(i);
        auto &pi = this->getParticleFromAllList(ptId);

        if (pi->d_material_p->isStateActive()) {

          const double horizon = pi->getHorizon();
          const double mesh_size = pi->getMeshSize();
          const auto &xi = this->d_xRef[i];
          const auto &ui = this->d_u[i];

          // update bond state and compute thetax
          const auto &m = this->d_mX[i];
          double theta = 0.;

          // upper and lower bound for volume correction
          auto check_up = horizon + 0.5 * mesh_size;
          auto check_low = horizon - 0.5 * mesh_size;

          size_t k = 0;
          for (size_t j : this->d_neighPd[i]) {

            const auto &xj = this->d_xRef[j];
            const auto &uj = this->d_u[j];
            double rji = (xj - xi).length();
            // double rji = std::sqrt(this->d_neighPdSqdDist[i][k]);
            double change_length = (xj - xi + uj - ui).length() - rji;

            // step 1: update the bond state
            double s = change_length / rji;
            double sc = pi->d_material_p->getSc(rji);

            // get fracture state, modify, and set
            auto fs = this->d_fracture_p->getBondState(i, k);
            if (!fs && util::isGreater(std::abs(s), sc + 1.0e-10))
              fs = true;
            this->d_fracture_p->setBondState(i, k, fs);

            if (!fs) {

              // get corrected volume of node j
              auto volj = this->d_vol[j];

              if (util::isGreater(rji, check_low))
                volj *= (check_up - rji) / mesh_size;

              theta += rji * change_length * pi->d_material_p->getInfFn(rji) *
                        volj;
            } // if bond is not broken

            k += 1;
          } // loop over neighbors

          this->d_thetaX[i] = 3. * theta / m;
        } // if it is state-based
      } // loop over nodes
    ); // for_each

    executor.run(taskflow).get();
  }

  // compute the internal forces
  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, d_fPdCompNodes.size(), (std::size_t) 1, [this](std::size_t II) {
      auto i = this->d_fPdCompNodes[II];

      // local variable to hold force
      util::Point force_i = util::Point();
      double scalar_f = 0.;

      // for damage
      float Zi = 0.;

      const auto rhoi = this->getDensity(i);
      const auto &ptIdi = this->getPtId(i);
      auto &pi = this->getParticleFromAllList(ptIdi);

      const double horizon = pi->getHorizon();
      const double mesh_size = pi->getMeshSize();
      const auto &xi = this->d_xRef[i];
      const auto &ui = this->d_u[i];
      const auto &mi = this->d_mX[i];
      const auto &thetai = this->d_thetaX[i];

      // upper and lower bound for volume correction
      auto check_up = horizon + 0.5 * mesh_size;
      auto check_low = horizon - 0.5 * mesh_size;

      // loop over neighbors
      {
        size_t k = 0;
        for (size_t j : this->d_neighPd[i]) {
          auto fs = this->d_fracture_p->getBondState(i, k);
          const auto &xj = this->d_xRef[j];
          const auto &uj = this->d_u[j];
          auto volj = this->d_vol[j];
          double rji = (xj - xi).length();
          double Sji = pi->d_material_p->getS(xj - xi, uj - ui);

          if (!fs) {
            const auto &mj = this->d_mX[j];
            const auto &thetaj = this->d_thetaX[j];

            // get corrected volume of node j
            if (util::isGreater(rji, check_low))
              volj *= (check_up - rji) / mesh_size;

            // handle two cases differently
            if (pi->d_material_p->isStateActive()) {

              auto ef_i =
                  pi->d_material_p->getBondEF(rji, Sji, fs, mi, thetai);
              auto ef_j =
                  pi->d_material_p->getBondEF(rji, Sji, fs, mj, thetaj);

              // compute the contribution of bond force to force at i
              scalar_f = (ef_i.second + ef_j.second) * volj;

              force_i += scalar_f * pi->d_material_p->getBondForceDirection(
                                        xj - xi, uj - ui);
            } // if state-based
            else {

              // Debug
              bool break_bonds = true;

              auto ef =
                  pi->d_material_p->getBondEF(rji, Sji, fs, break_bonds);
              this->d_fracture_p->setBondState(i, k, fs);

              // compute the contribution of bond force to force at i
              scalar_f = ef.second * volj;

              force_i += scalar_f * pi->d_material_p->getBondForceDirection(
                                        xj - xi, uj - ui);
            } // if bond-based
          }   // if bond not broken
          else {
            // add normal contact force
            auto yji = xj + uj - (xi + ui);
            auto Rji = yji.length();
            scalar_f = pi->d_Kn * volj * (Rji - pi->d_Rc) / Rji;
            if (scalar_f > 0.)
              scalar_f = 0.;
            force_i += scalar_f * yji;
          } // if bond is broken

          // calculate damage
          auto Sc = pi->d_material_p->getSc(rji);
          if (util::isGreater(std::abs(Sji / Sc), Zi))
            Zi = std::abs(Sji / Sc);

          k++;
        } // loop over neighbors

      } // peridynamic force

      // update force (we remove any force from
      // previous steps and add peridynamics force)
      this->d_f[i] = force_i;

      this->d_Z[i] = Zi;
    }
  ); // for_each

  executor.run(taskflow).get();
}

void model::DEMModel::computeExternalForces() {

  log("    Computing external force \n", 3);

  auto gravity = d_pDeck_p->d_gravity;

  if (gravity.length() > 1.0E-8) {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index((std::size_t) 0, d_x.size(), (std::size_t)1, [this, gravity](std::size_t i) {
          this->d_f[i] += this->getDensity(i) * gravity;
      } // loop over particles
    ); // for_each

    executor.run(taskflow).get();
  }

  //
  for (auto &p : d_particlesListTypeAll)
    d_fLoading_p->apply(d_time, p); // applied in parallel
}

void model::DEMModel::computeExternalDisplacementBC() {
  log("    Computing external displacement bc \n", 3);
  for (auto &p : d_particlesListTypeAll)
    d_uLoading_p->apply(d_time, p); // applied in parallel
}

void model::DEMModel::computeContactForces() {

  log("    Computing normal contact force \n", 3);

  // Description:
  // 1. Normal contact is applied between nodes of particles and walls
  // 2. Normal damping is applied between particle centers
  // 3. Normal damping is applied between nodes of particle and wall pairs

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0,
                          d_fContCompNodes.size(),
                          (std::size_t) 1,
                          [this](std::size_t II) {

                              auto i = this->d_fContCompNodes[II];

                              // local variable to hold force
                              util::Point force_i = util::Point();
                              double scalar_f = 0.;

                              const auto &ptIdi = this->getPtId(i);
                              auto &pi = this->getParticleFromAllList(ptIdi);
                              double horizon = pi->d_material_p->getHorizon();
                              double search_r = this->d_maxContactR;

                              // particle data
                              double rhoi = pi->getDensity();

                              const auto &yi = this->d_x[i]; // current coordinates
                              const auto &ui = this->d_u[i];
                              const auto &vi = this->d_v[i];
                              const auto &voli = this->d_vol[i];

                              const std::vector<size_t> &neighs = this->d_neighC[i];

                              if (neighs.size() > 0) {

                                for (const auto &j_id: neighs) {

                                  //auto &j_id = neighs[j];
                                  const auto &yj = this->d_x[j_id]; // current coordinates
                                  double Rji = (yj - yi).length();
                                  auto &ptIdj = this->d_ptId[j_id];
                                  auto &pj = this->getParticleFromAllList(ptIdj);
                                  double rhoj = pj->getDensity();

                                  bool both_walls =
                                          (pi->getTypeIndex() == 1 and pj->getTypeIndex() == 1);

                                  if (j_id != i) {
                                    if (ptIdj != ptIdi && !both_walls) {

                                      // apply particle-particle or particle-wall contact here
                                      const auto &contact =
                                              d_cDeck_p->getContact(pi->d_zoneId, pj->d_zoneId);

                                      if (util::isLess(Rji, contact.d_contactR)) {

                                        auto yji = this->d_x[j_id] - yi;
                                        auto volj = this->d_vol[j_id];
                                        auto vji = this->d_v[j_id] - vi;

                                        // resolve velocity vector in normal and tangential components
                                        auto en = yji / Rji;
                                        auto vn_mag = (vji * en);
                                        auto et = vji - vn_mag * en;
                                        if (util::isGreater(et.length(), 0.))
                                          et = et / et.length();
                                        else
                                          et = util::Point();

                                        // Formula using bulk modulus and horizon
                                        scalar_f = contact.d_Kn * (Rji - contact.d_contactR) *
                                                   volj; // divided by voli
                                        if (scalar_f > 0.)
                                          scalar_f = 0.;
                                        force_i += scalar_f * en;

                                        // compute friction force (since f < 0, |f| = -f)
                                        force_i += contact.d_mu * scalar_f * et;

                                        // if particle-wall pair, apply damping contact here <--
                                        // doesnt seem to work
                                        bool node_lvl_damp = false;
                                        // if (pi->getTypeIndex() == 0 and pj->getTypeIndex() == 1)
                                        //   node_lvl_damp = true;

                                        if (node_lvl_damp) {
                                          // apply damping at the node level
                                          auto meq = util::equivalentMass(rhoi * voli, rhoj * volj);
                                          auto beta_n =
                                                  contact.d_betan *
                                                  std::sqrt(contact.d_kappa * contact.d_contactR * meq);

                                          auto &pii = this->d_particlesListTypeAll[pi->getId()];
                                          vji = this->d_v[j_id] - pii->getVCenter();
                                          vn_mag = (vji * en);
                                          if (vn_mag > 0.)
                                            vn_mag = 0.;
                                          force_i += beta_n * vn_mag * en / voli;
                                        }
                                      } // within contact radius
                                    }   // particle-particle contact
                                  }     // if j_id is not i
                                }       // loop over neighbors
                              }         // contact neighbor

                              this->d_f[i] += force_i;
                          }
  ); // for_each

  executor.run(taskflow).get();


  // damping force
  log("    Computing normal damping force \n", 3);
  for (auto &pi : d_particlesListTypeParticle) {

    auto pi_id = pi->getId();

    double Ri = pi->d_geom_p->boundingRadius();
    double vol_pi = M_PI * Ri * Ri;
    auto pi_xc = pi->getXCenter();
    auto pi_vc = pi->getVCenter();
    auto rhoi = pi->getDensity();
    util::Point force_i = util::Point();

    // particle-particle
    for (auto &pj : this->d_particlesListTypeParticle) {
      if (pj->getId() != pi->getId()) {
        auto Rj = pj->d_geom_p->boundingRadius();
        auto xc_ji = pj->getXCenter() - pi_xc;
        auto dist_xcji = xc_ji.length();

        const auto &contact = d_cDeck_p->getContact(pi->d_zoneId, pj->d_zoneId);

        if (util::isLess(dist_xcji, Rj + Ri + 1.01 * contact.d_contactR)) {

          auto vol_pj = M_PI * Rj * Rj;
          auto rhoj = pj->getDensity();
          // equivalent mass
          auto meq = util::equivalentMass(rhoi * vol_pi, rhoj * vol_pj);

          // beta_n
          auto beta_n = contact.d_betan *
                        std::sqrt(contact.d_kappa * contact.d_contactR * meq);

          // center-center vector
          auto hat_xc_ji = util::Point();
          if (util::isGreater(dist_xcji, 0.))
            hat_xc_ji = xc_ji / dist_xcji;
          else
            hat_xc_ji = util::Point();

          // center-center velocity
          auto vc_ji = pj->getVCenter() - pi_vc;
          auto vc_mag = vc_ji * hat_xc_ji;
          if (vc_mag > 0.)
            vc_mag = 0.;

          // force at node of pi
          force_i += beta_n * vc_mag * hat_xc_ji / vol_pi;
        } // if within contact distance
      }   // if not same particles
    }     // other particles

    // particle-wall
    // Step 1: Create list of wall nodes that are within the Rc distance
    // of at least one of the particle
    // This is done already in updateContactNeighborList()

    // step 2 - condensed wall nodes into one vector (has to be done serially
    d_neighWallNodesCondensed[pi->getId()].clear();
    {
      for (size_t j=0; j<d_neighWallNodes[pi_id].size(); j++) {

        const auto &j_id = pi->getNodeId(j);
        const auto &yj = this->d_x[j_id];

        for (size_t k=0; k<d_neighWallNodes[pi_id][j].size(); k++) {

          const auto &k_id = d_neighWallNodes[pi_id][j][k];
          const auto &pk = d_particlesListTypeAll[d_ptId[k_id]];

          double Rjk = (this->d_x[k_id] - yj).length();

          const auto &contact =
                  d_cDeck_p->getContact(pi->d_zoneId, pk->d_zoneId);

          if (util::isLess(Rjk, contact.d_contactR))
            util::methods::addToList(k_id, d_neighWallNodesCondensed[pi_id]);

        } // loop over k
      } // loop over j
    } // step 2

    // now loop over wall nodes and add force to center of particle
    for (auto &j : d_neighWallNodesCondensed[pi_id]) {

      auto &ptIdj = this->d_ptId[j];
      auto &pj = this->d_particlesListTypeAll[ptIdj];
      auto rhoj = pj->getDensity();
      auto volj = this->d_vol[j];
      auto meq = rhoi * vol_pi;
      //auto meq = util::equivalentMass(rhoi * vol_pi, rhoj * volj);

      const auto &contact
              = d_cDeck_p->getContact(pi->d_zoneId, pj->d_zoneId);

      // beta_n
      auto beta_n = contact.d_betan *
                    std::sqrt(contact.d_kappa * contact.d_contactR * meq);

      // center-node vector
      auto xc_ji = this->d_x[j] - pi_xc;
      auto hat_xc_ji = util::Point();
      if (util::isGreater(xc_ji.length(), 0.))
        hat_xc_ji = xc_ji / xc_ji.length();

      // center-node velocity
      auto vc_ji = this->d_v[j] - pi_vc;
      auto vc_mag = vc_ji * hat_xc_ji;
      if (vc_mag > 0.)
        vc_mag = 0.;

      // force at node of pi
      force_i += beta_n * vc_mag * hat_xc_ji / vol_pi;
    }

    // distribute force_i to all nodes of particle pi
    {
      tf::Executor executor(util::parallel::getNThreads());
      tf::Taskflow taskflow;

      taskflow.for_each_index((std::size_t) 0, pi->getNumNodes(), (std::size_t) 1,
                              [this, pi, force_i](std::size_t i) {
                                  this->d_f[pi->getNodeId(i)] += force_i;
                              }
      ); // for_each

      executor.run(taskflow).get();
    }
  } // loop over particle for damping
}

void model::DEMModel::applyInitialCondition() {

  log("Applying initial condition \n", 3);

  if (!d_pDeck_p->d_icDeck.d_icActive)
    return;

  const auto ic_v = d_pDeck_p->d_icDeck.d_icVec;
  const auto ic_p_list = d_pDeck_p->d_icDeck.d_pList;

  // add specified velocity to particle
  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0,
                          ic_p_list.size(),
                          (std::size_t) 1,
                          [this, ic_v, ic_p_list](std::size_t i) {
      auto &p = this->d_particlesListTypeAll[ic_p_list[i]];

      // velocity
      for (size_t j = 0; j < p->getNumNodes(); j++)
        p->setVLocal(j, ic_v);
    } // loop over particles
  ); // for_each

  executor.run(taskflow).get();
}

void model::DEMModel::createParticles() {

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

    // get representative particle for this zone
    auto &rep_geom_p = pz.d_geom_p;
    auto rep_geom_params = pz.d_geomParams;

    // read mesh data
    log(d_name + ": Creating mesh for reference particle in zone = " +
                  std::to_string(z_id) + "\n");
    auto mesh = std::make_shared<fe::Mesh>(&pz.d_meshDeck);

    // create the reference particle
    log(d_name + ": Creating reference particle in zone = " +
                  std::to_string(z_id) + "\n");

    auto ref_p = std::make_shared<particle::RefParticle>(
            d_referenceParticles.size(),
            static_cast<std::shared_ptr<ModelData>>(this),
            rep_geom_p,
            mesh);

    d_referenceParticles.emplace_back(ref_p);

    // check the particle generation method
    log(d_name + ": Creating particles in zone = " +
                  std::to_string(z_id) + "\n");

    if (pz.d_genMethod == "From_File") {
      createParticlesFromFile(z, ref_p);
    }
    else {
      if (pz.d_createParticleUsingParticleZoneGeomObject) {
        createParticleUsingParticleZoneGeomObject(z, ref_p);
      }
      else {
        std::cerr << "Error: Particle generation method = " << pz.d_genMethod <<
                  " not recognized.\n";
        exit(1);
      }
    }

    // get new size of data
    auto psize_new = d_particlesListTypeAll.size();

    // store this in zone-info
    d_zInfo.emplace_back(std::vector<size_t>{psize, psize_new, z_id});
  }
}

void model::DEMModel::createParticleUsingParticleZoneGeomObject(
        size_t z,
        std::shared_ptr<particle::RefParticle> ref_p) {

  log(d_name + ": Creating particle using Particle Zone Geometry Object\n", 1);

  // get particle zone
  auto &pz = d_pDeck_p->d_particleZones[z];

  // get zone id
  auto z_id = pz.d_zone.d_zoneId;

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

}

void model::DEMModel::createParticlesFromFile(
    size_t z, std::shared_ptr<particle::RefParticle> ref_p) {

  log(d_name + ": Creating particle from file\n", 1);

  // get particle zone
  auto &pz = d_pDeck_p->d_particleZones[z];

  // get zone id
  auto z_id = pz.d_zone.d_zoneId;

  // read file which contains location of centers of particle, zone id, and
  // radius of particle
  std::vector<util::Point> centers;
  std::vector<double> rads;
  std::vector<double> orients;
  if (pz.d_particleFileDataType == "loc_rad") {
    rw::reader::readParticleCsvFile(pz.d_particleFile, d_modelDeck_p->d_dim,
                                    &centers, &rads, z_id);

    util::DistributionSample<UniformDistribution> uniform_dist(
        0., 1., d_modelDeck_p->d_seed);

    if (d_pDeck_p->d_testName == "two_particle") {
      for (size_t i = 0; i < rads.size(); i++)
        orients.push_back((double(i)) * M_PI);
    } else {
      for (size_t i = 0; i < rads.size(); i++)
        orients.push_back(
            util::transform_to_uniform_dist(0., 2. * M_PI, uniform_dist()));
    }
  }
  else if (pz.d_particleFileDataType == "loc_rad_orient") {
    rw::reader::readParticleWithOrientCsvFile(pz.d_particleFile,
                                              d_modelDeck_p->d_dim, &centers,
                                              &rads, &orients, z_id);
  }

  log(fmt::format("zone_id: {}, rads: {}, orients: {}, centers: {} \n", z_id,
                    util::io::printStr(rads), util::io::printStr(orients),
                    util::io::printStr(centers)), 2);

  // get representative particle for this zone
  const auto &rep_geom_p = pz.d_geom_p;
  auto rep_geom_params = pz.d_geomParams;

  // get zone bounding box
  std::pair<util::Point, util::Point> box = rep_geom_p->box();

  size_t p_counter = 0;
  size_t p_old_size = d_particlesListTypeAll.size();
  for (const auto &site : centers) {

    double particle_radius = rads[p_counter];
    double particle_orient = orients[p_counter];

    // create geometrical object
    std::shared_ptr<util::geometry::GeomObject> p_geom;
    createGeometryAtSite(particle_radius,
                         particle_orient,
                         site,
                         rep_geom_params,
                         rep_geom_p,
                         p_geom);

    // create transform
    auto p_transform = particle::ParticleTransform(
            site, util::Point(0., 0., 1.), particle_orient,
            particle_radius / ref_p->getParticleRadius());

    if (p_transform.d_scale < 1.E-8) {
      std::cerr << "Error: check scale in transform. "
                << " Scale: " << particle_radius / ref_p->getParticleRadius()
                << " p rad: " << particle_radius
                << " ref p rad: " << ref_p->getParticleRadius()
                << p_transform.printStr();
      exit(1);
    }

    // finally create dem particle at this site
    //auto particle_id = p_counter + p_old_size;
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
            p_geom,
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
    p_counter++;
  }
}

void model::DEMModel::createGeometryAtSite(const double &particle_radius,
                                           const double &particle_orient,
                                           const util::Point &site,
                                           const std::vector<double> &rep_geom_params,
                                           const std::shared_ptr<util::geometry::GeomObject> &rep_geom_p,
                                           std::shared_ptr<util::geometry::GeomObject> &p_geom) {
  std::vector<double> params;
  for (auto x : rep_geom_params)
    params.push_back(x);

  if (util::methods::isTagInList(rep_geom_p->d_name,
                                 util::geometry::acceptable_geometries)) {

    if (util::methods::isTagInList(rep_geom_p->d_name,
                                   {"circle", "sphere", "hexagon",
                                    "triangle", "square", "cube"})) {

      // case - objects requiring four parameters
      // here 'triangle' is a uniform triangle (see constructor of Triangle)
      // 'hexagon' is a hexagon with axis (1, 0, 0)
      size_t num_params = 4;

      if (params.size() < num_params)
        params.resize(num_params);
      params[0] = particle_radius;
      for (int dof = 0; dof < 3; dof++)
        params[dof + 1] = site[dof];
    }
    else if (rep_geom_p->d_name == "drum2d") {

      // case - objects requiring five parameters
      size_t num_params = 5;

      if (params.size() < num_params)
        params.resize(num_params);

      params[0] = particle_radius; // biggger length along x-direction
      params[1] = particle_radius * rep_geom_params[1] / rep_geom_params[0]; // neck length along x-direction
      for (int dof = 0; dof < 3; dof++)
        params[dof + 2] = site[dof];
    }
    else if (rep_geom_p->d_name == "rectangle") {

      // case - objects requiring five parameters
      size_t num_params = 5;

      if (params.size() < num_params)
        params.resize(num_params);

      params[0] = particle_radius; // length along x-direction
      params[1] = particle_radius * rep_geom_params[1] / rep_geom_params[0]; // length along y-direction
      for (int dof = 0; dof < 3; dof++)
        params[dof + 2] = site[dof];
    }
    else if (rep_geom_p->d_name == "cuboid") {

      // case - objects requiring six parameters

      if (params.size() < 6)
        params.resize(6);

      params[0] = particle_radius; // length is x-direction
      params[1] = particle_radius * rep_geom_params[1] / rep_geom_params[0]; // length in y-direction
      params[2] = particle_radius * rep_geom_params[2] / rep_geom_params[0]; // length in z-direction
      for (int dof = 0; dof < 3; dof++)
        params[dof + 2] = site[dof];
    }
  } else {
    std::cerr << fmt::format("Error: PeriDEM supports following type "
                             "of geometries for particles = {}\n",
                             util::io::printStr(util::geometry::acceptable_geometries));
    exit(EXIT_FAILURE);
  }

  // create geometry now
  std::vector<std::string> vec_geom_type;
  std::vector<std::string> vec_geom_flag;
  util::geometry::createGeomObject(rep_geom_p->d_name, params, vec_geom_type,
                                   vec_geom_flag, p_geom,
                                   d_modelDeck_p->d_dim, false);
}

void model::DEMModel::setupContact() {

  // loop over all particle zones and get minimum value of mesh size
  size_t c = 0;
  for (const auto *p : d_particlesListTypeAll) {

    auto h = p->getMeshSize();
    if (c == 0) {
      d_hMin = h;
      d_hMax = h;
      c++;
    }

    if (util::isGreater(d_hMin, h))
      d_hMin = h;
    if (util::isGreater(h, d_hMax))
      d_hMax = h;
  }

  log(fmt::format("{}: Contact setup\n  hmin = {:.6f}, hmax = {:.6f} \n",
                  d_name, d_hMin, d_hMax), 1);

  d_maxContactR = 0.;
  // precompute bulk modulus of all zones
  std::vector<double> bulk_modulus;
  // NOTE - d_data.size() and d_zoneVec.size() are equal
  for (size_t i = 0; i < d_cDeck_p->d_data.size(); i++) {

    double kappa_i = d_pDeck_p->d_particleZones[i].d_matDeck.d_matData.d_K;

    if (kappa_i < 0.) {
      std::cerr << "Error: We need bulk modulus provided in input file.\n";
      std::cerr << d_pDeck_p->d_particleZones[i].printStr();
      exit(1);
    }

    bulk_modulus.push_back(kappa_i);
  }

  for (size_t i = 0; i < d_cDeck_p->d_data.size(); i++) {
    for (size_t j = 0; j < d_cDeck_p->d_data.size(); j++) {

      inp::ContactPairDeck *deck = &(d_cDeck_p->d_data[i][j]);

      if (deck->d_computeContactR)
        deck->d_contactR *= d_hMin;

      if (d_maxContactR < deck->d_contactR)
        d_maxContactR = deck->d_contactR;

      // get effective bulk modulus for pair of zones and store it
      deck->d_kappa = util::equivalentMass(bulk_modulus[i], bulk_modulus[j]);

      // Kn
      deck->d_Kn *= deck->d_KnFactor;

      // Beta n
      double log_e = std::log(deck->d_eps);
      deck->d_betan =
          deck->d_betanFactor *
          (-2. * log_e * std::sqrt(1. / (M_PI * M_PI + log_e * log_e)));

      log(fmt::format("  contact_radius = {:.6f}, hmin = {:.6f}, Kn = {:5.3e}, "
                      "Vmax = {:5.3e}, "
                      "betan = {:7.5f}, mu = {:.4f}, kappa = {:5.3e}\n",
                      deck->d_contactR, d_hMin, deck->d_Kn, deck->d_vMax,
                      deck->d_betan, deck->d_mu, deck->d_kappa), 2);
    }
  }
}

void model::DEMModel::setupQuadratureData() {

  if (util::methods::isTagInList("Strain_Stress", d_outputDeck_p->d_outTags)
      or d_modelDeck_p->d_populateElementNodeConnectivity) {

    // read element-node connectivity data if not done
    for (auto &p: d_referenceParticles) {
      auto &particle_mesh_p = p->getMeshP();
      if (!particle_mesh_p->d_encDataPopulated && particle_mesh_p->d_enc.empty()) {
        particle_mesh_p->readElementData(particle_mesh_p->d_filename);
      }
    }

    // setup quadrature point and strain/stress data
    // we need to know size of the data
    size_t totalQuadPoints = 0;
    for (auto &p: d_particlesListTypeAll) {
      const auto &particle_mesh_p = p->getMeshP();

      // get Quadrature
      fe::BaseElem *elem;
      if (particle_mesh_p->getElementType() == util::vtk_type_line)
        elem = new fe::LineElem(d_modelDeck_p->d_quadOrder);
      else if (particle_mesh_p->getElementType() == util::vtk_type_triangle)
        elem = new fe::TriElem(d_modelDeck_p->d_quadOrder);
      else if (particle_mesh_p->getElementType() == util::vtk_type_quad)
        elem = new fe::QuadElem(d_modelDeck_p->d_quadOrder);
      else if (particle_mesh_p->getElementType() == util::vtk_type_tetra)
        elem = new fe::TetElem(d_modelDeck_p->d_quadOrder);
      else {
        std::cerr << fmt::format("Error: Can not compute strain/stress as the element "
                                 "type = {} is not yet supported in this routine.\n", particle_mesh_p->getElementType());
        exit(EXIT_FAILURE);
      }

      p->d_globQuadStart = totalQuadPoints;
      totalQuadPoints += particle_mesh_p->getNumElements() *
                         elem->getNumQuadPoints();
      p->d_globQuadEnd = totalQuadPoints;

      std::cout << fmt::format("p->id() = {}, "
                               "p->d_globQuadStart = {}, "
                               "totalQuadPoints = {}, "
                               "p->d_globQuadEnd = {}",
                               p->getId(), p->d_globQuadStart,
                               particle_mesh_p->getNumElements() *
                               elem->getNumQuadPoints(), p->d_globQuadEnd)
                << std::endl;
    }

    // resize data
    d_xQuadCur.resize(totalQuadPoints);
    d_strain.resize(totalQuadPoints);
    d_stress.resize(totalQuadPoints);
  } // setting up quadrature data
}

void model::DEMModel::updatePeridynamicNeighborlist() {

  d_neighPd.resize(d_x.size());
  // d_neighPdSqdDist.resize(d_x.size());
  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, d_x.size(), (std::size_t) 1, [this](std::size_t i) {
      const auto &pi = this->d_ptId[i];
      double search_r = this->d_particlesListTypeAll[pi]->d_material_p->getHorizon();

      std::vector<size_t> neighs;
      std::vector<double> sqr_dist;
      if (this->d_nsearch_p->radiusSearchIncludeTag(this->d_x[i],
                                                    search_r,
                                                    neighs,
                                                    sqr_dist,
                                                    this->d_ptId[i],
                                                    this->d_ptId) > 0) {
        for (std::size_t j = 0; j < neighs.size(); ++j)
          if (neighs[j] != i && this->d_ptId[neighs[j]] == pi) {
            this->d_neighPd[i].push_back(size_t(neighs[j]));
            // this->d_neighPdSqdDist[i].push_back(sqr_dist[j]);
          }
      }
    }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  log(fmt::format("{}: Peridynamics neighbor update time = {}\n",
                  d_name, util::methods::timeDiff(t1, t2)), 2);
}

void model::DEMModel::updateContactNeighborlist() {

  auto update = updateContactNeighborSearchParameters();

  if (!update)
    return;

  // update contact neighborlist

  // update the point cloud (make sure that d_x is updated along with displacement)
  auto pt_cloud_update_time = d_nsearch_p->setInputCloud();
  setKeyData("pt_cloud_update_time", pt_cloud_update_time);
  appendKeyData("tree_compute_time", pt_cloud_update_time);
  appendKeyData("avg_tree_update_time", pt_cloud_update_time/d_infoN);

  if (d_neighC.size() != d_x.size())
    d_neighC.resize(d_x.size());

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, d_x.size(), (std::size_t) 1,
                          [this](std::size_t i) {

    const auto &pi = this->d_ptId[i];
    const auto &pi_particle = this->d_particlesListTypeAll[pi];

    // search?
    bool perform_search_based_on_particle = true;
    if (pi_particle->d_typeIndex == 1) // wall
      perform_search_based_on_particle = false;

    if (pi_particle->d_allDofsConstrained or !pi_particle->d_computeForce)
      perform_search_based_on_particle = false;

    if (perform_search_based_on_particle) {

      std::vector<size_t> neighs;
      std::vector<double> sqr_dist;

      this->d_neighC[i].clear();

      auto n = this->d_nsearch_p->radiusSearchExcludeTag(
              this->d_x[i],
              this->d_contNeighSearchRadius,
              neighs,
              sqr_dist,
              this->d_ptId[i],
              this->d_ptId);

      if (n > 0) {
        for (auto neigh: neighs) {
          if (neigh != i)
            this->d_neighC[i].push_back(neigh);
        }
      }
    }
}
  ); // for_each

  executor.run(taskflow).get();


  // handle particle-wall neighborlist (based on the d_neighC that we already computed)
  d_neighWallNodes.resize(d_particlesListTypeAll.size());
  d_neighWallNodesDistance.resize(d_particlesListTypeAll.size());
  d_neighWallNodesCondensed.resize(d_particlesListTypeAll.size());

  for (auto &pi : d_particlesListTypeParticle) {

    d_neighWallNodes[pi->getId()].resize(pi->getNumNodes());
    d_neighWallNodesDistance[pi->getId()].resize(pi->getNumNodes());

    // get all wall nodes that are within contact distance to the nodes of this particle
    {
      tf::Executor executor(util::parallel::getNThreads());
      tf::Taskflow taskflow;

      taskflow.for_each_index((std::size_t) 0,
                              pi->getNumNodes(),
                              (std::size_t) 1,
                              [this, &pi](std::size_t i) {

            auto i_glob = pi->getNodeId(i);
            auto yi = this->d_x[i_glob];

            const std::vector<size_t> &neighs = this->d_neighC[i_glob];

            this->d_neighWallNodes[pi->getId()][i].clear();
            this->d_neighWallNodesDistance[pi->getId()][i].clear();

            for (const auto &j_id: neighs) {

              auto &ptIdj = this->d_ptId[j_id];
              auto &pj = this->getParticleFromAllList(
                      ptIdj);

              // we are only interested in nodes from wall
              if (pj->getTypeIndex() == 1) {
                  this->d_neighWallNodes[pi->getId()][i].push_back(j_id);
                  //this->d_neighWallNodesDistance[pi->getId()][i].push_back(Rji);
              }
            }
        }
      ); // for_each

      executor.run(taskflow).get();
    }
  } // loop over particles

}

bool model::DEMModel::updateContactNeighborSearchParameters() {

  // initialize parameters
  if (d_contNeighUpdateInterval == 0 and
      util::isLess(d_contNeighSearchRadius, 1.e-16)) {
    d_contNeighUpdateInterval = d_pDeck_p->d_pNeighDeck.d_neighUpdateInterval;
    d_contNeighTimestepCounter = d_n % d_contNeighUpdateInterval;
    d_contNeighSearchRadius = d_maxContactR * d_pDeck_p->d_pNeighDeck.d_sFactor;
  }

  // at d_n = 0, this function will be called twice because updateContactNeighborlist() will be
  // called twice: one inside init() and second inside computeForces()
  // so to match d_n and d_contNeighTimestepCounter in the initial stage of simulation, we need to handle the special case
  if (d_n == 0) {
    appendKeyData("update_contact_neigh_search_params_init_call_count", 1);

    if (int(getKeyData("update_contact_neigh_search_params_init_call_count")) == 1)
      return true;

    if (int(getKeyData("update_contact_neigh_search_params_init_call_count")) == 2) {
      d_contNeighTimestepCounter++;
      return (d_contNeighTimestepCounter - 1) % d_contNeighUpdateInterval == 0;
    }
  }

  // handle case of restart
  if (d_modelDeck_p->d_isRestartActive and d_n == d_restartDeck_p->d_step) {
    // assign correct value for restart step
    d_contNeighTimestepCounter = d_n % d_contNeighUpdateInterval;
  }

  if (d_contNeighUpdateInterval == 1) {
    // further optimization of parameters is not possible
    d_contNeighSearchRadius = d_maxContactR;

    // update counter and return condition for contact search
    d_contNeighTimestepCounter++;
    return (d_contNeighTimestepCounter - 1) % d_contNeighUpdateInterval == 0;
  }

  // check if we should proceed with parameter update
  // param update is done at smaller interval than the search itself to avoid
  // scenarios where particles suddenly move with a high velocity
  size_t update_param_interval =
          d_contNeighUpdateInterval > 5 ? size_t(
                  0.2 * d_contNeighUpdateInterval) : 1;

  // check if we ought to update search parameters; if not, return
  if (d_contNeighTimestepCounter > 0 and d_contNeighTimestepCounter % update_param_interval != 0) {
    // update counter and return condition for contact search
    d_contNeighTimestepCounter++;
    return (d_contNeighTimestepCounter - 1) % d_contNeighUpdateInterval == 0;
  }

  // first update the maximum velocity in all particles
  for (auto &pi : d_particlesListTypeAll) {
    auto max_v_node = util::methods::maxIndex(d_vMag,
                                              pi->d_globStart, pi->d_globEnd);

    if (max_v_node > pi->d_globEnd or max_v_node < pi->d_globStart) {
      std::cerr << fmt::format("Error: max_v_node = {} for "
                               "particle of id = {} is not in the limit.\n",
                               max_v_node, pi->getId())
                << "Particle info = \n"
                << pi->printStr()
                << "\n\n Magnitude of velocity = "
                << d_vMag[max_v_node] << "\n";
      exit(EXIT_FAILURE);
    }

    d_maxVelocityParticlesListTypeAll[pi->getId()]
            = d_vMag[max_v_node];
  }

  // find max velocity among all particles
  d_maxVelocity = util::methods::max(d_maxVelocityParticlesListTypeAll);

  // now we find the best parameters for contact search
  auto up_interval_old = d_contNeighUpdateInterval;

  // TO ensure that in d_neighUpdateInterval time steps, the search radius is above the
  // distance traveled by object with velocity d_maxVelocity
  // also multiply by a safety factor
  double safety_factor = d_pDeck_p->d_pNeighDeck.d_sFactor > 5 ? d_pDeck_p->d_pNeighDeck.d_sFactor : 10;
  auto max_search_r_from_contact_R = d_pDeck_p->d_pNeighDeck.d_sFactor * d_maxContactR;
  auto max_search_r = d_maxVelocity * d_currentDt
                      * d_pDeck_p->d_pNeighDeck.d_neighUpdateInterval
                      * safety_factor;


  if (util::isGreater(max_search_r, max_search_r_from_contact_R )) {

    d_contNeighUpdateInterval = size_t(d_maxContactR/(d_maxVelocity * d_currentDt));
    if (up_interval_old > d_contNeighUpdateInterval) {
      // issue warning
      log(fmt::format("Warning: Contact search radius based on velocity is greater than "
                      "the max contact radius.\n"
                      "Warning: Adjusting contact neighborlist update interval.\n"
                      "{:>13} = {:4.6e}, time step = {}, "
                      "velocity-based r = {:4.6e}, max contact r = {:4.6e}\n",
                      "Time", d_time, d_n, max_search_r, max_search_r_from_contact_R),
          2, d_n % d_infoN == 0, 3);
    }

    d_contNeighSearchRadius = max_search_r_from_contact_R;
    // reset time step counter for contact so that the contact list is updated in the current time step
    // and the update cycle starts from the current time step
    d_contNeighTimestepCounter = 0;

    if (d_contNeighUpdateInterval < 1) {
      d_contNeighUpdateInterval = 1;
      d_contNeighSearchRadius = d_maxContactR;
    }
  }
  else {
    // update search radius
    d_contNeighSearchRadius = d_contNeighUpdateInterval < 2 ? d_maxContactR : max_search_r_from_contact_R;
  }

  if (up_interval_old > d_contNeighUpdateInterval) {
    log(fmt::format("    Contact neighbor parameters: \n"
                    "      {:48s} = {:d}\n"
                    "      {:48s} = {:d}\n"
                    "      {:48s} = {:d}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n",
                    "time step", d_n,
                    "contact neighbor update interval",
                    d_contNeighUpdateInterval,
                    "contact neighbor update time step counter",
                    d_contNeighTimestepCounter,
                    "search radius", d_contNeighSearchRadius,
                    "max contact radius", d_maxContactR,
                    "search radius factor", d_pDeck_p->d_pNeighDeck.d_sFactor,
                    "max search r from velocity", max_search_r,
                    "max search r from contact r", max_search_r_from_contact_R,
                    "max velocity", d_maxVelocity),
        2, d_n % d_infoN == 0, 3);
  }

  // update counter and return condition for contact search
  d_contNeighTimestepCounter++;
  return (d_contNeighTimestepCounter - 1) % d_contNeighUpdateInterval == 0;
}

void model::DEMModel::updateNeighborlistCombine() {
  // Not used
  return;
}

void model::DEMModel::output() {

  // write out % completion of simulation at 10% interval
  {
    float p = float(d_n) * 100. / d_modelDeck_p->d_Nt;
    int m = std::max(1, int(d_modelDeck_p->d_Nt / 10));
    if (d_n % m == 0 && int(p) > 0)
      log(fmt::format("{}: Simulation {}% complete\n",
                      d_name, int(p)));
    ;
  }

  log(fmt::format("{}: Output step = {}, time = {:.6f} \n",
                  d_name, d_n, d_time),
      2);

  if (d_outputDeck_p->d_debug > 0 and getKeyData("debug_once") < 0) {

    setKeyData("debug_once", 1);

    size_t nt = 1;
    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "*******************************************\n";
    oss << tabS << "Debug various input decks\n\n\n";
    oss << d_modelDeck_p->printStr(nt + 1);
    oss << d_pDeck_p->printStr(nt + 1);
    oss << d_cDeck_p->printStr(nt + 1);
    oss << tabS << "\n\n*******************************************\n";
    oss << tabS << "Debug particle data\n\n\n";
    oss << tabS << "Number of particles = " << d_particlesListTypeAll.size() << std::endl;
    oss << tabS << "Number of particle zones = " << d_zInfo.size() << std::endl;
    for (auto zone : d_zInfo) {
      oss << tabS << "zone of d_zInfo: " << util::io::printStr(zone)
          << std::endl;
    }

    // wall info
    oss << tabS << "Number of walls = " << d_particlesListTypeWall.size() << std::endl;
    for (auto &d_wall : d_particlesListTypeWall)
      oss << tabS << "Number of nodes in wall in zone " << d_wall->d_zoneId
          << " is " << d_wall->getNumNodes() << std::endl;

    oss << tabS << "h_min = " << d_hMin << ", h_max = " << d_hMax << std::endl;

    log(oss, 2);
  } // end of debug

  size_t dt_out = d_outputDeck_p->d_dtOutCriteria;
  auto writer = rw::writer::VtkParticleWriter(
      d_outputDeck_p->d_path + "output_" + std::to_string(d_n / dt_out));
  if (d_outputDeck_p->d_performFEOut)
    writer.appendMesh(this, d_outputDeck_p->d_outTags);
  else
    writer.appendNodes(this, d_outputDeck_p->d_outTags);

  writer.addTimeStep(d_time);
  writer.close();

  if (util::methods::isTagInList("Strain_Stress", d_outputDeck_p->d_outTags)) {

    // compute current position of quadrature points and strain/stress data
    {
      // if particle mat data is not computed, compute them
      if (d_particlesMatDataList.empty()) {
        for (auto &p: d_particlesListTypeAll) {
          d_particlesMatDataList.push_back(p->getMaterial()->computeMaterialProperties(
                  p->getMeshP()->getDimension()));
        }
      }

      for (auto &p: d_particlesListTypeAll) {

        const auto particle_mesh_p = p->getMeshP();

        fe::getCurrentQuadPoints(particle_mesh_p.get(), d_xRef, d_u, d_xQuadCur,
                                 p->d_globStart,
                                 p->d_globQuadStart,
                                 d_modelDeck_p->d_quadOrder);

        auto p_z_id = p->d_zoneId;
        auto isPlaneStrain = d_pDeck_p->d_particleZones[p_z_id].d_matDeck.d_isPlaneStrain;
        fe::getStrainStress(particle_mesh_p.get(), d_xRef, d_u,
                            isPlaneStrain,
                            d_strain, d_stress,
                            p->d_globStart,
                            p->d_globQuadStart,
                            d_particlesMatDataList[p->getId()].d_nu,
                            d_particlesMatDataList[p->getId()].d_lambda,
                            d_particlesMatDataList[p->getId()].d_mu,
                            true,
                            d_modelDeck_p->d_quadOrder);
      } // for loop over particles
    } // compute strain/stress block

    auto writer1 = rw::writer::VtkParticleWriter(d_outputDeck_p->d_path +
                                                 "output_strain_" +
                                                 std::to_string(d_n / dt_out));
    writer1.appendStrainStress(this);
    writer1.addTimeStep(d_time);
    writer1.close();
  }

  // output particle locations to csv file
  if (util::methods::isTagInList("Particle_Locations",
                                 d_outputDeck_p->d_outTags)) {

    std::ofstream oss(d_outputDeck_p->d_path + "particle_locations_" +
                      std::to_string(d_n / dt_out) + ".csv");
    oss << "i, x, y, z, r\n";
    for (const auto &p : d_particlesListTypeAll) {
      auto xc = p->getXCenter();
      oss << p->d_zoneId << ", " << xc.d_x << ", " << xc.d_y << ", " << xc.d_z
          << ", " << p->d_geom_p->boundingRadius() << "\n";
    }
    oss.close();
  }
}

std::string model::DEMModel::ppTwoParticleTest() {

  bool continue_dt = false;
  auto check_dt = d_outputDeck_p->d_dtTestOut;
  if ((d_n % check_dt == 0) && (d_n >= check_dt))
    continue_dt = true;

  if (!continue_dt)
    return "";

  // get alias for particles
  const auto &p0 = this->d_particlesListTypeAll[0];
  const auto &p1 = this->d_particlesListTypeAll[1];

  // get penetration distance
  const auto &xc0 = p0->getXCenter();
  const auto &xc1 = p1->getXCenter();
  const double &r = p0->d_geom_p->boundingRadius();

  const auto &contact = d_cDeck_p->getContact(p0->d_zoneId, p1->d_zoneId);
  double r_e = r + contact.d_contactR;

  double pen_dist = xc1.dist(xc0) - r_e - r;
  double contact_area_radius = 0.;
  if (util::isLess(pen_dist, 0.))
    contact_area_radius =
        std::sqrt(std::pow(r_e, 2.) - std::pow(r_e + pen_dist, 2.));
  else if (util::isGreater(pen_dist, 0.)) {
    pen_dist = 0.;
    contact_area_radius = 0.;
  }

  // get max distance of second particle (i.e. the y-coord of center + radius)
  double max_dist = xc1.d_y + p1->d_geom_p->boundingRadius();

  // compute maximum y coordinate of particle 2
  double max_y_loc = p1->getXLocal(0).d_y;
  double max_y = 0.;
  for (size_t i = 0; i < p1->getNumNodes(); i++)
    if (util::isLess(max_y_loc, p1->getXLocal(i).d_y))
      max_y_loc = p1->getXLocal(i).d_y;

  if (util::isLess(max_y, max_y_loc))
    max_y = max_y_loc;

  setKeyData("pen_dist", pen_dist);
  setKeyData("contact_area_radius", contact_area_radius);
  setKeyData("max_y", max_y);
  setKeyData("max_dist", max_dist);
  setKeyData("max_y_loc", max_y_loc);


  return fmt::format("  Post-processing: max y = {:.6f} \n", max_y);
}

void model::DEMModel::checkStop() {

  if (d_outputDeck_p->d_outCriteria == "max_particle_dist" &&
      d_pDeck_p->d_testName == "two_particle") {

    // compute max distance between two particles
    // current center position
    const auto &xci = d_particlesListTypeAll[0]->getXCenter();
    const auto &xcj = d_particlesListTypeAll[1]->getXCenter();

    // check
    if (util::isGreater(xci.dist(xcj),
                        d_outputDeck_p->d_outCriteriaParams[0])) {

      if(d_ppFile.is_open())
        d_ppFile.close();
      exit(1);
    }
  }
  else if (d_outputDeck_p->d_outCriteria == "max_node_dist") {

    //    static int msg_printed = 0;
    //    if (msg_printed == 0) {
    //      std::cout << "Check = " << d_outputDeck_p->d_outCriteria
    //              << " is no longer supported. In future, this test will be implemented when function util::methods::maxLength() is defined." << std::endl;
    //      msg_printed = 1;
    //    }
    //exit(EXIT_FAILURE);
    auto max_pt_and_index = util::methods::maxLengthAndMaxLengthIndex(d_x);
    auto max_x = d_x[max_pt_and_index.second];

    // check
    if (util::isGreater(max_pt_and_index.first,
                        d_outputDeck_p->d_outCriteriaParams[0])) {

      // close open file
      if(d_ppFile.is_open())
        d_ppFile.close();

      log(fmt::format("{}: Terminating simulation as one of the failing"
                      " criteria is met. Point ({:.6f}, {:.6f}, {:.6f}) is at "
                      "distance {:.6f} "
                      "more than"
                      " allowed distance {:.6f}\n",
                      d_name, max_x.d_x, max_x.d_y, max_x.d_z, max_x.length(),
                      d_outputDeck_p->d_outCriteriaParams[0]));
      exit(1);
    }
  }
}

std::string model::DEMModel::ppCompressiveTest() {
  bool continue_dt = false;
  auto check_dt = d_outputDeck_p->d_dtTestOut;
  if ((d_n % check_dt == 0) && (d_n >= check_dt))
    continue_dt = true;

  if (!continue_dt)
    return "";

  // get wall
  auto w_id = d_pDeck_p->d_particleIdCompressiveTest;
  auto f_dir = d_pDeck_p->d_particleForceDirectionCompressiveTest - 1;
  const auto &wall = d_particlesListTypeAll[w_id];

  // find the penetration of the wall from it's original location
  auto dx = wall->getXLocal(0) - wall->getXRefLocal(0);
  double wall_penetration = dx[f_dir];

  // get the total reaction force on wall along the direction of loading
  double tot_reaction_force = 0.;
  for (size_t i = 0; i < wall->getNumNodes(); i++) {
    tot_reaction_force += wall->getFLocal(i)[f_dir] * wall->getVolLocal(i);
  }

  // open file and write
  bool use_static_file = true;
  if (use_static_file) {
    if (!d_ppFile.is_open()) {

      std::string filename = d_outputDeck_p->d_path + "pp_" +
                             d_pDeck_p->d_testName + "_" +
                             d_outputDeck_p->d_tagPPFile + ".csv";
      d_ppFile.open(filename.c_str(), std::ofstream::out | std::ofstream::app);

      d_ppFile << "t, delta, force \n";
    }

    d_ppFile << fmt::format("%4.6e, %4.6e, %4.6e\n", d_time, wall_penetration,
            tot_reaction_force);
  }

  setKeyData("wall_penetration", wall_penetration);
  setKeyData("tot_reaction_force", tot_reaction_force);

  return fmt::format("  Post-processing: wall penetration = {:"
                     ".6f}, "
                     "reaction force = {:5.3e} \n",
                     wall_penetration, tot_reaction_force);
}