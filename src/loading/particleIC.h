/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef LOADING_PARTILCEIC_H
#define LOADING_PARTILCEIC_H

#include "util/point.h"     // definition of Point
#include "inp/bcBaseDeck.h"

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
   * @brief Applies displacement initial condition
   * @param time Current time
   * @param particle Particle object pointer
   * @param icVec Initial condition data
   */
  void applyIC(particle::BaseParticle *particle, const std::vector<inp::BCBaseDeck> &icVec);

} // namespace loading

#endif // LOADING_PARTILCEIC_H
