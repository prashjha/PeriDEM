/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PARTICLE_PARTICLE_H
#define PARTICLE_PARTICLE_H

#include "baseParticle.h"
#include "fe/mesh.h"
#include "geometry/fracture.h"
#include "inp/pdecks/particleDeck.h"
#include "refParticle.h"
#include "util/geomObjects.h"
#include "util/matrix.h" // definition of Matrix3
#include "util/point.h"  // definition of Point
#include "util/transformation.h"

#include <cstring> // size_t type
#include <vector>

/*! @brief Collection of methods and data related to particle object */
namespace particle {

/*! @brief A class to store particle geometry, nodal discretization, and methods
 *
 * On top of this class, specialized class representing particle and wall are
 * built.
 */
class Particle : public BaseParticle {

public:
  /*!
   * @brief Constructor
   *
   * @param id Id of this object in group of all base particles
   * @param z_deck Zone input deck
   * @param particle_zone Id of zone this particle belongs to
   * @param ref_particle Pointer to reference particle
   * @param geom Geometrical object representing this object
   * @param transform Transform object to be applied to reference particle to
   * get this particle
   * @param model_data Global model data
   * @param populate_data Modify global model data to add the properties of
   * this object
   */
  Particle(size_t id, inp::ParticleZone &z_deck,
           size_t particle_zone,
           std::shared_ptr<particle::RefParticle> ref_particle,
           std::shared_ptr<util::geometry::GeomObject> geom,
           const particle::ParticleTransform &transform,
           std::shared_ptr<model::ModelData> model_data,
           bool populate_data = true);

  /*!
   * @brief Get current coordinate of center node
   * @return x Current coordinate
   */
  util::Point &getXCenter() {
    return d_modelData_p->getX(d_globStart + d_rp_p->getCenterNodeId());
  };

  /*! @copydoc getXCenter() */
  const util::Point &getXCenter() const {
    return d_modelData_p->getX(d_globStart + d_rp_p->getCenterNodeId());
  };

  /*!
   * @brief Get displacement of center node
   * @return u Displacement
   */
  util::Point &getUCenter() {
    return d_modelData_p->getU(d_globStart + d_rp_p->getCenterNodeId());
  };

  /*! @copydoc getUCenter() */
  const util::Point &getUCenter() const {
    return d_modelData_p->getU(d_globStart + d_rp_p->getCenterNodeId());
  };

  /*!
   * @brief Get velocity of center node
   * @return v Velocity
   */
  util::Point &getVCenter() {
    return d_modelData_p->getV(d_globStart + d_rp_p->getCenterNodeId());
  };

  /*! @copydoc getVCenter() */
  const util::Point &getVCenter() const {
    return d_modelData_p->getV(d_globStart + d_rp_p->getCenterNodeId());
  };

public:
  /*! @brief Pointer to reference particle */
  std::shared_ptr<particle::RefParticle> d_rp_p;

  /*! @brief Geometrical object defining this particle */
  std::shared_ptr<util::geometry::GeomObject> d_geom_p;

  /*! @brief Transformation related data */
  particle::ParticleTransform d_tform;
};

} // namespace particle

#endif // PARTICLE_PARTICLE_H
