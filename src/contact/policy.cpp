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

std::unique_ptr<PairForce> makePairForce(const std::string &friction_law) {
  if (friction_law != "coulomb_simple" && friction_law != "stick_slip") {
    throw std::runtime_error(
        "Unknown Contact.Friction_Law '" + friction_law +
        "'. Supported: coulomb_simple, stick_slip.");
  }

  if (friction_law == "stick_slip")
    return std::make_unique<StickSlipPairForce>();
  return std::make_unique<PairForce>();
}

std::unique_ptr<Damping> makeDamping(const std::string &name) {
  if (name == "com_and_node" || name == "com")
    return std::make_unique<Damping>();
  if (name == "node" || name == "off")
    return nullptr;
  throw std::runtime_error(
      "Unknown Contact.Damping_Law '" + name +
      "'. Supported: com_and_node, com, node, off.");
}

bool usesComDamping(const std::string &damping_law) {
  return damping_law == "com" || damping_law == "com_and_node";
}

bool usesNodeDamping(const std::string &damping_law) {
  return damping_law == "node" || damping_law == "com_and_node";
}

} // namespace contact
