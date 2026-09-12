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

std::unique_ptr<PairForce> makePairForce(const std::string &pair_law,
                                         const std::string &friction_law) {
  if (pair_law != "volume_j" && pair_law != "volume_product") {
    throw std::runtime_error(
        "Unknown Contact.Pair_Law '" + pair_law +
        "'. Supported: volume_j, volume_product.");
  }
  if (friction_law != "coulomb_simple" && friction_law != "stick_slip") {
    throw std::runtime_error(
        "Unknown Contact.Friction_Law '" + friction_law +
        "'. Supported: coulomb_simple, stick_slip.");
  }

  // Stick-slip owns the spring+friction kernel; volume_product is applied in
  // Contact assembly (× Vi after the neighbor loop).
  if (friction_law == "stick_slip")
    return std::make_unique<StickSlipPairForce>();
  if (pair_law == "volume_product")
    return std::make_unique<VolumeProductPairForce>();
  return std::make_unique<PairForce>();
}

std::unique_ptr<Damping> makeDamping(const std::string &name) {
  if (name == "com_and_node")
    return std::make_unique<Damping>();

  throw std::runtime_error(
      "Unknown Contact.Damping_Law '" + name +
      "'. Supported: com_and_node.");
}

} // namespace contact
