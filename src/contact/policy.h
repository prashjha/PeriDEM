/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef CONTACT_POLICY_H
#define CONTACT_POLICY_H

#include "damping.h"
#include "pairForce.h"

#include <memory>
#include <string>

namespace contact {

/*! @brief Build the pair-force implementation named by the contact deck. */
std::unique_ptr<PairForce> makePairForce(const std::string &name);

/*! @brief Build the damping implementation named by the contact deck. */
std::unique_ptr<Damping> makeDamping(const std::string &name);

} // namespace contact

#endif // CONTACT_POLICY_H
