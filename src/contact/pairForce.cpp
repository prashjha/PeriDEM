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

double contact::correctedContactVolume(double volj, double Rji, double Rc,
                                       double h) {
  if (!(volj > 0.) || !(Rc > 0.) || !(h > 0.) || !(Rji > 0.))
    return volj;

  const double check_up = Rc + 0.5 * h;
  const double check_low = Rc - 0.5 * h;
  if (util::isGreater(Rji, check_low)) {
    volj *= (check_up - Rji) / h;
    if (volj < 0.)
      volj = 0.;
  }
  return volj;
}

util::Point contact::PairForce::springForce(const Pair &p) {
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
  return f;
}

util::Point contact::PairForce::nodeDampingForce(const Pair &p) {
  const auto yji = p.yj - p.yi;
  const auto Rji = yji.length();
  if (!(Rji > 0.) || !util::isLess(Rji, p.deck.d_contactR))
    return {};

  if (!p.deck.d_dampingOn || !(p.voli > 0.) || !(p.deck.d_K > 0.) ||
      !(p.deck.d_contactR > 0.))
    return {};

  const auto vji = p.vj - p.vi;
  auto en = yji / Rji;
  auto vn_mag = vji * en;
  if (!util::isLess(vn_mag, 0.))
    return {};

  const double meq =
      util::equivalentMass(p.rhoi * p.voli, p.rhoj * p.volj);
  const double beta_n =
      p.deck.d_betan * std::sqrt(p.deck.d_K * p.deck.d_contactR * meq);
  return (beta_n * vn_mag / p.voli) * en;
}

util::Point contact::PairForce::force(const Pair &p) {
  return springForce(p) + nodeDampingForce(p);
}

void contact::StickSlipPairForce::beginStep() { ++d_stamp; }

void contact::StickSlipPairForce::endStep() {
  std::lock_guard<std::mutex> lock(d_mutex);
  for (auto it = d_hist.begin(); it != d_hist.end();) {
    if (it->second.stamp != d_stamp)
      it = d_hist.erase(it);
    else
      ++it;
  }
}

util::Point contact::StickSlipPairForce::springForce(const Pair &p) {
  const auto yji = p.yj - p.yi;
  const auto Rji = yji.length();
  if (!(Rji > 0.) || !util::isLess(Rji, p.deck.d_contactR))
    return {};

  auto en = yji / Rji;
  auto scalar_f = p.deck.d_Kn * (Rji - p.deck.d_contactR) * p.volj;
  if (scalar_f > 0.)
    scalar_f = 0.;

  util::Point f = scalar_f * en;
  if (!p.deck.d_frictionOn)
    return f;

  const double fn_mag = -scalar_f; // scalar_f <= 0 in contact
  if (!(fn_mag > 0.) || !(p.dt > 0.))
    return f;

  const auto vji = p.vj - p.vi;
  auto vt = vji - (vji * en) * en;

  util::Point ft_trial;
  {
    std::lock_guard<std::mutex> lock(d_mutex);
    auto &hist = d_hist[key(p.i, p.j)];
    hist.stamp = d_stamp;
    hist.delta_t += vt * p.dt;
    // Keep tangential history in the current tangent plane.
    hist.delta_t -= (hist.delta_t * en) * en;

    // Tangential stiffness taken equal to normal Kn (density form × volj).
    const double Kt = p.deck.d_Kn;
    const double kt_vol = Kt * p.volj;
    ft_trial = hist.delta_t * (-kt_vol);
    const double ft_mag = ft_trial.length();
    const double ft_max = p.deck.d_mu * fn_mag;

    if (util::isGreater(ft_mag, ft_max) && ft_mag > 0.) {
      ft_trial *= (ft_max / ft_mag);
      if (kt_vol > 0.)
        hist.delta_t = ft_trial * (-1.0 / kt_vol);
    }
  }

  f += ft_trial;
  return f;
}
