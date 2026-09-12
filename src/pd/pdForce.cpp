/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "pdForce.h"
#include "selfContact.h"

#include <memory>
#include <string>
#include "pdMpi.h"

#include "data/modelData.h"
#include "util/io.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/point.h"
#include "util/parallelUtil.h"

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

void pd::computeForces(data::ModelData &data) {


  util::io::log(3, "    Computing peridynamic force \n");

  const auto dim = data.d_modelDeck_p->d_dim;
  const bool is_state = data.d_particlesListTypeAll[0]->getMaterial()->isStateActive();
  const auto selfContact =
      pd::makeSelfContact(data.d_modelDeck_p->d_selfContact);
  const bool abs_stretch_break =
      (data.d_modelDeck_p->d_bondBreak == "absolute_stretch");

  // compute state-based helper quantities
  if (is_state) {

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1,
      [&data, abs_stretch_break](std::size_t II) {
        auto i = data.d_fPdCompNodes[II];

        const auto rho = data.getDensity(i);
        const auto &fix = data.d_fix[i];
        const auto &ptId = data.getPtId(i);
        auto &pi = data.getParticleFromAllList(ptId);

        if (pi->d_material_p->isStateActive()) {

          const double horizon = pi->getHorizon();
          const double mesh_size = pi->getMeshSize();
          const auto &xi = data.d_xRef[i];
          const auto &ui = data.d_u[i];

          // update bond state and compute thetax
          const auto &m = data.d_mX[i];
          double theta = 0.;

          // upper and lower bound for volume correction
          auto check_up = horizon + 0.5 * mesh_size;
          auto check_low = horizon - 0.5 * mesh_size;

          size_t k = 0;
          for (size_t j : data.d_neighPd[i]) {

            const auto &xj = data.d_xRef[j];
            const auto &uj = data.d_u[j];
            double rji = (xj - xi).length();
            if (!(rji > 0.)) {
              k += 1;
              continue;
            }
            // double rji = std::sqrt(data.d_neighPdSqdDist[i][k]);
            double change_length = (xj - xi + uj - ui).length() - rji;

            // step 1: update the bond state
            double s = change_length / rji;
            double sc = pi->d_material_p->getSc(rji);

            // get fracture state, modify, and set
            auto fs = data.d_fracture_p->getBondState(i, k);
            // Model.Bond_Break: tension (s > sc) or absolute_stretch (|s| > sc).
            if (!fs) {
              const bool broke = abs_stretch_break
                                     ? util::isGreater(std::abs(s), sc + 1.0e-10)
                                     : util::isGreater(s, sc + 1.0e-10);
              if (broke)
                fs = true;
            }
            data.d_fracture_p->setBondState(i, k, fs);

            if (!fs) {

              // get corrected volume of node j
              auto volj = data.d_vol[j];

              if (util::isGreater(rji, check_low))
                volj *= (check_up - rji) / mesh_size;

              theta += rji * change_length * pi->d_material_p->getInfFn(rji) *
                        volj;
            } // if bond is not broken

            k += 1;
          } // loop over neighbors

          data.d_thetaX[i] = 3. * theta / m;
        } // if it is state-based
      } // loop over nodes
    ); // for_each

    executor.run(taskflow).get();

    // Ghost nodes need owner dilatations before the force pass.
    pd::exchangeGhostTheta(data);
  }

  // compute the internal forces
  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1,
    [&data, selfContact = selfContact.get()](std::size_t II) {
      auto i = data.d_fPdCompNodes[II];

      // local variable to hold force
      util::Point force_i = util::Point();
      double scalar_f = 0.;

      // for damage
      float Zi = 0.;

      const auto rhoi = data.getDensity(i);
      const auto &ptIdi = data.getPtId(i);
      auto &pi = data.getParticleFromAllList(ptIdi);

      const double horizon = pi->getHorizon();
      const double mesh_size = pi->getMeshSize();
      const auto &xi = data.d_xRef[i];
      const auto &ui = data.d_u[i];
      const auto &mi = data.d_mX[i];
      const auto &thetai = data.d_thetaX[i];

      // upper and lower bound for volume correction
      auto check_up = horizon + 0.5 * mesh_size;
      auto check_low = horizon - 0.5 * mesh_size;

      // loop over neighbors
      {
        size_t k = 0;
        for (size_t j : data.d_neighPd[i]) {
          auto fs = data.d_fracture_p->getBondState(i, k);
          const auto &xj = data.d_xRef[j];
          const auto &uj = data.d_u[j];
          auto volj = data.d_vol[j];
          double rji = (xj - xi).length();
          if (!(rji > 0.)) {
            k++;
            continue;
          }
          double Sji = pi->d_material_p->getS(xj - xi, uj - ui);

          if (!fs) {
            const auto &mj = data.d_mX[j];
            const auto &thetaj = data.d_thetaX[j];

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
              data.d_fracture_p->setBondState(i, k, fs);

              // compute the contribution of bond force to force at i
              scalar_f = ef.second * volj;

              force_i += scalar_f * pi->d_material_p->getBondForceDirection(
                                        xj - xi, uj - ui);
            } // if bond-based
          }   // if bond not broken
          else {
            const auto yji = xj + uj - (xi + ui);
            force_i +=
                selfContact->force(yji, volj, pi->d_Kn, pi->d_Rc, rji);
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
      data.d_f[i] = force_i;

      data.d_Z[i] = Zi;
    }
  ); // for_each

  executor.run(taskflow).get();

}
