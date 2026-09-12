/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PD_SELFCONTACT_H
#define PD_SELFCONTACT_H

#include "util/point.h"

#include <memory>
#include <string>

namespace pd {

/*! @brief Intra-body force between nodes when the PD bond is inactive. */
class SelfContact {
public:
  virtual ~SelfContact() = default;

  /*!
   * Force contribution on node i from neighbor j (broken / unbound pair).
   * @param yji Current relative position xj+uj - (xi+ui)
   * @param volj Neighbor nodal volume
   * @param Kn Contact stiffness
   * @param Rc Contact radius for this particle (used by broken_bond_kn)
   * @param r0 Reference distance |xj - xi| (used by reference_gap)
   */
  virtual util::Point force(const util::Point &yji, double volj, double Kn,
                            double Rc, double r0) const = 0;
};

/*! @brief Default: Kn * volj * capped_gap / R along yji with gap = R - Rc. */
class BrokenBondKnSelfContact : public SelfContact {
public:
  util::Point force(const util::Point &yji, double volj, double Kn, double Rc,
                    double r0) const override;
};

/*!
 * Repulsive only when current distance is shorter than the reference
 * distance r0 (compressed relative to the undeformed pair). Cap like the
 * default law.
 */
class ReferenceGapSelfContact : public SelfContact {
public:
  util::Point force(const util::Point &yji, double volj, double Kn, double Rc,
                    double r0) const override;
};

/*! @brief Build self-contact law named by Model.Self_Contact. */
std::unique_ptr<SelfContact> makeSelfContact(const std::string &name);

} // namespace pd

#endif // PD_SELFCONTACT_H
