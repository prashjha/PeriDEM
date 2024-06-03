/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MATERIAL_UTIL_H
#define MATERIAL_UTIL_H

#include "util/point.h"
#include "mparticle/material.h"
#include "geometry/fracture.h"
#include "model/modelData.h"
#include <limits>
#include <string>
#include <vector>

namespace material {

/*!
   * @brief Computes the moment \f$ m_x \f$ term in state-based peridynamic
   * formulation
   */
void computeStateMx(model::ModelData *model, bool compute_in_parallel = false);

void computeStateThetax(model::ModelData *model, bool compute_in_parallel = false);


void computeHydrostaticStrain(model::ModelData *model, bool compute_in_parallel = false);

void updateBondFractureData(model::ModelData *model, bool compute_in_parallel = false);

} // namespace material

#endif // MATERIAL_UTIL_H
