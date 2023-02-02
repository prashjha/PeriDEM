/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "demModel.h"

// utils
#include "material/materialUtil.h"
#include "particle/particle.h"
#include "particle/wall.h"
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
#include "inp/decks/materialDeck.h"
#include "inp/decks/modelDeck.h"
#include "inp/decks/outputDeck.h"
#include "inp/decks/restartDeck.h"
#include "rw/vtkParticleWriter.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>

#include <taskflow/taskflow/taskflow.hpp>

namespace {

FILE *pp_file = nullptr;

double pen_dist = 0.;
double contact_area_radius = 0.;

double tree_compute_time = 0.;
double contact_compute_time = 0.;
double pd_compute_time = 0.;
double extf_compute_time = 0.;
double integrate_compute_time = 0.;

double max_y = 0.;

steady_clock::time_point clock_begin = steady_clock::now();
steady_clock::time_point clock_end = steady_clock::now();

std::ostringstream oss;

void addToList(std::vector<size_t> *list, size_t i) {
  for (auto j : *list)
    if (j == i)
      return;

  list->emplace_back(i);
}

bool isInList(const std::vector<size_t> *list, size_t i) {
  for (auto j : *list)
    if (j == i)
      return true;

  return false;
}

} // namespace

model::DEMModel::DEMModel(inp::Input *deck) : ModelData(deck) {

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
}

void model::DEMModel::restart(inp::Input *deck) {

  log("DEMModel: Restarting the simulation\n");

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

  // compute reference configuration
  // If asked, change only those particles/walls configuration which are free
  // (at least one dof is free)
  //  for (auto &p: d_allParticles) {
  //    bool p_all_fixed = true;
  //    for (size_t i=0;i<p->getNumNodes(); i++) {
  //      for (int dof=0; dof<3; dof++)
  //        if (util::methods::isFree(p->getFixLocal(i), dof))
  //          p_all_fixed = false;
  //
  //      if (!p_all_fixed)
  //        break;
  //    }
  //
  //    // change reference configuration
  //    bool change_config = true;
  //    if (d_restartDeck_p->d_changeRefFreeDofs and p_all_fixed)
  //      change_config = false;
  //
  //    if (change_config) {
  //      for (size_t i=0;i<p->getNumNodes(); i++) {
  //        p->setXRefLocal(i, p->getXLocal(i) - p->getULocal(i));
  //      }
  //    }
  //  }
}

void model::DEMModel::init() {

  // init time step
  d_n = 0;
  d_time = 0.;
  if (d_outputDeck_p->d_dtTestOut == 0)
    d_outputDeck_p->d_dtTestOut = d_outputDeck_p->d_dtOut / 10;
  d_infoN = d_outputDeck_p->d_dtOut;

  auto t1 = steady_clock::now();
  log("DEMModel: Initializing objects.\n");

  // create particles
  log("DEMModel: Creating particles.\n");
  createParticles();

  // create particles
  log("DEMModel: Creating walls.\n");
  createWalls();

  // setup contact
  log("DEMModel: Setting up contact.\n");
  setupContact();

  // create search object
  log("DEMModel: Creating neighbor search tree.\n");

  // create tree object
  d_nsearch_p = std::make_unique<NSearch>(d_x, d_outputDeck_p->d_debug);

  // setup tree
  double set_tree_time = d_nsearch_p->updatePointCloud(d_x, true);
  set_tree_time += d_nsearch_p->setInputCloud();
  log(fmt::format("DEMModel: Tree setup time (ms) = {}. \n", set_tree_time));

  // create neighborlists
  log("DEMModel: Creating neighborlist for peridynamics.\n");
  // log("DEMModel: Creating neighborlist for contact.\n");
  updatePeridynamicNeighborlist();
  // updateContactNeighborlist();
  // updateNeighborlistCombine();

  // create peridynamic bonds
  log("DEMModel: Creating peridynamics bonds.\n");
  d_fracture_p = std::make_unique<geometry::Fracture>(&d_x, &d_neighPd);

  // compute quantities in state-based simulations
  log("DEMModel: Compute state-based peridynamic quantities.\n");
  material::computeStateMx(this, true);

  // initialize loading class
  log("DEMModel: Initializing displacement loading object.\n");
  d_uLoading_p =
      std::make_unique<loading::ParticleULoading>(d_pDeck_p->d_dispDeck);
  for (auto &p : d_allParticles)
    d_uLoading_p->setFixity(p);

  log("DEMModel: Initializing force loading object.\n");
  d_fLoading_p =
      std::make_unique<loading::ParticleFLoading>(d_pDeck_p->d_forceDeck);

  // if all dofs of particle is fixed, then mark it so that we do not
  // compute force
  // MAYBE NOT as we may be interested in reaction forces
  //  for (auto &p : d_allParticles)
  //    p->checkFixityForForce(); // TODO implement

  // if this is a two-particle test, we set the force calculation off in
  // first particle
  if (d_pDeck_p->d_testName == "two_particle")
    d_particles[0]->d_computeForce = false;

  log(fmt::format("DEMModel: Total particles = {}. \n", d_particles.size()));

  for (const auto &w : d_walls)
    if (!w->d_computeForce)
      log(fmt::format("DEMModel: force OFF in Wall i = {}. \n", w->getTypeId()));

  log("DEMModel: Creating list of nodes on which force is to be computed.\n");
  // TODO for now we simply look at particle/wall and check if we compute
  //  force on any of its node. Later, one can have control on individual
  //  nodes of particle/wall and remove from d_fCompNodes if no force is to
  //  be computed on them
  for (size_t i = 0; i < d_x.size(); i++) {
    const auto &ptId = d_ptId[i];
    const auto &pi = getBaseParticle(ptId);
    if (pi->d_computeForce) {
      d_fContCompNodes.push_back(i);
      if (pi->getTypeIndex() == 0)
        d_fPdCompNodes.push_back(i); // internal force on particles
    }
  }

  // initialize remaining fields (if any)
  d_Z = std::vector<float>(d_x.size(), 0.);

  auto t2 = steady_clock::now();
  log(fmt::format("DEMModel: Total setup time (ms) = {}. \n",
                  util::methods::timeDiff(t1, t2)));

  // compute complexity information
  size_t free_dofs = 0;
  for (const auto &f : d_fix) {
    for (size_t dof = 0; dof < 3; dof++)
      if (util::methods::isFree(f, dof))
        free_dofs++;
  }
  log(fmt::format("DEMModel: Computational complexity information \n"
                  "  Number of "
                  "particles = {}, number of walls = {}, number of dofs = {},"
                  " number of free dofs = {}. \n",
                  d_particles.size(), d_walls.size(), 3 * d_x.size(),
                  free_dofs));
}

