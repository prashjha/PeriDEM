/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "contact.h"

#include "model/modelData.h"
#include "model/modelLog.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/matrix.h"
#include "util/vecMethods.h"
#include "util/point.h"
#include "util/parallelUtil.h"
#include "inp/input.h"

#include <cmath>
#include <format>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

void contact::Contact::setup(model::ModelData &data) {


  // loop over all particle zones and get minimum value of mesh size
  size_t c = 0;
  for (const auto *p : data.d_particlesListTypeAll) {

    auto h = p->getMeshSize();
    if (c == 0) {
      data.d_hMin = h;
      data.d_hMax = h;
      c++;
    }

    if (util::isGreater(data.d_hMin, h))
      data.d_hMin = h;
    if (util::isGreater(h, data.d_hMax))
      data.d_hMax = h;
  }

  model::log(data, std::format("{}: Contact setup\n  hmin = {:.6f}, hmax = {:.6f} \n",
                  data.d_name, data.d_hMin, data.d_hMax), 1);

  data.d_maxContactR = 0.;

  auto &contactDeck = data.d_particleDeck_p->d_contactDeck;

  for (size_t i = 0; i < contactDeck.d_data.size(); i++) {
    for (size_t j = 0; j < contactDeck.d_data.size(); j++) {

      inp::ContactPairDeck *deck = &(contactDeck.d_data[i][j]);

      if (deck->d_computeContactR)
        deck->d_contactR *= data.d_hMin;

      if (data.d_maxContactR < deck->d_contactR)
        data.d_maxContactR = deck->d_contactR;

      // Kn
      deck->d_Kn *= deck->d_KnFactor;

      // Beta n
      double log_e = std::log(deck->d_eps);
      deck->d_betan =
          deck->d_betanFactor *
          (-2. * log_e * std::sqrt(1. / (M_PI * M_PI + log_e * log_e)));

      model::log(data, std::format("  contact_radius = {:.6f}, hmin = {:.6f}, Kn = {:5.3e}, "
                      "Vmax = {:5.3e}, "
                      "betan = {:7.5f}, mu = {:.4f}, kappa = {:5.3e}\n",
                      deck->d_contactR, data.d_hMin, deck->d_Kn, deck->d_vMax,
                      deck->d_betan, deck->d_mu, deck->d_K), 2);
    }
  }

}

