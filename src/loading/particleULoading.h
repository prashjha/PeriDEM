/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef LOADING_PARTILCE_ULOADING_H
#define LOADING_PARTILCE_ULOADING_H

#include "particleLoading.h"        // base class Loading
#include "util/point.h"     // definition of Point

// forward declaration
namespace fe {
class Mesh;
}

namespace particle {
class BaseParticle;
}

namespace model {
class ModelData;
}

namespace loading {

/*!
 * @brief A class to apply displacement boundary condition
 */
class ParticleULoading : public ParticleLoading {

public:
  /*!
   * @brief Constructor
   * @param bc_data Boundary condition data
   */
  ParticleULoading(std::vector<inp::BCBaseDeck> &bc_data);

  /*!
   * @brief Sets fixity mask
   * @param particle Particle object pointer
   */
  void setFixity(particle::BaseParticle *particle);

  /*!
   * @brief Applies displacement boundary condition
   * @param time Current time
   * @param particle Particle object pointer
   */
  void apply(const double &time, particle::BaseParticle *particle);

  /*! @brief Flag to indicate whether particles are fixed */
  std::vector<bool> d_pZeroDisplacementApplied;
};

} // namespace loading

#endif // LOADING_PARTILCE_ULOADING_H
