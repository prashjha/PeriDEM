/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
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
   * @param deck Input deck which contains user-specified information
   * @param mesh Mesh object
   */
  ParticleULoading(std::vector<inp::PBCData> &bc_data);

  /*!
   * @brief Applies displacement boundary condition
   * @param time Current time
   * @param u Vector nodal displacements
   * @param v Vector nodal velocities
   * @param mesh Mesh object
   */
  void setFixityParticle(particle::BaseParticle *particle);
  void setFixityWall(particle::BaseParticle *wall);
  void setFixity(particle::BaseParticle *wall);


  /*!
   * @brief Applies displacement boundary condition
   * @param time Current time
   * @param u Vector nodal displacements
   * @param v Vector nodal velocities
   * @param mesh Mesh object
   */
  void applyParticle(const double &time, particle::BaseParticle *particle);
  void applyWall(const double &time, particle::BaseParticle *wall);
  void apply(const double &time, particle::BaseParticle *wall);

  std::vector<bool> d_pZeroDisplacementApplied;
  std::vector<bool> d_wZeroDisplacementApplied;
};

} // namespace loading

#endif // LOADING_PARTILCE_ULOADING_H