bool contact::Contact::updateSearchParameters(model::ModelData &data) {


  // initialize parameters
  if (data.d_contNeighUpdateInterval == 0 and
      util::isLess(data.d_contNeighSearchRadius, 1.e-16)) {
    data.d_contNeighUpdateInterval = data.d_particleDeck_p->d_pNeighDeck.d_neighUpdateInterval;
    data.d_contNeighTimestepCounter = data.d_n % data.d_contNeighUpdateInterval;
    data.d_contNeighSearchRadius = data.d_maxContactR * data.d_particleDeck_p->d_pNeighDeck.d_sFactor;
  }

  // at data.d_n = 0, this function will be called twice because updateContactNeighborlist() will be
  // called twice: one inside init() and second inside computeForces()
  // so to match data.d_n and data.d_contNeighTimestepCounter in the initial stage of simulation, we need to handle the special case
  if (data.d_n == 0) {
    data.appendKeyData("update_contact_neigh_search_params_init_call_count", 1);

    if (int(data.getKeyData("update_contact_neigh_search_params_init_call_count")) == 1)
      return true;

    if (int(data.getKeyData("update_contact_neigh_search_params_init_call_count")) == 2) {
      data.d_contNeighTimestepCounter++;
      return (data.d_contNeighTimestepCounter - 1) % data.d_contNeighUpdateInterval == 0;
    }
  }

  // handle case of restart
  if (data.d_modelDeck_p->d_isRestartActive and data.d_n == data.d_restartDeck_p->d_step) {
    // assign correct value for restart step
    data.d_contNeighTimestepCounter = data.d_n % data.d_contNeighUpdateInterval;
  }

  if (data.d_contNeighUpdateInterval == 1) {
    // further optimization of parameters is not possible
    data.d_contNeighSearchRadius = data.d_maxContactR;

    // update counter and return condition for contact search
    data.d_contNeighTimestepCounter++;
    return (data.d_contNeighTimestepCounter - 1) % data.d_contNeighUpdateInterval == 0;
  }

  // check if we should proceed with parameter update
  // param update is done at smaller interval than the search itself to avoid
  // scenarios where particles suddenly move with a high velocity
  size_t update_param_interval =
          data.d_contNeighUpdateInterval > 5 ? size_t(
                  0.2 * data.d_contNeighUpdateInterval) : 1;

  // check if we ought to update search parameters; if not, return
  if (data.d_contNeighTimestepCounter > 0 and data.d_contNeighTimestepCounter % update_param_interval != 0) {
    // update counter and return condition for contact search
    data.d_contNeighTimestepCounter++;
    return (data.d_contNeighTimestepCounter - 1) % data.d_contNeighUpdateInterval == 0;
  }

  // first update the maximum velocity in all particles
  for (auto &pi : data.d_particlesListTypeAll) {
    auto max_v_node = util::methods::maxIndex(data.d_vMag,
                                              pi->d_globStart, pi->d_globEnd);

    if (max_v_node > pi->d_globEnd or max_v_node < pi->d_globStart) {
      std::cerr << std::format("Error: max_v_node = {} for "
                               "particle of id = {} is not in the limit.\n",
                               max_v_node, pi->getId())
                << "Particle info = \n"
                << pi->printStr()
                << "\n\n Magnitude of velocity = "
                << data.d_vMag[max_v_node] << "\n";
      exit(EXIT_FAILURE);
    }

    data.d_maxVelocityParticlesListTypeAll[pi->getId()]
            = data.d_vMag[max_v_node];
  }

  // find max velocity among all particles
  data.d_maxVelocity = util::methods::max(data.d_maxVelocityParticlesListTypeAll);

  // now we find the best parameters for contact search
  auto up_interval_old = data.d_contNeighUpdateInterval;

  // TO ensure that in data.d_neighUpdateInterval time steps, the search radius is above the
  // distance traveled by object with velocity data.d_maxVelocity
  // also multiply by a safety factor
  double safety_factor = data.d_particleDeck_p->d_pNeighDeck.d_sFactor > 5 ? data.d_particleDeck_p->d_pNeighDeck.d_sFactor : 10;
  auto max_search_r_from_contact_R = data.d_particleDeck_p->d_pNeighDeck.d_sFactor * data.d_maxContactR;
  auto max_search_r = data.d_maxVelocity * data.d_currentDt
                      * data.d_particleDeck_p->d_pNeighDeck.d_neighUpdateInterval
                      * safety_factor;


  if (util::isGreater(max_search_r, max_search_r_from_contact_R )) {

    data.d_contNeighUpdateInterval = size_t(data.d_maxContactR/(data.d_maxVelocity * data.d_currentDt));
    if (up_interval_old > data.d_contNeighUpdateInterval) {
      // issue warning
      model::log(data, std::format("Warning: Contact search radius based on velocity is greater than "
                      "the max contact radius.\n"
                      "Warning: Adjusting contact neighborlist update interval.\n"
                      "{:>13} = {:4.6e}, time step = {}, "
                      "velocity-based r = {:4.6e}, max contact r = {:4.6e}\n",
                      "Time", data.d_time, data.d_n, max_search_r, max_search_r_from_contact_R),
          2, data.d_n % data.d_infoN == 0, 3);
    }

    data.d_contNeighSearchRadius = max_search_r_from_contact_R;
    // reset time step counter for contact so that the contact list is updated in the current time step
    // and the update cycle starts from the current time step
    data.d_contNeighTimestepCounter = 0;

    if (data.d_contNeighUpdateInterval < 1) {
      data.d_contNeighUpdateInterval = 1;
      data.d_contNeighSearchRadius = data.d_maxContactR;
    }
  }
  else {
    // update search radius
    data.d_contNeighSearchRadius = data.d_contNeighUpdateInterval < 2 ? data.d_maxContactR : max_search_r_from_contact_R;
  }

  if (up_interval_old > data.d_contNeighUpdateInterval) {
    model::log(data, std::format("    Contact neighbor parameters: \n"
                    "      {:48s} = {:d}\n"
                    "      {:48s} = {:d}\n"
                    "      {:48s} = {:d}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n"
                    "      {:48s} = {:4.6e}\n",
                    "time step", data.d_n,
                    "contact neighbor update interval",
                    data.d_contNeighUpdateInterval,
                    "contact neighbor update time step counter",
                    data.d_contNeighTimestepCounter,
                    "search radius", data.d_contNeighSearchRadius,
                    "max contact radius", data.d_maxContactR,
                    "search radius factor", data.d_particleDeck_p->d_pNeighDeck.d_sFactor,
                    "max search r from velocity", max_search_r,
                    "max search r from contact r", max_search_r_from_contact_R,
                    "max velocity", data.d_maxVelocity),
        2, data.d_n % data.d_infoN == 0, 3);
  }

  // update counter and return condition for contact search
  data.d_contNeighTimestepCounter++;
  return (data.d_contNeighTimestepCounter - 1) % data.d_contNeighUpdateInterval == 0;

}

