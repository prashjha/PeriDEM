/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef CONTACT_PAIRFORCE_H
#define CONTACT_PAIRFORCE_H

#include "util/point.h"
#include <cstddef>

namespace inp {
struct ContactPairDeck;
}

namespace contact {

/*! @brief One node-node contact pair. Assembly fills this; the law uses it. */
struct Pair {
  const inp::ContactPairDeck &deck;
  util::Point yi, yj, vi, vj;
  std::size_t i = 0;
  std::size_t j = 0;
  std::size_t pt_i = 0;
  std::size_t pt_j = 0;
  double voli = 0.;
  double volj = 0.;
  double rhoi = 0.;
  double rhoj = 0.;
  double dt = 0.;
  bool wall_i = false;
  bool wall_j = false;
};

/*!
 * Node-node contact law (same role as material::Material for PD bonds).
 *
 * Contact walks neighbors and calls force(). Subclass this to change the
 * relation; put extra parameters and history on the subclass. Do not copy
 * the neighbor loop.
 */
class PairForce {
public:
  virtual ~PairForce() = default;

  virtual util::Point force(const Pair &p);
};

} // namespace contact

#endif // CONTACT_PAIRFORCE_H
