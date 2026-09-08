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

util::Point contact::PairForce::force(const Pair &p) {
  const auto yji = p.yj - p.yi;
  const auto Rji = yji.length();
  if (!(Rji > 0.) || !util::isLess(Rji, p.deck.d_contactR))
    return {};

  const auto vji = p.vj - p.vi;
  auto en = yji / Rji;
  const auto vn_mag = vji * en;
  auto et = vji - vn_mag * en;
  if (util::isGreater(et.length(), 0.))
    et = et / et.length();
  else
    et = util::Point();

  auto scalar_f = p.deck.d_Kn * (Rji - p.deck.d_contactR) * p.volj;
  if (scalar_f > 0.)
    scalar_f = 0.;

  return scalar_f * en + p.deck.d_mu * scalar_f * et;
}