void contact::Contact::updateNeighborlist(model::ModelData &data) {


  auto update = updateSearchParameters(data);

  if (!update)
    return;

  // update contact neighborlist

  // update the point cloud (make sure that data.d_x is updated along with displacement)
  auto pt_cloud_update_time = data.d_nsearch_p->setInputCloud();
  data.setKeyData("pt_cloud_update_time", pt_cloud_update_time);
  data.appendKeyData("tree_compute_time", pt_cloud_update_time);
  data.appendKeyData("avg_tree_update_time", pt_cloud_update_time/data.d_infoN);

  if (data.d_neighC.size() != data.d_x.size())
    data.d_neighC.resize(data.d_x.size());

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, data.d_x.size(), (std::size_t) 1,
                          [&data](std::size_t i) {

    const auto &pi = data.d_ptId[i];
    const auto &pi_particle = data.d_particlesListTypeAll[pi];

    // search?
    bool perform_search_based_on_particle = true;
    if (pi_particle->isWall()) // wall
      perform_search_based_on_particle = false;

    if (pi_particle->d_allDofsConstrained or !pi_particle->d_computeForce)
      perform_search_based_on_particle = false;

    if (perform_search_based_on_particle) {

      std::vector<size_t> neighs;
      std::vector<double> sqr_dist;

      data.d_neighC[i].clear();

      auto n = data.d_nsearch_p->radiusSearchExcludeTag(
              data.d_x[i],
              data.d_contNeighSearchRadius,
              neighs,
              sqr_dist,
              data.d_ptId[i],
              data.d_ptId);

      if (n > 0) {
        for (auto neigh: neighs) {
          if (neigh != i)
            data.d_neighC[i].push_back(neigh);
        }
      }
    }
}
  ); // for_each

  executor.run(taskflow).get();


  // handle particle-wall neighborlist (based on the data.d_neighC that we already computed)
  data.d_neighWallNodes.resize(data.d_particlesListTypeAll.size());
  data.d_neighWallNodesDistance.resize(data.d_particlesListTypeAll.size());
  data.d_neighWallNodesCondensed.resize(data.d_particlesListTypeAll.size());

  for (auto &pi : data.d_particlesListTypeParticle) {

    data.d_neighWallNodes[pi->getId()].resize(pi->getNumNodes());
    data.d_neighWallNodesDistance[pi->getId()].resize(pi->getNumNodes());

    // get all wall nodes that are within contact distance to the nodes of this particle
    {
      tf::Executor executor(util::parallel::getNThreads());
      tf::Taskflow taskflow;

      taskflow.for_each_index((std::size_t) 0,
                              pi->getNumNodes(),
                              (std::size_t) 1,
                              [&data, &pi](std::size_t i) {

            auto i_glob = pi->getNodeId(i);
            auto yi = data.d_x[i_glob];

            const std::vector<size_t> &neighs = data.d_neighC[i_glob];

            data.d_neighWallNodes[pi->getId()][i].clear();
            data.d_neighWallNodesDistance[pi->getId()][i].clear();

            for (const auto &j_id: neighs) {

              auto &ptIdj = data.d_ptId[j_id];
              auto &pj = data.getParticleFromAllList(
                      ptIdj);

              // we are only interested in nodes from wall
              if (pj->isWall()) {
                  data.d_neighWallNodes[pi->getId()][i].push_back(j_id);
                  //data.d_neighWallNodesDistance[pi->getId()][i].push_back(Rji);
              }
            }
        }
      ); // for_each

      executor.run(taskflow).get();
    }
  } // loop over particles


}