void model::DEMModel::integrate() {

  // perform output at the beginning
  if (d_n == 0 && d_outputDeck_p->d_performOut)
    output();

  // apply initial condition
  if (d_n == 0)
    applyInitialCondition();

  // apply loading
  computeExternalDisplacementBC();
  computeForces();

  for (size_t i = d_n; i < d_modelDeck_p->d_Nt; i++) {

    log(fmt::format("DEMModel: Time step: {}, time: {:.6f}\n", i, d_time), 2, d_n % d_infoN == 0, 3);
    
    clock_begin = steady_clock::now();
    log("Integrating\n", false, 0, 3);
    integrateStep();
    double integrate_time =
        util::methods::timeDiff(clock_begin, steady_clock::now());
    integrate_compute_time += integrate_time;

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
      clock_begin = steady_clock::now();
      output();
    }

    // check for stop
    checkStop();

  } // loop over time steps

  log(fmt::format(
          "DEMModel: Total compute time information (s) \n"
          "  Integration = {:.6f}, Peridynamic = {:.6f}, Contact = {:.6f}, "
          "Tree update = {:.6f}, External force = {:.6f}\n",
          integrate_compute_time * 1.e-6, pd_compute_time * 1.e-6,
          contact_compute_time * 1.e-6, tree_compute_time * 1.e-6,
          extf_compute_time * 1.e-6), 1);
}

void model::DEMModel::integrateStep() {
  if (d_modelDeck_p->d_timeDiscretization == "central_difference")
    integrateCD();
  else if (d_modelDeck_p->d_timeDiscretization == "velocity_verlet")
    integrateVerlet();
}

void model::DEMModel::integrateCD() {

  // update velocity and displacement
  const auto dt = d_modelDeck_p->d_dt;
  const auto dim = d_modelDeck_p->d_dim;

  tf::Executor executor;
  tf::Taskflow taskflow;

  taskflow.for_each(
    0, d_fPdCompNodes.size(),
      [this, dt, dim](uint64_t II) {
        auto i = this->d_fPdCompNodes[II];

        const auto rho = this->getDensity(i);
        const auto &fix = this->d_fix[i];

        for (int dof = 0; dof < dim; dof++) {
          if (util::methods::isFree(fix, dof)) {
            this->d_v[i][dof] += (dt / rho) * this->d_f[i][dof];
            this->d_u[i][dof] += dt * this->d_v[i][dof];
            this->d_x[i][dof] += dt * this->d_v[i][dof];
          }
        }
      } // loop over nodes
  ); // for_each

  executor.run(taskflow).get();

  d_n++;
  d_time += dt;

  // update displacement bc
  computeExternalDisplacementBC();

  // compute force
  computeForces();
}

