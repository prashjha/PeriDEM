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

/*! @brief Build pair-force object from friction-law name. */
std::unique_ptr<PairForce> makePairForce(const std::string &friction_law);

/*! @brief Build COM damping object, or nullptr when the law has no COM term. */
std::unique_ptr<Damping> makeDamping(const std::string &name);

/*! @brief True for com / com_and_node. */
bool usesComDamping(const std::string &damping_law);

/*! @brief True for node / com_and_node. */
bool usesNodeDamping(const std::string &damping_law);

} // namespace contact

#endif // CONTACT_POLICY_H
