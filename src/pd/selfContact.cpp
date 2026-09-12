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

namespace {

util::Point cappedRepulsive(const util::Point &yji, double volj, double Kn,
                            double natural_R) {
  const double Rji = yji.length();
  if (!(Rji > 0.) || !(natural_R > 0.))
    return util::Point();

  double gap = Rji - natural_R;
  if (gap > 0.)
    gap = 0.;
  else {
    // Cap penetration so a bond that breaks while already deep inside
    // natural_R cannot inject a discontinuous Kn*(R-natural_R) kick.
    const double gap_cap = -0.25 * natural_R;
    if (gap < gap_cap)
      gap = gap_cap;
  }

  const double scalar_f = Kn * volj * gap / Rji;
  return scalar_f * yji;
}

} // namespace

namespace pd {

util::Point BrokenBondKnSelfContact::force(const util::Point &yji, double volj,
                                           double Kn, double Rc,
                                           double /*r0*/) const {
  return cappedRepulsive(yji, volj, Kn, Rc);
}

util::Point ReferenceGapSelfContact::force(const util::Point &yji, double volj,
                                           double Kn, double /*Rc*/,
                                           double r0) const {
  return cappedRepulsive(yji, volj, Kn, r0);
}

std::unique_ptr<SelfContact> makeSelfContact(const std::string &name) {
  if (name == "broken_bond_kn")
    return std::make_unique<BrokenBondKnSelfContact>();
  if (name == "reference_gap")
    return std::make_unique<ReferenceGapSelfContact>();

  throw std::runtime_error(
      "Unknown Model.Self_Contact '" + name +
      "'. Supported: broken_bond_kn, reference_gap.");
}

} // namespace pd