void model::DEMModel::integrateVerlet() {

  // update velocity and displacement
  const auto dt = d_modelDeck_p->d_dt;
  const auto dim = d_modelDeck_p->d_dim;

  {
    tf::Executor executor;
    tf::Taskflow taskflow;

    taskflow.for_each(
      0, d_fPdCompNodes.size(),
        [this, dt, dim](boost::uint64_t II) {
          auto i = this->d_fPdCompNodes[II];

          const auto rho = this->getDensity(i);
          const auto &fix = this->d_fix[i];

          for (int dof = 0; dof < dim; dof++) {
            if (util::methods::isFree(fix, dof)) {
              this->d_v[i][dof] += 0.5 * (dt / rho) * this->d_f[i][dof];
              this->d_u[i][dof] += dt * this->d_v[i][dof];
              this->d_x[i][dof] += dt * this->d_v[i][dof];
            }
          }
        } // loop over nodes
    ); // for_each

    executor.run(taskflow).get();
  }

  d_n++;
  d_time += dt;

  // update displacement bc
  computeExternalDisplacementBC();

  // compute force
  computeForces();

  {
    tf::Executor executor;
    tf::Taskflow taskflow;

    taskflow.for_each(
      0, d_fPdCompNodes.size(),
      [this, dt, dim](boost::uint64_t II) {
        auto i = this->d_fPdCompNodes[II];

        const auto rho = this->getDensity(i);
        const auto &fix = this->d_fix[i];
        for (int dof = 0; dof < dim; dof++) {
          if (util::methods::isFree(fix, dof)) {
            this->d_v[i][dof] += 0.5 * (dt / rho) * this->d_f[i][dof];
          }
        }
      } // loop over nodes
    ); // for_each

    executor.run(taskflow).get();
  }
}

void model::DEMModel::computeForces() {

  bool dbg_condition = d_n % d_infoN == 0;

  log("  Compute forces \n", 2, dbg_condition, 3);

  // update the point cloud (make sure that d_x is updated along with
  // displacment)
  auto pt_cloud_update_time = d_nsearch_p->updatePointCloud(d_x, true);
  pt_cloud_update_time += d_nsearch_p->setInputCloud();
  tree_compute_time += pt_cloud_update_time;
  log(fmt::format("    Point cloud update time (ms) = {} \n",
                    pt_cloud_update_time), 2, dbg_condition, 3);

  // reset force
  auto t1 = steady_clock::now();
  tf::Executor executor;
  tf::Taskflow taskflow;

  taskflow.for_each(
    0, d_x.size(),
      [this](boost::uint64_t i) { this->d_f[i] = util::Point(); }
  ); // for_each

  executor.run(taskflow).get();

  log(fmt::format("    Force reset time (ms) = {} \n",
                    util::methods::timeDiff(t1, steady_clock::now())), 2, dbg_condition, 3);

  // compute peridynamic forces
  t1 = steady_clock::now();
  computePeridynamicForces();
  auto pd_time = util::methods::timeDiff(t1, steady_clock::now());
  pd_compute_time += pd_time;
  log(fmt::format("    Peridynamics force time (ms) = {} \n", pd_time), 2, dbg_condition, 3);

  // compute contact forces between particles
  t1 = steady_clock::now();
  computeContactForces();
  auto contact_time = util::methods::timeDiff(t1, steady_clock::now());
  contact_compute_time += contact_time;
  log(fmt::format("    Contact force time (ms) = {} \n", contact_time), 2, dbg_condition, 3);

  // Compute external forces
  t1 = steady_clock::now();
  computeExternalForces();
  auto extf_time = util::methods::timeDiff(t1, steady_clock::now());
  extf_compute_time += extf_time;
  log(fmt::format("    External force time (ms) = {} \n", extf_
  time), 2, dbg_condition, 3);
}

void model::DEMModel::computePeridynamicForces() {

  log("    Computing peridynamic force \n", 3);

  const auto dim = d_modelDeck_p->d_dim;
  const bool is_state = d_allParticles[0]->getMaterial()->isStateActive();

  // compute state-based helper quantities
  if (is_state) {
    auto f = hpx::parallel::for_loop(
        hpx::parallel::execution::par(hpx::parallel::execution::task), 0,
        d_fPdCompNodes.size(),
        [this](boost::uint64_t II) {
          auto i = this->d_fPdCompNodes[II];

          const auto rho = this->getDensity(i);
          const auto &fix = this->d_fix[i];
          const auto &ptId = this->getPtId(i);
          auto &pi = this->getBaseParticle(ptId);

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
        }   // loop over nodes
    );      // end of parallel for loop
    f.get();
  }

  // compute the internal forces
  auto f = hpx::parallel::for_loop(
      hpx::parallel::execution::par(hpx::parallel::execution::task), 0,
      d_fPdCompNodes.size(), [this](boost::uint64_t II) {
        auto i = this->d_fPdCompNodes[II];

        // local variable to hold force
        util::Point force_i = util::Point();
        double scalar_f = 0.;

        // for damage
        float Zi = 0.;

        const auto rhoi = this->getDensity(i);
        const auto &ptIdi = this->getPtId(i);
        auto &pi = this->getBaseParticle(ptIdi);

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

        // update force (combines clearing of previous force and addition of
        // internal force if compute force loop over nodes end of parallel
        // for loop)
        this->d_f[i] = force_i;

        this->d_Z[i] = Zi;
      });
  f.get();
}

