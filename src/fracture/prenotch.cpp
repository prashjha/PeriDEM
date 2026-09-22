/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "prenotch.h"

#include <stdexcept>

namespace {

/*! Breaks the bonds of one particle for which the predicate is true. */
template <typename Predicate>
std::size_t breakBondsIf(
    geometry::Fracture &fracture, const std::vector<util::Point> &xRef,
    const std::vector<std::vector<std::size_t>> &neighbors,
    const std::vector<std::size_t> &nodeParticleId, std::size_t particleId,
    Predicate crosses) {
  if (neighbors.size() > xRef.size() ||
      nodeParticleId.size() < neighbors.size())
    throw std::invalid_argument(
        "prenotch: node arrays are inconsistent with the neighbour list");

  std::size_t broken = 0;
  for (std::size_t i = 0; i < neighbors.size(); ++i) {
    if (nodeParticleId[i] != particleId)
      continue;
    const auto &xi = xRef[i];
    for (std::size_t k = 0; k < neighbors[i].size(); ++k) {
      const std::size_t j = neighbors[i][k];
      if (j >= nodeParticleId.size() || nodeParticleId[j] != particleId)
        continue;
      const auto &xj = xRef[j];
      if (crosses(xi.d_x, xi.d_y, xj.d_x, xj.d_y)) {
        fracture.setBondState(i, k, true);
        ++broken;
      }
    }
  }
  return broken;
}

} // namespace

namespace geometry {

bool segmentCrossesVerticalLine(double x0, double y0, double x1, double y1,
                                double xLine, double yLo, double yHi) {
  // The endpoints lie on opposite sides of the line. This excludes x1 == x0,
  // so the division is defined and the parameter lies in (0, 1).
  if ((x0 - xLine) * (x1 - xLine) >= 0.)
    return false;
  const double t = (xLine - x0) / (x1 - x0);
  const double y = y0 + t * (y1 - y0);
  return y >= yLo && y <= yHi;
}

bool segmentEntersSlot(double x0, double y0, double x1, double y1,
                       double xCenter, double halfWidth, double yLo,
                       double yHi, int nSamples) {
  const double xa = xCenter - halfWidth, xb = xCenter + halfWidth;
  auto inSlot = [&](double x, double y) {
    return x >= xa && x <= xb && y >= yLo && y <= yHi;
  };
  if (inSlot(x0, y0) || inSlot(x1, y1))
    return true;
  if (nSamples > 0) {
    for (int s = 0; s <= nSamples; ++s) {
      const double t = static_cast<double>(s) / nSamples;
      if (inSlot(x0 + t * (x1 - x0), y0 + t * (y1 - y0)))
        return true;
    }
  }
  // A slot narrower than the sample spacing can be crossed with no sample
  // inside it.
  return segmentCrossesVerticalLine(x0, y0, x1, y1, xCenter, yLo, yHi);
}

std::size_t breakBondsCrossingVerticalLines(
    Fracture &fracture, const std::vector<util::Point> &xRef,
    const std::vector<std::vector<std::size_t>> &neighbors,
    const std::vector<std::size_t> &nodeParticleId, std::size_t particleId,
    const std::vector<double> &xLines, double yLo, double yHi) {
  return breakBondsIf(
      fracture, xRef, neighbors, nodeParticleId, particleId,
      [&](double x0, double y0, double x1, double y1) {
        for (double xl : xLines)
          if (segmentCrossesVerticalLine(x0, y0, x1, y1, xl, yLo, yHi))
            return true;
        return false;
      });
}

std::size_t breakBondsInSlots(
    Fracture &fracture, const std::vector<util::Point> &xRef,
    const std::vector<std::vector<std::size_t>> &neighbors,
    const std::vector<std::size_t> &nodeParticleId, std::size_t particleId,
    const std::vector<double> &xCenters, double width, double yLo,
    double yHi) {
  if (width < 0.)
    throw std::invalid_argument("prenotch: slot width must not be negative");
  const double hw = 0.5 * width;
  return breakBondsIf(
      fracture, xRef, neighbors, nodeParticleId, particleId,
      [&](double x0, double y0, double x1, double y1) {
        for (double xc : xCenters)
          if (segmentEntersSlot(x0, y0, x1, y1, xc, hw, yLo, yHi))
            return true;
        return false;
      });
}

} // namespace geometry
