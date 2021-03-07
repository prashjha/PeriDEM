/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
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
   * @param deck Input deck which contains user-specified information
   * @param mesh Mesh object
   */
  ParticleFLoading(std::vector<inp::PBCData> &bc_data);

  /*!
   * @brief Applies displacement boundary condition
   * @param time Current time
   * @param f Vector nodal forces
   * @param mesh Mesh object
   */
  void apply(const double &time, particle::BaseParticle *particle);
  void applyParticle(const double &time, particle::BaseParticle *particle);
  void applyWall(const double &time, particle::BaseParticle *wall);
};

} // namespace loading

#endif // LOADING_PARTILCE_FLOADING_H
