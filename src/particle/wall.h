/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef PARTICLE_WALL_H
#define PARTICLE_WALL_H

#include "model/modelData.h"
#include "baseParticle.h"
#include "fe/mesh.h"
#include "inp/pdecks/particleDeck.h"
#include <string.h> // size_t type
#include <vector>

namespace particle {

/*! @brief Specialization of BaseParticle for modeling of walls. Wall differ
 * from the particles in the sense that they do not have any reference
 * particle and they have their own mesh object.
 */
class Wall : public BaseParticle {

public:
  /*!
   * @brief Constructor
   *
   * @param id Id of this object in group of all base particles
   * @param wall_id Wall id of this object in group of walls
   * @param z_deck Wall zone input deck
   * @param z_id Zone id this object belongs to
   * @param mesh Mesh of this object
   * @param model_data Global model data
   * @param populate_data Modify global model data to add the properties of
   * this object
   */
  Wall(size_t id, size_t wall_id, inp::WallZone &z_deck, size_t z_id,
       fe::Mesh *mesh, std::shared_ptr<model::ModelData> model_data,
       bool populate_data = true);

  /*!
   * @brief Get pointer to mesh object
   * @return mesh Pointer to mesh
   */
  const fe::Mesh *getMeshP() const { return d_mesh_p.get(); };
  fe::Mesh *getMeshP() { return d_mesh_p.get(); };

  /*!
   * @brief Get reference to mesh object
   * @return mesh Reference to mesh
   */
  const fe::Mesh &getMesh() const { return *d_mesh_p; };
  fe::Mesh &getMesh() { return *d_mesh_p; };

  /*!
   * @brief Get id of this wall
   * @return id Wall id
   */
  size_t getWallId() const {return getTypeId(); }

public:
  /*! @brief Wall type */
  std::string d_wallType;

  /*! @brief Pointer to mesh on reference particle */
  std::unique_ptr<fe::Mesh> d_mesh_p;
};

} // namespace particle

#endif // PARTICLE_WALL_H