void contact::Contact::computeForces(model::ModelData &data) {


  model::log(data, "    Computing normal contact force \n", 3);

  // Description:
  // 1. Normal contact is applied between nodes of particles and walls
  // 2. Normal damping is applied between particle centers
  // 3. Normal damping is applied between nodes of particle and wall pairs

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0,
                          data.d_fContCompNodes.size(),
                          (std::size_t) 1,
                          [&data](std::size_t II) {

                              auto i = data.d_fContCompNodes[II];

                              // local variable to hold force
                              util::Point force_i = util::Point();
                              double scalar_f = 0.;

                              const auto &ptIdi = data.getPtId(i);
                              auto &pi = data.getParticleFromAllList(ptIdi);
                              double horizon = pi->d_material_p->getHorizon();
                              double search_r = data.d_maxContactR;

                              // particle data
                              double rhoi = pi->getDensity();

                              const auto &yi = data.d_x[i]; // current coordinates
                              const auto &ui = data.d_u[i];
                              const auto &vi = data.d_v[i];
                              const auto &voli = data.d_vol[i];

                              const std::vector<size_t> &neighs = data.d_neighC[i];

                              if (neighs.size() > 0) {

                                for (const auto &j_id: neighs) {

                                  //auto &j_id = neighs[j];
                                  const auto &yj = data.d_x[j_id]; // current coordinates
                                  double Rji = (yj - yi).length();
                                  auto &ptIdj = data.d_ptId[j_id];
                                  auto &pj = data.getParticleFromAllList(ptIdj);
                                  double rhoj = pj->getDensity();

                                  bool both_walls =
                                          (pi->isWall() and pj->isWall());

                                  if (j_id != i) {
                                    if (ptIdj != ptIdi && !both_walls) {

                                      // apply particle-particle or particle-wall contact here
                                      const auto &contact =
                                              data.d_particleDeck_p->d_contactDeck.getContact(pi->getGroupId("contact_id"), pj->getGroupId("contact_id"));

                                      if (util::isLess(Rji, contact.d_contactR)) {

                                        auto yji = data.d_x[j_id] - yi;
                                        auto volj = data.d_vol[j_id];
                                        auto vji = data.d_v[j_id] - vi;

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
                                                  std::sqrt(contact.d_K * contact.d_contactR * meq);

                                          auto &pii = data.d_particlesListTypeAll[pi->getId()];
                                          vji = data.d_v[j_id] - pii->getVCenter();
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

                              data.d_f[i] += force_i;
                          }
  ); // for_each

  executor.run(taskflow).get();


  // damping force
  model::log(data, "    Computing normal damping force \n", 3);
  for (auto &pi : data.d_particlesListTypeParticle) {

    auto pi_id = pi->getId();

    double Ri = pi->d_geom_p->boundingRadius();
    double vol_pi = M_PI * Ri * Ri;
    auto pi_xc = pi->getXCenter();
    auto pi_vc = pi->getVCenter();
    auto rhoi = pi->getDensity();
    util::Point force_i = util::Point();

    // particle-particle
    for (auto &pj : data.d_particlesListTypeParticle) {
      if (pj->getId() != pi->getId()) {
        auto Rj = pj->d_geom_p->boundingRadius();
        auto xc_ji = pj->getXCenter() - pi_xc;
        auto dist_xcji = xc_ji.length();

        const auto &contact = data.d_particleDeck_p->d_contactDeck.getContact(pi->getGroupId("contact_id"), pj->getGroupId("contact_id"));

        if (util::isLess(dist_xcji, Rj + Ri + 1.01 * contact.d_contactR)) {

          auto vol_pj = M_PI * Rj * Rj;
          auto rhoj = pj->getDensity();
          // equivalent mass
          auto meq = util::equivalentMass(rhoi * vol_pi, rhoj * vol_pj);

          // beta_n
          auto beta_n = contact.d_betan *
                        std::sqrt(contact.d_K * contact.d_contactR * meq);

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
    data.d_neighWallNodesCondensed[pi->getId()].clear();
    {
      for (size_t j=0; j<data.d_neighWallNodes[pi_id].size(); j++) {

        const auto &j_id = pi->getNodeId(j);
        const auto &yj = data.d_x[j_id];

        for (size_t k=0; k<data.d_neighWallNodes[pi_id][j].size(); k++) {

          const auto &k_id = data.d_neighWallNodes[pi_id][j][k];
          const auto &pk = data.d_particlesListTypeAll[data.d_ptId[k_id]];

          double Rjk = (data.d_x[k_id] - yj).length();

          const auto &contact =
              data.d_particleDeck_p->d_contactDeck.getContact(pi->getGroupId("contact_id"), pk->getGroupId("contact_id"));

          if (util::isLess(Rjk, contact.d_contactR))
            util::methods::addToList(k_id, data.d_neighWallNodesCondensed[pi_id]);

        } // loop over k
      } // loop over j
    } // step 2

    // now loop over wall nodes and add force to center of particle
    for (auto &j : data.d_neighWallNodesCondensed[pi_id]) {

      auto &ptIdj = data.d_ptId[j];
      auto &pj = data.d_particlesListTypeAll[ptIdj];
      auto rhoj = pj->getDensity();
      auto volj = data.d_vol[j];
      auto meq = rhoi * vol_pi;
      //auto meq = util::equivalentMass(rhoi * vol_pi, rhoj * volj);

      const auto &contact
              = data.d_particleDeck_p->d_contactDeck.getContact(pi->getGroupId("contact_id"), pj->getGroupId("contact_id"));

      // beta_n
      auto beta_n = contact.d_betan *
                    std::sqrt(contact.d_K * contact.d_contactR * meq);

      // center-node vector
      auto xc_ji = data.d_x[j] - pi_xc;
      auto hat_xc_ji = util::Point();
      if (util::isGreater(xc_ji.length(), 0.))
        hat_xc_ji = xc_ji / xc_ji.length();

      // center-node velocity
      auto vc_ji = data.d_v[j] - pi_vc;
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
                              [&data, pi, force_i](std::size_t i) {
                                  data.d_f[pi->getNodeId(i)] += force_i;
                              }
      ); // for_each

      executor.run(taskflow).get();
    }
  } // loop over particle for damping

}
