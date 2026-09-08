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
#include "damping.h"

#include "data/modelData.h"
#include "util/io.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/matrix.h"
#include "util/vecMethods.h"
#include "util/point.h"
#include "util/parallelUtil.h"
#include "inp/input.h"

#include <cmath>
#include <format>
#include <memory>
#include <vector>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

contact::Contact::Contact()
    : d_pairForce(std::make_unique<PairForce>()),
      d_damping(std::make_unique<Damping>()) {}

void contact::Contact::setup(data::ModelData &data) {


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

  util::io::log(1, std::format("{}: Contact setup\n  hmin = {:.6f}, hmax = {:.6f} \n",
                  data.d_name, data.d_hMin, data.d_hMax));

  data.d_maxContactR = 0.;

  auto &contactDeck = data.d_particleDeck_p->d_contactDeck;

  // Paper κ_eff and the original DEMModel setup: pair bulk modulus from the
  // two contact groups' materials. JSON Contact.K is optional (usually absent).
  std::vector<double> bulk(contactDeck.d_data.size(), -1.);
  for (const auto *p : data.d_particlesListTypeAll) {
    if (p->getMaterial() == nullptr)
      continue;
    const auto cid = p->getGroupId("contact_id");
    if (cid >= bulk.size())
      continue;
    bulk[cid] = p->getMaterial()->computeMaterialProperties(data.dimension()).d_K;
  }

  for (size_t i = 0; i < contactDeck.d_data.size(); i++) {
    for (size_t j = 0; j < contactDeck.d_data.size(); j++) {

      inp::ContactPairDeck *deck = &(contactDeck.d_data[i][j]);

      if (deck->d_computeContactR)
        deck->d_contactR *= data.d_hMin;

      if (data.d_maxContactR < deck->d_contactR)
        data.d_maxContactR = deck->d_contactR;

      if (bulk[i] > 0. && bulk[j] > 0.)
        deck->d_K = util::equivalentMass(bulk[i], bulk[j]);

      // Kn
      deck->d_Kn *= deck->d_KnFactor;

      // Beta n
      double log_e = std::log(deck->d_eps);
      deck->d_betan =
          deck->d_betanFactor *
          (-2. * log_e * std::sqrt(1. / (M_PI * M_PI + log_e * log_e)));

      util::io::log(1, std::format("  contact_radius = {:.6f}, hmin = {:.6f}, Kn = {:5.3e}, "
                      "Vmax = {:5.3e}, "
                      "betan = {:7.5f}, mu = {:.4f}, kappa = {:5.3e}\n",
                      deck->d_contactR, data.d_hMin, deck->d_Kn, deck->d_vMax,
                      deck->d_betan, deck->d_mu, deck->d_K));
    }
  }

}

bool contact::Contact::updateSearchParameters(data::ModelData &data) {


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
  if (!std::isfinite(data.d_maxVelocity) || data.d_maxVelocity < 0.)
    data.d_maxVelocity = 0.;
  auto max_search_r = data.d_maxVelocity * data.d_currentDt
                      * data.d_particleDeck_p->d_pNeighDeck.d_neighUpdateInterval
                      * safety_factor;
  if (!std::isfinite(max_search_r) || max_search_r < 0.)
    max_search_r = 0.;


  if (util::isGreater(max_search_r, max_search_r_from_contact_R )) {

    data.d_contNeighUpdateInterval = size_t(data.d_maxContactR/(data.d_maxVelocity * data.d_currentDt));
    if (up_interval_old > data.d_contNeighUpdateInterval) {
      // issue warning
      util::io::log(2, std::format("Warning: Contact search radius based on velocity is greater than "
                      "the max contact radius.\n"
                      "Warning: Adjusting contact neighborlist update interval.\n"
                      "{:>13} = {:4.6e}, time step = {}, "
                      "velocity-based r = {:4.6e}, max contact r = {:4.6e}\n",
                      "Time", data.d_time, data.d_n, max_search_r, max_search_r_from_contact_R), data.d_n % data.d_infoN == 0, 3);
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
    util::io::log(2, std::format("    Contact neighbor parameters: \n"
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
                    "max velocity", data.d_maxVelocity), data.d_n % data.d_infoN == 0, 3);
  }

  // update counter and return condition for contact search
  data.d_contNeighTimestepCounter++;
  return (data.d_contNeighTimestepCounter - 1) % data.d_contNeighUpdateInterval == 0;

}

void contact::Contact::updateNeighborlist(data::ModelData &data) {


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

  {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index((std::size_t) 0, data.d_x.size(), (std::size_t) 1,
                            [&data](std::size_t i) {

      if (data.d_contNeighSearchRadius <= 0. ||
          !std::isfinite(data.d_contNeighSearchRadius))
        return;

      const auto &pi = data.d_ptId[i];
      const auto &pi_particle = data.d_particlesListTypeAll[pi];

      // Walls still search: contact reaction on the plate/cup needs neighC on
      // wall nodes. Skip only when forces are not computed on this body.
      bool perform_search_based_on_particle = true;
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
    }); // for_each

    executor.run(taskflow).get();
  }


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

void contact::Contact::computeForces(data::ModelData &data) {

  util::io::log(3, "    Computing normal contact force \n");

  auto *pair = d_pairForce.get();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0,
                          data.d_fContCompNodes.size(),
                          (std::size_t) 1,
                          [&data, pair](std::size_t II) {

                              auto i = data.d_fContCompNodes[II];

                              util::Point force_i = util::Point();

                              const auto &ptIdi = data.getPtId(i);
                              auto &pi = data.getParticleFromAllList(ptIdi);

                              const auto &yi = data.d_x[i];
                              const auto &vi = data.d_v[i];
                              const std::vector<size_t> &neighs = data.d_neighC[i];

                              for (const auto &j_id: neighs) {
                                if (j_id == i)
                                  continue;

                                const auto &ptIdj = data.d_ptId[j_id];
                                if (ptIdj == ptIdi)
                                  continue;

                                auto &pj = data.getParticleFromAllList(ptIdj);
                                if (pi->isWall() and pj->isWall())
                                  continue;

                                const auto &contact =
                                    data.d_particleDeck_p->d_contactDeck.getContact(
                                        pi->getGroupId("contact_id"),
                                        pj->getGroupId("contact_id"));

                                Pair p{contact,
                                       yi, data.d_x[j_id],
                                       vi, data.d_v[j_id],
                                       i, j_id,
                                       ptIdi, ptIdj,
                                       data.d_vol[i], data.d_vol[j_id],
                                       pi->getDensity(), pj->getDensity(),
                                       data.d_currentDt,
                                       pi->isWall(), pj->isWall()};
                                force_i += pair->force(p);
                              }

                              data.d_f[i] += force_i;
                          }
  );

  executor.run(taskflow).get();

  if (d_damping)
    d_damping->apply(data);
}
