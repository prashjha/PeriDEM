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

#include <cstdlib>
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

namespace pd {
/*!
 * Jha et al. JMPS 2021 Eq. (11): h(s)=1 iff s < s0 (tension-only break).
 * Legacy PeriDEM used |s| > sc (also breaks in compression). That hands a
 * deeply penetrated pair to the broken-bond Kn contact branch and detonates.
 * Set PERIDEM_ABS_STRETCH_BREAK=1 to restore the legacy |s| criterion.
 */
bool absStretchBreak() {
  static const bool flag = [] {
    const char *e = std::getenv("PERIDEM_ABS_STRETCH_BREAK");
    return e != nullptr && std::string(e) == "1";
  }();
  return flag;
}
} // namespace pd

void pd::computeForces(data::ModelData &data) {


  util::io::log(3, "    Computing peridynamic force \n");

  const auto dim = data.d_modelDeck_p->d_dim;
  const bool is_state = data.d_particlesListTypeAll[0]->getMaterial()->isStateActive();

  // compute state-based helper quantities
  if (is_state) {

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1, [&data](std::size_t II) {
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
            // Paper: break when s >= sc (tension only). Legacy |s|>sc also
            // breaks in compression and can detonate via broken-bond contact.
            if (!fs) {
              const bool broke = pd::absStretchBreak()
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
    (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1, [&data](std::size_t II) {
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
            // Broken-bond contact (intra-grain). Cap penetration so a bond
            // that somehow breaks while already deep inside Rc cannot inject
            // a discontinuous Kn*(R-Rc) kick (T11 C2).
            auto yji = xj + uj - (xi + ui);
            auto Rji = yji.length();
            if (!(Rji > 0.)) {
              k++;
              continue;
            }
            double gap = Rji - pi->d_Rc;
            if (gap > 0.)
              gap = 0.;
            else {
              const double gap_cap = -0.25 * pi->d_Rc;
              if (gap < gap_cap)
                gap = gap_cap;
            }
            scalar_f = pi->d_Kn * volj * gap / Rji;
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
      data.d_f[i] = force_i;

      data.d_Z[i] = Zi;
    }
  ); // for_each

  executor.run(taskflow).get();

}