void model::DEMModel::computeExternalForces() {

  log("    Computing external force \n", 3);

  auto gravity = d_pDeck_p->d_gravity;

  if (gravity.length() > 1.0E-8) {
    auto f = hpx::parallel::for_loop(
        hpx::parallel::execution::par(hpx::parallel::execution::task), 0,
        d_x.size(),
        [this, gravity](boost::uint64_t i) {
          this->d_f[i] += this->getDensity(i) * gravity;
        } // loop over particles
    );    // end of parallel for loop
    f.get();
  }

  //
  for (auto &p : d_allParticles)
    d_fLoading_p->apply(d_time, p); // applied in parallel
}

void model::DEMModel::computeExternalDisplacementBC() {
  log("    Computing external displacement bc \n", 3);
  for (auto &p : d_allParticles)
    d_uLoading_p->apply(d_time, p); // applied in parallel
}

void model::DEMModel::computeContactForces() {

  log("    Computing normal contact force \n", 3);

  // Description:
  // 1. Normal contact is applied between nodes of particles and walls
  // 2. Normal damping is applied between particle centers
  // 3. Normal damping is applied between nodes of particle and wall pairs

  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, d_fContCompNodes.size(),
      [this](boost::uint64_t II) {
        auto i = this->d_fContCompNodes[II];

        // local variable to hold force
        util::Point force_i = util::Point();
        double scalar_f = 0.;

        const auto &ptIdi = this->getPtId(i);
        auto &pi = this->getBaseParticle(ptIdi);
        double horizon = pi->d_material_p->getHorizon();
        double search_r = this->d_maxContactR;

        // particle data
        double rhoi = pi->getDensity();

        const auto &yi = this->d_x[i]; // current coordinates
        const auto &ui = this->d_u[i];
        const auto &vi = this->d_v[i];
        const auto &voli = this->d_vol[i];

        std::vector<size_t> neighs;
        std::vector<double> sqr_dist;
        auto search_status =
            this->d_nsearch_p->radiusSearch(yi, search_r, neighs, sqr_dist);

        if (search_status > 0) {
          for (std::size_t j = 0; j < neighs.size(); ++j) {

            auto &j_id = neighs[j];
            auto &rij_sqr = sqr_dist[j];
            auto &ptIdj = this->d_ptId[j_id];
            auto &pj = this->getBaseParticle(ptIdj);
            double rhoj = pj->getDensity();
            // double Rji = std::sqrt(rij_sqr);
            double Rji = (this->d_x[j_id] - yi).length();

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

                    auto &pii = this->d_particles[pi->getTypeId()];
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
      });

  // damping force
  log("    Computing normal damping force \n", 3);
  for (auto &pi : d_particles) {

    double Ri = pi->d_geom_p->boundingRadius();
    double vol_pi = M_PI * Ri * Ri;
    auto pi_xc = pi->getXCenter();
    auto pi_vc = pi->getVCenter();
    auto rhoi = pi->getDensity();
    util::Point force_i = util::Point();

    // particle-particle
    for (auto &pj : this->d_particles) {
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
    std::vector<std::vector<size_t>> wall_nodes(pi->getNumNodes());
    hpx::parallel::for_loop(
        hpx::parallel::execution::par, 0, pi->getNumNodes(),
        [this, pi, &wall_nodes](boost::uint64_t i) {
          auto yi = this->d_x[pi->getNodeId(i)];
          double search_r = this->d_maxContactR;
          std::vector<size_t> neighs;
          std::vector<double> sqr_dist;
          auto search_status =
              this->d_nsearch_p->radiusSearch(yi, search_r, neighs, sqr_dist);

          if (search_status > 0) {
            for (std::size_t j = 0; j < neighs.size(); ++j) {
              auto &j_id = neighs[j];
              auto &ptIdj = this->d_ptId[j_id];
              auto &pj = this->getBaseParticle(ptIdj);

              // we are only interested in nodes from wall
              if (pj->getTypeIndex() == 1) {
                double Rji = (this->d_x[j_id] - yi).length();
                const auto &contact =
                    d_cDeck_p->getContact(pi->d_zoneId, pj->d_zoneId);
                if (util::isLess(Rji, contact.d_contactR))
                  wall_nodes[i].push_back(j_id);
              }
            }
          }
        });

    // condense wall nodes into single vector
    std::vector<size_t> wall_nodes_condense;
    for (auto &nds : wall_nodes) {
      for (auto &j : nds)
        addToList(&wall_nodes_condense, j);
    }

    // now loop over wall nodes and add force to center of particle
    for (auto &j : wall_nodes_condense) {
      auto &ptIdj = this->d_ptId[j];
      auto &pj = this->getWall(this->d_allParticles[ptIdj]->getTypeId());
      auto rhoj = pj->getDensity();
      auto volj = this->d_vol[j];
      auto meq = rhoi * vol_pi;
      //auto meq = util::equivalentMass(rhoi * vol_pi, rhoj * volj);

      const auto &contact = d_cDeck_p->getContact(pi->d_zoneId, pj->d_zoneId);

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
    hpx::parallel::for_loop(
        hpx::parallel::execution::par, 0, pi->getNumNodes(),
        [this, pi, force_i](boost::uint64_t i) {
          this->d_f[pi->getNodeId(i)] += force_i;
        });
  }
}

void model::DEMModel::applyInitialCondition() {

  log("Applying initial condition \n", 3);

  if (!d_pDeck_p->d_icDeck.d_icActive)
    return;

  const auto ic_v = d_pDeck_p->d_icDeck.d_icVec;
  const auto ic_p_list = d_pDeck_p->d_icDeck.d_pList;

  // add specified velocity to particle
  auto f = hpx::parallel::for_loop(
      hpx::parallel::execution::par(hpx::parallel::execution::task), 0,
      ic_p_list.size(),
      [this, ic_v, ic_p_list](boost::uint64_t i) {
        auto &p = this->d_particles[ic_p_list[i]];

        // velocity
        for (size_t j = 0; j < p->getNumNodes(); j++)
          p->setVLocal(j, ic_v);
      } // loop over particles
  );    // end of parallel for loop
  f.get();
}

void model::DEMModel::createParticles() {

  d_particles.resize(0);
  d_allParticles.resize(0);
  d_rParticles.clear();

  // loop over all particle zones
  for (size_t z = 0; z < d_pDeck_p->d_pZones.size(); z++) {

    // get current size of particles data
    auto psize = d_particles.size();

    // get particle zone
    auto &pz = d_pDeck_p->d_pZones[z];

    // get zone id
    auto z_id = pz.d_zone.d_zoneId;

    //    log(fmt::format("Processing zone = {} with ref particle\n {}\n", z_id,
    //                    pz.d_rParticle_p->printStr(0,0)));

    // get representative particle for this zone
    auto &rep_p = pz.d_particle_p;
    auto rep_p_params = pz.d_params;

    // read mesh data
    log("DEMModel: Creating mesh for reference particle in zone = " +
                  std::to_string(z_id) + "\n");
    auto *mesh = new fe::Mesh(&pz.d_meshDeck);
    // mesh->print();

    // create the reference particle
    log("DEMModel: Creating reference particle in zone = " +
                  std::to_string(z_id) + "\n");

    auto ref_p = std::make_shared<particle::RefParticle>(&pz, mesh);
    // ref_p->print();
    d_rParticles.emplace_back(ref_p);

    //
    // create material for particles in this zone
    // If horizon is provided, set horizon to that value.
    // If horizon to mesh ratio is provided, set horizon accordingly.
    // If both are not provided, take horizon as radius of particle.
    //
    auto &mdeck = pz.d_matDeck;
    double radius = rep_p_params[0];
    double horizon = radius;
    if (mdeck.d_horizon > 0.)
      horizon = mdeck.d_horizon;
    else {
      if (mdeck.d_horizonMeshRatio > 0.)
        horizon = mdeck.d_horizonMeshRatio *
                  (radius / ref_p->getParticleRadius()) * mesh->getMeshSize();
    }
    mdeck.d_horizon = horizon;
    // mdeck.print();

    // check the particle generation method
    log("DEMModel: Creating particles in zone = " +
                  std::to_string(z_id) + "\n");

    if (pz.d_genMethod == "From_File")
      createParticlesFromFile(z, ref_p);
    else {
      std::cerr << "Error: Particle generation method not recognized.\n";
      exit(1);
    }

    // get new size of data
    auto psize_new = d_particles.size();

    // store this in zone-info
    d_zInfo.emplace_back(std::vector<size_t>{psize, psize_new, z_id});
  }
}

void model::DEMModel::createParticlesFromFile(
    size_t z, std::shared_ptr<particle::RefParticle> ref_p) {

  // get particle zone
  auto &pz = d_pDeck_p->d_pZones[z];

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
  } else if (pz.d_particleFileDataType == "loc_rad_orient") {
    rw::reader::readParticleWithOrientCsvFile(pz.d_particleFile,
                                              d_modelDeck_p->d_dim, &centers,
                                              &rads, &orients, z_id);
  }

  log(fmt::format("zone_id: {}, rads: {}, orients: {}, centers: {} \n", z_id,
                    util::io::printStr(rads), util::io::printStr(orients),
                    util::io::printStr(centers)), 2);

  // get representative particle for this zone
  const auto &rep_p = pz.d_particle_p;
  auto rep_p_params = pz.d_params;

  // get zone bounding box
  std::pair<util::Point, util::Point> box = pz.d_zone.d_geom_p->box();

  size_t p_counter = 0;
  size_t p_old_size = d_particles.size();
  for (const auto &site : centers) {

    double particle_orient = orients[p_counter];
    double particle_radius = rads[p_counter];
    std::shared_ptr<util::geometry::GeomObject> po;
    // call createGeomObject for particle creation
    {
      std::vector<double> params;
      for (auto x : rep_p_params)
        params.push_back(x);
      if (rep_p->d_name == "circle" or rep_p->d_name == "sphere" or
          rep_p->d_name == "hexagon" or rep_p->d_name == "triangle") {

        if (params.size() < 4)
          params.resize(4);
        params[0] = particle_radius;
        for (int dof = 0; dof < 3; dof++)
          params[dof + 1] = site[dof];
      } else if (rep_p->d_name == "drum2d") {

        if (params.size() < 5)
          params.resize(5);

        params[0] = particle_radius;
        params[1] = particle_radius * params[1] / params[0];
        for (int dof = 0; dof < 3; dof++)
          params[dof + 2] = site[dof];
      } else {
        std::cerr << "Error: PeriDEM currently only supports circle, sphere, "
                     ", hexagon, triangle, or drum2d shaped particles.\n";
        exit(1);
      }

      std::vector<std::string> vec_geom_type;
      std::vector<std::string> vec_geom_flag;
      util::geometry::createGeomObject(rep_p->d_name, params, vec_geom_type,
                                       vec_geom_flag, po, d_modelDeck_p->d_dim,
                                       false);
    }

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
    auto p = new particle::Particle(
        p_counter + p_old_size, pz, z_id, ref_p, po, p_transform,
        static_cast<std::shared_ptr<ModelData>>(this), true);

    // push p to list
    d_particles.push_back(p);
    d_allParticles.push_back(p);

    p_counter++;
  }
}

void model::DEMModel::createWalls() {

  d_walls.resize(0);
  size_t wall_id_begin = d_particles.size();

  for (size_t z = 0; z < d_pDeck_p->d_wZones.size(); z++) {

    // get wall zone
    auto &wz = d_pDeck_p->d_wZones[z];

    // get zone id
    auto z_id = wz.d_zone.d_zoneId;

    // read and create mesh data
    fe::Mesh *mesh;
    if (wz.d_meshFlag) {
      log("DEMModel: Creating mesh for wall in zone = " +
                    std::to_string(z_id) + "\n");
      // wz.d_meshDeck.print();
      mesh = new fe::Mesh(&wz.d_meshDeck);
    } else
      mesh = new fe::Mesh(d_modelDeck_p->d_dim);

    //
    // create material for particles in this zone
    // If horizon is provided, set horizon to that value.
    // If horizon to mesh ratio is provided, set horizon accordingly.
    // If both are not provided, take horizon as radius of particle.
    //
    auto &mdeck = wz.d_matDeck;
    double horizon = 0.;
    if (mdeck.d_horizon > 0.)
      horizon = mdeck.d_horizon;
    else {
      if (mdeck.d_horizonMeshRatio > 0.)
        horizon = mdeck.d_horizonMeshRatio * mesh->getMeshSize();
      else {
        std::cerr
            << "Error: Can not calculate horizon for wall in zone = " << z
            << ". Either provide horizon or horizon/mesh ratio in input file"
               ".\n";
        exit(1);
      }
    }

    mdeck.d_horizon = horizon;

    // create wall
    log("DEMModel: Creating wall in zone = " + std::to_string(z_id) +
                  "\n");
    auto wall =
        new particle::Wall(z + wall_id_begin, z, wz, z_id, mesh,
                           static_cast<std::shared_ptr<ModelData>>(this), true);

    // add data
    d_walls.push_back(wall);
    d_allParticles.push_back(wall);
  }
}

void model::DEMModel::setupContact() {

  // loop over all particle zones and get minimum value of mesh size
  size_t c = 0;
  for (const auto *p : d_particles) {

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

  log(fmt::format("DEMModel: Contact setup\n  hmin = {:.6f}, hmax = {:.6f} \n", d_hMin, d_hMax), 1);

  d_maxContactR = 0.;
  // precompute bulk modulus of all zones
  std::vector<double> bulk_modulus;
  for (size_t i = 0; i < d_cDeck_p->d_data.size(); i++) {

    double kappa_i = -1.;
    if (d_pDeck_p->d_zoneToPorWDeck[i].first == "particle") {
      auto z = d_pDeck_p->d_pZones[d_pDeck_p->d_zoneToPorWDeck[i].second];
      kappa_i = z.d_matDeck.d_matData.d_K;
    } else if (d_pDeck_p->d_zoneToPorWDeck[i].first == "wall") {
      auto z = d_pDeck_p->d_wZones[d_pDeck_p->d_zoneToPorWDeck[i].second];
      kappa_i = z.d_matDeck.d_matData.d_K;
    } else {
      std::cerr << "Error: Should not reach here. zone id: " << i << "\n";
      for (auto a : d_pDeck_p->d_zoneToPorWDeck)
        std::cerr << a.first << ", " << a.second << "\n";

      exit(1);
    }

    if (kappa_i < 0.) {
      std::cerr << "Error: We need bulk modulus provided in input file.\n";

      if (d_pDeck_p->d_zoneToPorWDeck[i].first == "particle")
        std::cerr << d_pDeck_p->d_pZones[d_pDeck_p->d_zoneToPorWDeck[i].second]
                         .printStr();
      else if (d_pDeck_p->d_zoneToPorWDeck[i].first == "wall")
        std::cerr << d_pDeck_p->d_wZones[d_pDeck_p->d_zoneToPorWDeck[i].second]
                         .printStr();
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
                      deck->d_betan, deck->d_mu, deck->d_kappa), 1);
    }
  }
}

void model::DEMModel::updatePeridynamicNeighborlist() {

  d_neighPd.resize(d_x.size());
  // d_neighPdSqdDist.resize(d_x.size());
  auto t1 = steady_clock::now();
  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, d_x.size(), [this](boost::uint64_t i) {
        const auto &pi = this->d_ptId[i];
        double search_r = this->d_allParticles[pi]->d_material_p->getHorizon();

        std::vector<size_t> neighs;
        std::vector<double> sqr_dist;
        if (this->d_nsearch_p->radiusSearch(this->d_x[i], search_r, neighs,
                                            sqr_dist) > 0) {
          for (std::size_t j = 0; j < neighs.size(); ++j)
            if (neighs[j] != i && this->d_ptId[neighs[j]] == pi) {
              this->d_neighPd[i].push_back(size_t(neighs[j]));
              // this->d_neighPdSqdDist[i].push_back(sqr_dist[j]);
            }
        }
      });
  auto t2 = steady_clock::now();
  log(fmt::format("DEMModel: Peridynamics neighbor update time = {}\n",
                  util::methods::timeDiff(t1, t2)), 2);
}

