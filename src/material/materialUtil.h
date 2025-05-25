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
#include "fracture/fracture.h"
#include "model/modelData.h"
#include <limits>
#include <string>
#include <vector>

namespace material {

     /**
   * @name Conversion methods
   */
  /**@{*/

  /*!
   * @brief Compute Poisson's ratio from Lame parameters
   * @param lambda Lame first parameter
   * @param mu Lame second parameter
   * @return nu Poisson's ratio
   */
  inline double toNu(double lambda, double mu) { return lambda * 0.5 / (lambda + mu); }

  /*!
   * @brief Compute Poisson's ratio from Young's and Shear modulus
   * @param E Youngs modulus
   * @param G Shear modulus
   * @return nu Poisson's ratio
   */
  inline double toNuEG(double E, double G) { return E * 0.5 / G - 1.; }

  /*!
   * @brief Compute Young's modulus E from Bulk modulus K and Poisson's ratio nu
   * @param K Bulk modulus
   * @param nu Poisson's ratio
   * @return E Young's modulus
   */
  inline double toE(double K, double nu) { return K * (3. * (1. - 2. * nu)); }

  /*!
   * @brief Compute Bulk modulus K from Young's modulus K and Poisson's ratio nu
   * @param E Young's modulus
   * @param nu Poisson's ratio
   * @return K Bulk modulus
   */
  inline double toK(double E, double nu) { return E / (3. * (1. - 2. * nu)); }

  /*!
   * @brief Compute Lame first parameter lambda from Young's modulus E
   * and Poisson's ratio nu
   * @param E Young's modulus
   * @param nu Poisson's ratio
   * @return lambda Lame first parameter
   */
  inline double toLambdaE(double E, double nu) {
    return E * nu / ((1. + nu) * (1. - 2. * nu));
  }

  /*!
   * @brief Compute Lame first parameter lambda from Bulk modulus K and
   * Poisson's ratio nu
   * @param K Bulk modulus
   * @param nu Poisson's ratio
   * @return lambda Lame first parameter
   */
  inline double toLambdaK(double K, double nu) { return 3. * K * nu / (1. + nu); }

  /*!
   * @brief Compute shear modulus from Young's modulus E and Poisson's ratio nu
   * @param E Young's modulus
   * @param nu Poisson's ratio
   * @return G Shear modulus
   */
  inline double toGE(double E, double nu) { return E / (2. * (1. + nu)); }

  /*!
   * @brief Compute shear modulus from Bulk modulus K and Poisson's ratio nu
   * @param K Bulk modulus
   * @param nu Poisson's ratio
   * @return G Shear modulus
   */
  inline double toGK(double K, double nu) {
    return 3. * K * (1. - 2. * nu) / (2. * (1. + nu));
  }

  /*!
   * @brief Compute Young's modulus E from Lame first parameter lambda and
   * Poisson's ratio nu
   * @param lambda Lame first parameter
   * @param nu Poisson's ratio
   * @return E Young's modulus
   */
  inline double toELambda(double lambda, double nu) {
    return lambda * (1. + nu) * (1. - 2. * nu) / nu;
  }

  /*!
   * @brief Compute critical energy release rate Gc from critical
   * stress-intensity factor KIc, Poisson's ratio nu, and Young's modulus E
   *
   * Below conversion from KIc to Gc assumes **plane-stress** condition. For
   * **plane-stress** condition, we need to modify the Young's modulus \f$
   * E\f$ to \f$ \frac{E}{1 - \nu^2} \f$ where \f$ \nu\f$ is the Poisson's
   * ratio.
   * @param KIc Critical stress-intensity factor
   * @param nu Poisson's ratio
   * @param E Young's modulus
   * @return Gc Critical energy release rate
   */
  inline double toGc(double KIc, double nu, double E) { return KIc * KIc / E; }

  /*!
   * @brief Compute critical stress-intensity factor KIc from critical energy
   * release rate Gc, Poisson's ratio \f$ nu\f$, and Young's modulus E
   *
   * Below conversion from Gc to KIc assumes **plane-stress** condition. For
   * **plane-stress** condition, we need to modify the Young's modulus \f$
   * E\f$ to \f$ \frac{E}{1 - \nu^2} \f$ where \f$ \nu\f$ is the Poisson's
   * ratio.
   * @param Gc Critical energy release rate
   * @param nu Poisson's ratio
   * @param E Young's modulus
   * @return KIc Critical stress-intensity factor
   */
  inline double toKIc(double Gc, double nu, double E) { return std::sqrt(Gc * E); }

  /** @}*/

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
