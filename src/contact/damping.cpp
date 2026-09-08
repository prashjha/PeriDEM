/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "damping.h"

#include "data/modelData.h"
#include "util/io.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "util/point.h"
#include "inp/input.h"

#include <cmath>
#include <format>

void contact::Damping::apply(data::ModelData &data) {
  util::io::log(3, "    Computing normal damping force \n");
  for (auto &pi : data.d_particlesListTypeParticle) {

    if (!pi->d_computeForce)
      continue;

    auto pi_id = pi->getId();

    double Ri = pi->d_geom_p->boundingRadius();
    double vol_pi = M_PI * Ri * Ri;
    auto pi_xc = pi->getXCenter();
    auto pi_vc = pi->getVCenter();
    auto rhoi = pi->getDensity();
    util::Point force_i = util::Point();

    for (auto &pj : data.d_particlesListTypeParticle) {
      if (pj->getId() != pi->getId()) {
        auto Rj = pj->d_geom_p->boundingRadius();
        auto xc_ji = pj->getXCenter() - pi_xc;
        auto dist_xcji = xc_ji.length();

        const auto &contact = data.d_particleDeck_p->d_contactDeck.getContact(pi->getGroupId("contact_id"), pj->getGroupId("contact_id"));

        if (!contact.d_dampingOn)
          continue;

        if (util::isLess(dist_xcji, Rj + Ri + 1.01 * contact.d_contactR)) {

          auto vol_pj = M_PI * Rj * Rj;
          auto rhoj = pj->getDensity();
          auto meq = util::equivalentMass(rhoi * vol_pi, rhoj * vol_pj);

          auto beta_n = contact.d_betan *
                        std::sqrt(contact.d_K * contact.d_contactR * meq);

          auto hat_xc_ji = util::Point();
          if (util::isGreater(dist_xcji, 0.))
            hat_xc_ji = xc_ji / dist_xcji;
          else
            hat_xc_ji = util::Point();

          auto vc_ji = pj->getVCenter() - pi_vc;
          auto vc_mag = vc_ji * hat_xc_ji;
          if (vc_mag > 0.)
            vc_mag = 0.;

          force_i += beta_n * vc_mag * hat_xc_ji / vol_pi;
        }
      }
    }

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

        }
      }
    }

    for (auto &j : data.d_neighWallNodesCondensed[pi_id]) {

      auto &ptIdj = data.d_ptId[j];
      auto &pj = data.d_particlesListTypeAll[ptIdj];
      auto meq = rhoi * vol_pi;

      const auto &contact
              = data.d_particleDeck_p->d_contactDeck.getContact(pi->getGroupId("contact_id"), pj->getGroupId("contact_id"));

      if (!contact.d_dampingOn)
        continue;

      auto beta_n = contact.d_betan *
                    std::sqrt(contact.d_K * contact.d_contactR * meq);

      auto xc_ji = data.d_x[j] - pi_xc;
      auto hat_xc_ji = util::Point();
      if (util::isGreater(xc_ji.length(), 0.))
        hat_xc_ji = xc_ji / xc_ji.length();

      auto vc_ji = data.d_v[j] - pi_vc;
      auto vc_mag = vc_ji * hat_xc_ji;
      if (vc_mag > 0.)
        vc_mag = 0.;

      force_i += beta_n * vc_mag * hat_xc_ji / vol_pi;
    }

    for (size_t i = 0; i < pi->getNumNodes(); i++)
      data.d_f[pi->getNodeId(i)] += force_i;
  }
}