void model::DEMModel::updateContactNeighborlist() {
  d_neighC.resize(d_x.size());
  // d_neighCSqdDist.resize(d_x.size());
  auto t1 = steady_clock::now();
  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, d_x.size(), [this](boost::uint64_t i) {
        const auto &pi = this->d_ptId[i];
        double search_r = this->d_maxContactR;
        std::vector<size_t> neighs;
        std::vector<double> sqr_dist;
        if (this->d_nsearch_p->radiusSearch(this->d_x[i], search_r, neighs,
                                            sqr_dist) > 0) {
          for (std::size_t j = 0; j < neighs.size(); ++j)
            if (neighs[j] != i && this->d_ptId[neighs[j]] != pi) {
              this->d_neighC[i].push_back(size_t(neighs[j]));
              // this->d_neighCSqdDist[i].push_back(sqr_dist[j]);
            }
        }
      });
  auto t2 = steady_clock::now();
  log(fmt::format("DEMModel: Contact neighbor update time = {}\n",
                  util::methods::timeDiff(t1, t2)), 2);
}

void model::DEMModel::updateNeighborlistCombine() {

  d_neighC.resize(d_x.size());
  // d_neighCSqdDist.resize(d_x.size());
  d_neighPd.resize(d_x.size());
  // d_neighPdSqdDist.resize(d_x.size());
  auto t1 = steady_clock::now();
  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, d_x.size(), [this](boost::uint64_t i) {
        const auto &pi = this->d_ptId[i];
        double horizon = this->d_allParticles[pi]->d_material_p->getHorizon();
        double search_r = horizon;
        if (this->d_maxContactR > search_r)
          search_r = this->d_maxContactR;
        std::vector<size_t> neighs;
        std::vector<double> sqr_dist;
        if (this->d_nsearch_p->radiusSearch(this->d_x[i], search_r, neighs,
                                            sqr_dist) > 0) {
          for (std::size_t j = 0; j < neighs.size(); ++j) {
            auto &j_id = neighs[j];
            auto &rij = sqr_dist[j];

            // for contact
            if (j_id != i && this->d_ptId[j_id] != pi &&
                rij < this->d_maxContactR * 1.001) {
              this->d_neighC[i].push_back(size_t(j_id));
              // this->d_neighCSqdDist[i].push_back(rij);
            }

            // for peridynamic
            if (j_id != i && this->d_ptId[j_id] == pi &&
                rij < horizon * 1.001) {
              this->d_neighPd[i].push_back(size_t(j_id));
              // this->d_neighPdSqdDist[i].push_back(rij);
            }
          }
        }
      });
  auto t2 = steady_clock::now();
  log(fmt::format("DEMModel: Peridynamics+Contact neighbor update time = {}\n",
                  util::methods::timeDiff(t1, t2)), 2);
}

