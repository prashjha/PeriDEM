/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
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
#include <cstdint>
#include <mutex>
#include <unordered_map>

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
 * Spring force density uses neighbor volume Vj (optionally partial-volume
 * corrected by assembly). Default friction is coulomb_simple
 * (mu * |Fn| along tangential velocity).
 */
class PairForce {
public:
  virtual ~PairForce() = default;

  virtual void beginStep() {}
  virtual void endStep() {}

  /*! Spring + friction contribution (force density ∝ volj). */
  virtual util::Point springForce(const Pair &p);

  /*! Node-level normal damping (density form). */
  virtual util::Point nodeDampingForce(const Pair &p);

  /*! Default: spring + node damping (used by tests / simple callers). */
  virtual util::Point force(const Pair &p);
};

/*!
 * Stick-slip tangential friction: incremental tangential spring with stiffness
 * Kn, capped by mu * |Fn|. History is per directed pair (i,j).
 */
class StickSlipPairForce : public PairForce {
public:
  void beginStep() override;
  void endStep() override;
  util::Point springForce(const Pair &p) override;

private:
  struct Hist {
    util::Point delta_t;
    std::size_t stamp = 0;
  };
  static std::uint64_t key(std::size_t i, std::size_t j) {
    return (static_cast<std::uint64_t>(i) << 32) ^
           static_cast<std::uint64_t>(j);
  }
  std::unordered_map<std::uint64_t, Hist> d_hist;
  std::size_t d_stamp = 0;
  std::mutex d_mutex;
};

} // namespace contact

#endif // CONTACT_PAIRFORCE_H
