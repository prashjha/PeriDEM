/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "wallContact.h"

#include "data/modelData.h"
#include "geom/geomObjects.h"
#include "inp/contactPairDeck.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/parallelUtil.h"

#include <cmath>
#include <stdexcept>
#include <taskflow/taskflow/algorithm/for_each.hpp>
#include <taskflow/taskflow/taskflow.hpp>

namespace contact {

void MeshedWallContact::apply(data::ModelData &, PairForce *, bool) const {}

void AnalyticalPlaneWallContact::apply(data::ModelData &data, PairForce *,
                                       bool use_node_damping) const {
  if (data.d_particlesListTypeWall.empty())
    return;

  const unsigned n_workers = util::parallel::getNThreads();
  tf::Executor executor(n_workers);
  tf::Taskflow taskflow;

  taskflow.for_each_index(
      (std::size_t)0, data.d_fContCompNodes.size(), (std::size_t)1,
      [&data, use_node_damping](std::size_t II) {
        const auto i = data.d_fContCompNodes[II];
        const auto &ptIdi = data.getPtId(i);
        auto &pi = data.getParticleFromAllList(ptIdi);
        if (pi->isWall())
          return;

        const auto &yi = data.d_x[i];
        const auto &vi = data.d_v[i];
        const double voli = data.d_vol[i];
        util::Point force_i;

        for (auto *wall : data.d_particlesListTypeWall) {
          if (!wall || !wall->d_geom_p)
            continue;

          geom::WallContactHit hit;
          if (!wall->d_geom_p->wallContactQuery(yi, hit) || !hit.active)
            continue;

          const auto &contact =
              data.d_particleDeck_p->d_contactDeck.getContact(
                  pi->getGroupId("contact_id"),
                  wall->getGroupId("contact_id"));

          // Signed gap from geom (positive = free space). Contact when gap < Rc.
          const double R = hit.signed_gap;
          if (!util::isLess(R, contact.d_contactR))
            continue;

          // en points from grain toward wall (into the material).
          const util::Point en = -1. * hit.outward_n;
          auto scalar_f = contact.d_Kn * (R - contact.d_contactR) * voli;
          if (scalar_f > 0.)
            scalar_f = 0.;

          util::Point f = scalar_f * en;

          if (contact.d_frictionOn) {
            const util::Point vji = -1. * vi; // wall velocity ~ 0
            const double vn = vji * en;
            util::Point et = vji - vn * en;
            if (util::isGreater(et.length(), 0.))
              et = et / et.length();
            else
              et = util::Point();
            f += contact.d_mu * scalar_f * et;
          }

          if (use_node_damping && contact.d_dampingOn && voli > 0. &&
              contact.d_K > 0. && contact.d_contactR > 0.) {
            const util::Point vji = -1. * vi;
            const double vn = vji * en;
            if (util::isLess(vn, 0.)) {
              const double meq =
                  util::equivalentMass(pi->getDensity() * voli,
                                       wall->getDensity() * voli);
              const double beta_n =
                  contact.d_betan *
                  std::sqrt(contact.d_K * contact.d_contactR * meq);
              f += (beta_n * vn / voli) * en;
            }
          }

          force_i += f;
        }

        data.d_f[i] += force_i;
      });

  executor.run(taskflow).get();
}

std::unique_ptr<WallContact> makeWallContact(const std::string &name) {
  if (name == "meshed")
    return std::make_unique<MeshedWallContact>();
  if (name == "analytical_plane")
    return std::make_unique<AnalyticalPlaneWallContact>();
  throw std::runtime_error(
      "Unknown Model.Wall_Contact '" + name +
      "'. Supported: meshed, analytical_plane.");
}

} // namespace contact