void model::DEMModel::output() {

  // write out % completion of simulation at 10% interval
  {
    float p = float(d_n) * 100. / d_modelDeck_p->d_Nt;
    int m = std::max(1, int(d_modelDeck_p->d_Nt / 10));
    if (d_n % m == 0 && int(p) > 0)
      log(fmt::format("DEMModel: Simulation {}% complete\n", int(p)));
    ;
  }

  log(fmt::format("DEMModel: Output step = {}, time = {:.6f} \n", d_n, d_time),
      2);

  static int debug_once = 0;
  if (d_outputDeck_p->d_debug > 0 and debug_once < 0) {

    debug_once = 0;

    size_t nt = 1;
    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "*******************************************" << std::endl;
    oss << tabS << "Debug various input decks" << std::endl;
    oss << tabS << std::endl;
    oss << tabS << std::endl;
    oss << d_modelDeck_p->printStr(nt + 1);
    oss << d_pDeck_p->printStr(nt + 1);
    oss << d_cDeck_p->printStr(nt + 1);
    oss << tabS << std::endl;
    oss << tabS << std::endl;
    oss << tabS << "*******************************************" << std::endl;
    oss << tabS << "Debug particle data" << std::endl;
    oss << tabS << std::endl;
    oss << tabS << std::endl;
    oss << tabS << "Number of particles = " << d_particles.size() << std::endl;
    oss << tabS << "Number of particle zones = " << d_zInfo.size() << std::endl;
    for (auto zone : d_zInfo) {
      oss << tabS << "zone of d_zInfo: " << util::io::printStr(zone)
          << std::endl;
    }

    // wall info
    oss << tabS << "Number of walls = " << d_walls.size() << std::endl;
    for (auto &d_wall : d_walls)
      oss << tabS << "Number of nodes in wall in zone " << d_wall->d_zoneId
          << " is " << d_wall->getNumNodes() << std::endl;

    oss << tabS << "h_min = " << d_hMin << ", h_max = " << d_hMax << std::endl;

    // print particles
    // for (const auto &p : d_particles)
    //  oss << p->printStr(nt + 1);

    log(oss, 2);

    // dump particle and zone in txt file
    if (false) {
      std::ofstream file;
      file.open(d_outputDeck_p->d_path + "dump.txt");
      file.precision(15);
      // write particles
      for (size_t i = 0; i < d_allParticles.size(); i++) {

        auto particle = d_particles[i];
        for (size_t j = 0; j < particle->getNumNodes(); j++) {
          auto xj = particle->getXLocal(j);
          file << xj.d_x << ", " << xj.d_y << ", " << i << ", "
               << particle->d_zoneId << std::endl;
        }
      }
      file.close();
    }
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

  bool compute_strain_stress = false;
  for (const auto &tag : d_outputDeck_p->d_outTags)
    if (tag == "Strain_Stress")
      compute_strain_stress = true;

  if (compute_strain_stress && false) {
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
    for (const auto &p : d_particles) {
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
  const auto &p0 = this->d_particles[0];
  const auto &p1 = this->d_particles[1];

  // get penetration distance
  const auto &xc0 = p0->getXCenter();
  const auto &xc1 = p1->getXCenter();
  const double &r = p0->d_geom_p->boundingRadius();

  const auto &contact = d_cDeck_p->getContact(p0->d_zoneId, p1->d_zoneId);
  double r_e = r + contact.d_contactR;

  pen_dist = xc1.dist(xc0) - r_e - r;
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
  for (size_t i = 0; i < p1->getNumNodes(); i++)
    if (util::isLess(max_y_loc, p1->getXLocal(i).d_y))
      max_y_loc = p1->getXLocal(i).d_y;

  if (util::isLess(max_y, max_y_loc))
    max_y = max_y_loc;

  return fmt::format("  Post-processing: max y = {:.6f} \n", max_y);
}

void model::DEMModel::checkStop() {

  if (d_outputDeck_p->d_outCriteria == "max_particle_dist" &&
      d_pDeck_p->d_testName == "two_particle") {

    // compute max distance between two particles
    // current center position
    const auto &xci = d_particles[0]->getXCenter();
    const auto &xcj = d_particles[1]->getXCenter();

    // check
    if (util::isGreater(xci.dist(xcj),
                        d_outputDeck_p->d_outCriteriaParams[0])) {

      fclose(pp_file);
      exit(1);
    }
  } else if (d_outputDeck_p->d_outCriteria == "max_node_dist") {

    auto max_pt = util::methods::maxLength(d_x);

    // check
    if (util::isGreater(max_pt.length(),
                        d_outputDeck_p->d_outCriteriaParams[0])) {

      // close open file
      if (pp_file)
        fclose(pp_file);

      log(fmt::format("DEMModel: Terminating simulation as one of the failing"
                      " criteria is met. Point ({:.6f}, {:.6f}, {:.6f}) is at "
                      "distance {:.6f} "
                      "more than"
                      " allowed distance {:.6f}\n",
                      max_pt.d_x, max_pt.d_y, max_pt.d_z, max_pt.length(),
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
  auto w_id = d_pDeck_p->d_wallIdCompressiveTest;
  auto f_dir = d_pDeck_p->d_wallForceDirectionCompressiveTest - 1;
  const auto &wall = d_walls[w_id];

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
    if (pp_file == nullptr) {

      std::string filename = d_outputDeck_p->d_path + "pp_" +
                             d_pDeck_p->d_testName + "_" +
                             d_outputDeck_p->d_tagPPFile + ".csv";
      pp_file = fopen(filename.c_str(), "w");

      fprintf(pp_file, "t, delta, force \n");
    }

    fprintf(pp_file, "%4.6e, %4.6e, %4.6e\n", d_time, wall_penetration,
            tot_reaction_force);
  }

  return fmt::format("  Post-processing: wall penetration = {:"
                     ".6f}, "
                     "reaction force = {:5.3e} \n",
                     wall_penetration, tot_reaction_force);
}
