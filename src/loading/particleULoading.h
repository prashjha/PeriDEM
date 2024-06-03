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
   * @param bc_data Boundary condition data
   */
  ParticleULoading(std::vector<inp::PBCData> &bc_data);

  /*!
   * @brief Sets fixity mask
   * @param particle Particle object pointer
   */
  void setFixityParticle(particle::BaseParticle *particle);

  /*!
   * @brief Sets fixity mask
   * @param wall Wall object pointer
   */
  void setFixityWall(particle::BaseParticle *wall);

  /*!
   * @brief Sets fixity mask
   * @param wall Wall object pointer
   */
  void setFixity(particle::BaseParticle *wall);


  /*!
   * @brief Applies displacement boundary condition
   * @param time Current time
   * @param particle Particle object pointer
   */
  void applyParticle(const double &time, particle::BaseParticle *particle);

  /*!
   * @brief Applies displacement boundary condition
   * @param time Current time
   * @param wall Wall object pointer
   */
  void applyWall(const double &time, particle::BaseParticle *wall);

  /*!
   * @brief Applies displacement boundary condition
   * @param time Current time
   * @param wall Wall object pointer
   */
  void apply(const double &time, particle::BaseParticle *wall);

  /*! @brief Flag to indicate whether particles are fixed */
  std::vector<bool> d_pZeroDisplacementApplied;

  /*! @brief Flag to indicate whether particles are fixed */
  std::vector<bool> d_wZeroDisplacementApplied;
};

} // namespace loading

#endif // LOADING_PARTILCE_ULOADING_H
