/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef CONTACT_WALLCONTACT_H
#define CONTACT_WALLCONTACT_H

#include "pairForce.h"

#include <memory>
#include <string>

namespace data {
class ModelData;
}

namespace contact {

/*!
 * @brief Optional analytical wall-contact path (Model.Wall_Contact).
 *
 * Meshed mode is a no-op here — grain–wall pairs stay in the node-node loop.
 * Analytical mode applies geom::GeomObject::wallContactQuery forces and expects
 * the assembly loop to skip meshed grain–wall pairs.
 */
class WallContact {
public:
  virtual ~WallContact() = default;

  virtual void apply(data::ModelData &data, PairForce *pair,
                     bool use_node_damping) const = 0;

  /*! True when meshed grain–wall pairs must be skipped in Contact::computeForces. */
  virtual bool skipsMeshedGrainWall() const { return false; }
};

class MeshedWallContact : public WallContact {
public:
  void apply(data::ModelData &data, PairForce *pair, bool use_node_damping)
      const override;
};

class AnalyticalPlaneWallContact : public WallContact {
public:
  void apply(data::ModelData &data, PairForce *pair, bool use_node_damping)
      const override;
  bool skipsMeshedGrainWall() const override { return true; }
};

std::unique_ptr<WallContact> makeWallContact(const std::string &name);

} // namespace contact

#endif // CONTACT_WALLCONTACT_H
