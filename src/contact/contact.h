/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef CONTACT_CONTACT_H
#define CONTACT_CONTACT_H

#include "damping.h"
#include "pairForce.h"
#include "wallContact.h"

#include <memory>

namespace data {
class ModelData;
}

namespace contact {

/*! @brief Contact neighbor search and force assembly. Pair law and damping
 *  are separate types on this object. */
class Contact {
public:
  Contact();
  virtual ~Contact() = default;

  void setup(data::ModelData &data);
  bool updateSearchParameters(data::ModelData &data);
  void updateNeighborlist(data::ModelData &data);
  void computeForces(data::ModelData &data);

  void setPairForce(std::unique_ptr<PairForce> p) { d_pairForce = std::move(p); }
  void setDamping(std::unique_ptr<Damping> d) { d_damping = std::move(d); }
  void setWallContact(std::unique_ptr<WallContact> w) {
    d_wallContact = std::move(w);
  }

  std::unique_ptr<PairForce> d_pairForce;
  std::unique_ptr<Damping> d_damping;
  std::unique_ptr<WallContact> d_wallContact;
  /*! @brief Apply node-level damping inside the pair assembly loop. */
  bool d_useNodeDamping = true;
};

} // namespace contact

#endif // CONTACT_CONTACT_H
