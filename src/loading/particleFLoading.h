/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef LOADING_PARTILCE_FLOADING_H
#define LOADING_PARTILCE_FLOADING_H

#include "particleLoading.h"        // base class Loading
#include "util/point.h"     // definition of Point

// forward declaration
namespace fe {
class Mesh;
}

namespace particle {
class BaseParticle;
}

namespace loading {

/*!
 * @brief A class to apply force boundary condition
 */
class ParticleFLoading : public ParticleLoading {

public:
  /*!
   * @brief Constructor
   * @param bc_data Boundary condition data
   */
  ParticleFLoading(std::vector<inp::PBCData> &bc_data);

  /*!
   * @brief Function that checks if given particle with id = id needs to be processed
   * within boundary condition data bc
   * @param id Id of particle in all particle list
   * @param bc Boundary condition data
   * @return bool True if particle should be processed further
   */
  bool needToProcessParticle(size_t id,
                             const inp::PBCData &bc);

  /*!
   * @brief Function that checks if we need to do computation at a given point x within a particle with id = id
   * @param x Coordinates of a point within particle (reference coordinate)
   * @param id Id of particle in all particle list
   * @param bc Boundary condition data
   * @return bool True if we compute at x
   */
  bool needToComputeDof(const util::Point &x,
                        size_t id,
                        const inp::PBCData &bc);

  /*!
   * @brief Applies force boundary condition
   * @param time Current time
   * @param particle Particle object pointer
   */
  void apply(const double &time, particle::BaseParticle *particle);
};

} // namespace loading

#endif // LOADING_PARTILCE_FLOADING_H
