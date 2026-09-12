/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "selfContact.h"

#include <stdexcept>

namespace pd {

util::Point BrokenBondKnSelfContact::force(const util::Point &yji, double volj,
                                           double Kn, double Rc) const {
  const double Rji = yji.length();
  if (!(Rji > 0.))
    return util::Point();

  double gap = Rji - Rc;
  if (gap > 0.)
    gap = 0.;
  else {
    // Cap penetration so a bond that breaks while already deep inside Rc
    // cannot inject a discontinuous Kn*(R-Rc) kick.
    const double gap_cap = -0.25 * Rc;
    if (gap < gap_cap)
      gap = gap_cap;
  }

  const double scalar_f = Kn * volj * gap / Rji;
  return scalar_f * yji;
}

std::unique_ptr<SelfContact> makeSelfContact(const std::string &name) {
  if (name == "broken_bond_kn")
    return std::make_unique<BrokenBondKnSelfContact>();

  throw std::runtime_error(
      "Unknown Model.Self_Contact '" + name +
      "'. Supported: broken_bond_kn.");
}

} // namespace pd
