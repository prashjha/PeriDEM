/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "pairForce.h"

#include "util/io.h"
#include "inp/contactPairDeck.h"
#include "util/function.h"

#include <cmath>

util::Point contact::PairForce::force(const Pair &p) {
  const auto yji = p.yj - p.yi;
  const auto Rji = yji.length();
  if (!(Rji > 0.) || !util::isLess(Rji, p.deck.d_contactR))
    return {};

  const auto vji = p.vj - p.vi;
  auto en = yji / Rji;
  auto vn_mag = vji * en;
  auto et = vji - vn_mag * en;
  if (util::isGreater(et.length(), 0.))
    et = et / et.length();
  else
    et = util::Point();

  auto scalar_f = p.deck.d_Kn * (Rji - p.deck.d_contactR) * p.volj;
  if (scalar_f > 0.)
    scalar_f = 0.;

  util::Point f = scalar_f * en;
  if (p.deck.d_frictionOn)
    f += p.deck.d_mu * scalar_f * et;

  // Node-level damping (Jha 2021 Eq. 22). Original demModel had this behind
  // node_lvl_damp=false and relied on COM damping only; stiff node contact
  // then rings into the grain (T11). Apply when Damping_On.
  if (p.deck.d_dampingOn && util::isLess(vn_mag, 0.) && p.voli > 0. &&
      p.deck.d_K > 0. && p.deck.d_contactR > 0.) {
    const double meq =
        util::equivalentMass(p.rhoi * p.voli, p.rhoj * p.volj);
    const double beta_n =
        p.deck.d_betan *
        std::sqrt(p.deck.d_K * p.deck.d_contactR * meq);
    f += (beta_n * vn_mag / p.voli) * en;
  }

  return f;
}
