/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
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
      [&data, abs_stretch_break, dim](std::size_t II) {
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

          // dilatation: 3/m in 3D, 2/m in 2D (Yang et al. 2024, eqs. 8-9)
          data.d_thetaX[i] = double(dim) * theta / m;
        } // if it is state-based
      } // loop over nodes
    ); // for_each

    executor.run(taskflow).get();

    // Ghost nodes need owner dilatations before the force pass.
    pd::exchangeGhostTheta(data);
  }

  // strain energy density per node
  if (data.d_e.size() != data.d_x.size())
    data.d_e.resize(data.d_x.size(), 0.f);

  // compute the internal forces
  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1,
    [&data, selfContact = selfContact.get(), abs_stretch_break](std::size_t II) {
      auto i = data.d_fPdCompNodes[II];

      // local variable to hold force
      util::Point force_i = util::Point();
      double scalar_f = 0.;
      double energy_i = 0.;

      // for damage
      float Zi = 0.;
      // Silling (2000) damage: phi = 1 - ∫ mu dV' / ∫ dV' over the horizon
      // (reported by Silling 2003 and Trask). Bhattacharya & Lipton instead
      // define damage as the broken-bond count fraction, kept separately.
      double vol_all = 0.;
      double vol_intact = 0.;
      size_t n_bonds = 0;
      size_t n_broken = 0;

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
              energy_i += ef_i.first * volj;

              force_i += scalar_f * pi->d_material_p->getBondForceDirection(
                                        xj - xi, uj - ui);
            } // if state-based
            else {

              // Bond-based materials (e.g. PMB) historically broke inside
              // getBondEF using |s|>Sc. Honor Model.Bond_Break here instead:
              // tension (default, literature PMB) vs absolute_stretch.
              const double Sc = pi->d_material_p->getBreakSc(rji);
              const bool broke = abs_stretch_break
                                     ? util::isGreater(std::abs(Sji), Sc + 1.0e-10)
                                     : util::isGreater(Sji, Sc + 1.0e-10);
              if (broke)
                fs = true;
              data.d_fracture_p->setBondState(i, k, fs);

              if (!fs) {
                auto ef =
                    pi->d_material_p->getBondEF(rji, Sji, fs, /*break_bonds=*/false);
                scalar_f = ef.second * volj;
                energy_i += ef.first * volj;
                force_i += scalar_f * pi->d_material_p->getBondForceDirection(
                                          xj - xi, uj - ui);
              } else {
                const auto yji = xj + uj - (xi + ui);
                force_i +=
                    selfContact->force(yji, volj, pi->d_Kn, pi->d_Rc, rji);
              }
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

          // damage function phi (bond-state weighted by neighbor volume)
          vol_all += data.d_vol[j];
          n_bonds++;
          if (!data.d_fracture_p->getBondState(i, k))
            vol_intact += data.d_vol[j];
          else
            n_broken++;

          k++;
        } // loop over neighbors

      } // peridynamic force

      // update force (we remove any force from
      // previous steps and add peridynamics force)
      data.d_f[i] = force_i;
      if (pi->d_material_p->isStateActive())
        energy_i += pi->d_material_p->getDilatationEnergyDensity(thetai);
      data.d_e[i] = static_cast<float>(energy_i);

      data.d_Z[i] = Zi;
      if (!data.d_phi.empty())
        data.d_phi[i] =
            (vol_all > 0.) ? static_cast<float>(1. - vol_intact / vol_all) : 0.f;
      if (!data.d_phiBond.empty())
        data.d_phiBond[i] =
            (n_bonds > 0) ? static_cast<float>(double(n_broken) / double(n_bonds))
                          : 0.f;
    }
  ); // for_each

  executor.run(taskflow).get();

  // total strain energy over the nodes computed here
  double te = 0.;
  for (auto i : data.d_fPdCompNodes)
    te += double(data.d_e[i]) * data.d_vol[i];
  data.d_te = static_cast<float>(te);
}
