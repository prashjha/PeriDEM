/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef FRACTURE_PRENOTCH_H
#define FRACTURE_PRENOTCH_H

#include "fracture.h"
#include "util/point.h"

#include <cstddef>
#include <vector>

namespace geometry {

/*!
 * @brief Pre-notch by breaking peridynamic bonds
 *
 * Removing the notch from the mesh does not by itself open it. The horizon is
 * wider than the notch in the cases here, so bonds still span the gap and carry
 * force across it. These functions break those bonds, after the model is
 * initialised and before the time loop starts.
 *
 * They take the node data rather than a model object, so that src/ does not
 * depend on the driver. Call them with data.d_xRef, data.d_neighPd,
 * data.d_ptId and *data.d_fracture_p.
 */

/*!
 * @brief Checks whether the segment (x0,y0)-(x1,y1) crosses the vertical line
 * x = xLine within [yLo, yHi]
 *
 * @param x0 First endpoint x
 * @param y0 First endpoint y
 * @param x1 Second endpoint x
 * @param y1 Second endpoint y
 * @param xLine Position of the cut line
 * @param yLo Lower end of the cut
 * @param yHi Upper end of the cut
 * @return bool True if the segment crosses the cut
 */
bool segmentCrossesVerticalLine(double x0, double y0, double x1, double y1,
                                double xLine, double yLo, double yHi);

/*!
 * @brief Checks whether the segment enters the open slot centred on xCenter
 *
 * The slot spans xCenter +/- halfWidth and yLo to yHi. The segment is sampled
 * at nSamples interior points, so that a bond with an endpoint inside the slot
 * is detected and not only one that crosses the slot entirely.
 *
 * @param x0 First endpoint x
 * @param y0 First endpoint y
 * @param x1 Second endpoint x
 * @param y1 Second endpoint y
 * @param xCenter Slot centreline
 * @param halfWidth Half of the slot width
 * @param yLo Lower end of the slot
 * @param yHi Upper end of the slot
 * @param nSamples Number of interior samples along the segment
 * @return bool True if the segment enters the slot
 */
bool segmentEntersSlot(double x0, double y0, double x1, double y1,
                       double xCenter, double halfWidth, double yLo,
                       double yHi, int nSamples = 8);

/*!
 * @brief Break the bonds of one particle that cross vertical cut lines
 *
 * This is the zero-width pre-notch: a cut along each line in @p xLines between
 * @p yLo and @p yHi. A V-notch seeded on its midplane is the same geometric
 * test and uses this function.
 *
 * Only bonds with both ends in particle @p particleId are considered.
 *
 * @param fracture Bond-state store to modify
 * @param xRef Reference coordinates of all nodes
 * @param neighbors Peridynamic neighbour list
 * @param nodeParticleId Particle id of each node
 * @param particleId Particle to cut
 * @param xLines Positions of the cut lines
 * @param yLo Lower end of the cuts
 * @param yHi Upper end of the cuts
 * @return n Number of bonds broken
 */
std::size_t breakBondsCrossingVerticalLines(
    Fracture &fracture, const std::vector<util::Point> &xRef,
    const std::vector<std::vector<std::size_t>> &neighbors,
    const std::vector<std::size_t> &nodeParticleId, std::size_t particleId,
    const std::vector<double> &xLines, double yLo, double yHi);

/*!
 * @brief Break the bonds of one particle that enter open notch slots
 *
 * This is the finite-width pre-notch: each slot is @p width across, centred on
 * an entry of @p xCenters, spanning @p yLo to @p yHi.
 *
 * Only bonds with both ends in particle @p particleId are considered.
 *
 * @param fracture Bond-state store to modify
 * @param xRef Reference coordinates of all nodes
 * @param neighbors Peridynamic neighbour list
 * @param nodeParticleId Particle id of each node
 * @param particleId Particle to cut
 * @param xCenters Slot centrelines
 * @param width Full slot width
 * @param yLo Lower end of the slots
 * @param yHi Upper end of the slots
 * @return n Number of bonds broken
 */
std::size_t breakBondsInSlots(
    Fracture &fracture, const std::vector<util::Point> &xRef,
    const std::vector<std::vector<std::size_t>> &neighbors,
    const std::vector<std::size_t> &nodeParticleId, std::size_t particleId,
    const std::vector<double> &xCenters, double width, double yLo, double yHi);

} // namespace geometry

#endif // FRACTURE_PRENOTCH_H
