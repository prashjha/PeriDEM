/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PARTILCE_REFPARTILCE_H
#define PARTILCE_REFPARTILCE_H

#include "mesh/mesh.h"
#include "fracture/fracture.h"
#include "inp/particleDeck.h"
#include "material/mparticle/material.h"
#include "geom/geomIncludes.h"
#include "util/matrix.h" // definition of Matrix3
#include "util/point.h"  // definition of Point
#include "model/modelData.h"

#include <cstdint> // uint8_t type
#include <cstring> // string and size_t type
#include <vector>

/*! @brief Collection of methods and data related to particle object */
namespace particle {

/*! @brief A class to store reference particle related data. Consider a case
 * of multiple hexagon-shaped particle related to each other by affine
 * transformation. In such a case, it is possible to consider a reference
 * hexagon particle and store the mesh and other details of only the reference
 * particle. To get the data relevant to specific hexagon particle, one can
 * apply the appropriate transformation on data for the reference hexagon
 * particle.
 */
class RefParticle {
public:
  /*!
   * @brief Constructor
   * @param id Id of this object in list of all reference particles in ModelData
   * @param model_data Global model data
   * @param geom Particle geometry object
   * @param mesh Pointer to mesh
   */
  RefParticle(size_t id,
              std::shared_ptr<model::ModelData> model_data,
              std::shared_ptr<geom::GeomObject> geom,
              std::shared_ptr<mesh::Mesh> mesh);

  /**
   * @name Accessors
   */
  /**@{*/

  /*!
   * @brief Get pointer to mesh object
   * @return mesh Pointer to mesh
   */
  std::shared_ptr<mesh::Mesh> &getMeshP() { return d_mesh_p; };

  /*! @copydoc getMeshP() */
  const std::shared_ptr<mesh::Mesh> &getMeshP() const { return d_mesh_p; };

  /*!
   * @brief Get pointer to geometry object
   * @return pointer Pointer
   */
  std::shared_ptr<geom::GeomObject> &getGeomP() { return d_geom_p; };

  /*! @copydoc getGeomP() */
  const std::shared_ptr<geom::GeomObject> &getGeomP() const { return d_geom_p; };

  /*!
   * @brief Get reference to mesh object
   * @return mesh Reference to mesh
   */
  mesh::Mesh &getMesh() { return *d_mesh_p; };

  /*! @copydoc getMesh() */
  const mesh::Mesh &getMesh() const { return *d_mesh_p; };

  /*!
   * @brief Get the dimension of the domain
   * @return N Dimension
   */
  size_t getDimension() const {
    return d_mesh_p->getDimension();
  };

  /*!
   * @brief Get the number of nodes
   * @return N number of nodes
   */
  size_t getNumNodes() const {
    return d_mesh_p->getNumNodes();
  };

  /*!
   * @brief Get reference coordinate of a node
   * @param i Index of node
   * @return x Reference coordinate
   */
  util::Point getNode(const size_t &i) const {
    return d_mesh_p->getNode(i);
  };

  /*!
   * @brief Get nodal volume
   * @param i Index of node
   * @return volume Nodal volume
   */
  double getNodalVolume(const size_t &i) const {
    return d_mesh_p->getNodalVolume(i);
  };

  /*!
   * @brief Get id of center node of particle
   * @return Id Id of center node
   */
  size_t getCenterNodeId() const { return d_centerNode; };

  /*!
   * @brief Get radius of reference particle
   * @return Radius Radius of reference particle
   */
  double getParticleRadius() const { return d_pRadius; };

  /** @}*/

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt = 0, int lvl = 0) const;

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }

public:
  /*! @brief Id of reference particle in list d_referenceParticles in ModelData */
  size_t d_id;

  /*! @brief Reference to model class */
  std::shared_ptr<model::ModelData> d_modelData_p;

  /*! @brief Pointer to mesh on reference particle */
  std::shared_ptr<mesh::Mesh> d_mesh_p;

  /*! @brief Id of mesh node closest to the particle center */
  size_t d_centerNode;

  /*! @brief Geometrical object defining this particle */
  std::shared_ptr<geom::GeomObject> d_geom_p;

  /*! @brief Particle radius */
  double d_pRadius;

  /*! @brief List of nodes near boundary */
  std::vector<size_t> d_bNodes;

  /*!
   * @brief Interior flags. For given node i the flag is d_intFlags[i%8]. We
   * use 1 bit per node.
   */
  std::vector<uint8_t> d_intFlags;
};

} // namespace particle

#endif // PARTILCE_REFPARTILCE_H
