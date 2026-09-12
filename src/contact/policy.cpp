/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "policy.h"

#include <stdexcept>

namespace contact {

std::unique_ptr<PairForce> makePairForce(const std::string &name) {
  if (name == "volume_j")
    return std::make_unique<PairForce>();
  if (name == "volume_product")
    return std::make_unique<VolumeProductPairForce>();

  throw std::runtime_error(
      "Unknown Contact.Pair_Law '" + name +
      "'. Supported: volume_j, volume_product.");
}

std::unique_ptr<Damping> makeDamping(const std::string &name) {
  if (name == "com_and_node")
    return std::make_unique<Damping>();

  throw std::runtime_error(
      "Unknown Contact.Damping_Law '" + name +
      "'. Supported: com_and_node.");
}

} // namespace contact
