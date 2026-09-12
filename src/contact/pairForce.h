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
 * Partial volume of neighbor j near the contact radius (same idea as the
 * horizon correction in the PD force loop). Full volume when R is well inside
 * Rc - h/2; linearly tapers to zero by Rc + h/2.
 */
double correctedContactVolume(double volj, double Rji, double Rc, double h);

/*!
 * Node-node contact law (same role as material::Material for PD bonds).
 *
 * Spring uses neighbor volume Vj (optionally corrected by assembly). For
 * Pair_Law volume_product, Contact multiplies the spring sum by Vi after the
 * neighbor loop. Node damping stays a density term and is not scaled by Vi.
 */
class PairForce {
public:
  virtual ~PairForce() = default;

  /*! Spring + friction contribution (∝ volj). */
  virtual util::Point springForce(const Pair &p);

  /*! Node-level normal damping (density form). */
  virtual util::Point nodeDampingForce(const Pair &p);

  /*! Default: spring + node damping (used by tests / simple callers). */
  virtual util::Point force(const Pair &p);
};

/*!
 * Marker law for Contact.Pair_Law = volume_product. Spring kernel matches
 * volume_j (∝ Vj); assembly applies × Vi after the neighbor loop.
 */
class VolumeProductPairForce : public PairForce {};

} // namespace contact

#endif // CONTACT_PAIRFORCE_H
