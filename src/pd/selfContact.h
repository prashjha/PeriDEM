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
   * @param Rc Contact radius for this particle
   */
  virtual util::Point force(const util::Point &yji, double volj, double Kn,
                            double Rc) const = 0;
};

/*! @brief Default: Kn * volj * capped_gap / R along yji. */
class BrokenBondKnSelfContact : public SelfContact {
public:
  util::Point force(const util::Point &yji, double volj, double Kn,
                    double Rc) const override;
};

/*! @brief Build self-contact law named by Model.Self_Contact. */
std::unique_ptr<SelfContact> makeSelfContact(const std::string &name);

} // namespace pd

#endif // PD_SELFCONTACT_H
