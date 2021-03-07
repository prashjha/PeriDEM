/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef PARTILCE_REFPARTILCE_H
#define PARTILCE_REFPARTILCE_H

#include "fe/mesh.h"
#include "geometry/fracture.h"
#include "inp/pdecks/particleDeck.h"
#include "material/mparticle/material.h"
#include "util/geomObjects.h"
#include "util/matrix.h" // definition of Matrix3
#include "util/point.h"  // definition of Point
#include "util/transformation.h"

#include <cstring>      // size_t type
#include <vector>

/*! @brief Collection of methods and data related to particle object */
namespace particle {

/*! @brief A struct that stores transformation parameters and provides method
 * to transform the particle. Basically, given a reference particle, this
 * provides method to translate, rotate, and scale the reference particle.
 */
struct ParticleTransform {

  /*! @brief Translational vector */
  util::Point d_translation;

  /*! @brief Axis of rotation */
  util::Point d_axis;

  /*! @brief Angle of rotation */
  double d_theta;

  /*! @brief Volumetric scaling factor */
  double d_scale;

  /*!
   * @brief Constructor
   */
  ParticleTransform()
      : d_translation(util::Point()), d_axis(util::Point()),
        d_theta(0.), d_scale(1.){};

  /*!
   * @brief Constructor
   *
   * @param translate Translation vector
   * @param axis Axis of rotation
   * @param thetaa Angle of rotation
   * @param scale Volumetric scaling
   */
  ParticleTransform(util::Point translate, util::Point axis, double theta,
                    double scale = 1.)
      : d_translation(translate), d_axis(axis / axis.length()), d_theta(theta),
        d_scale(scale){};

  /*!
   * @brief Copy constructor
   */
  ParticleTransform(const ParticleTransform &t)
      : d_translation(t.d_translation), d_axis(t.d_axis),
        d_theta(t.d_theta), d_scale(t.d_scale){};

  /*!
   * @brief Returns the transformed vector. We assume that the passed vector
   * passes through origin.
   *
   * Let B(0, R) is the ball centered at origin. Let v is a point in ball B
   * (0,R).
   *
   * Suppose we want to transform v so that it is now in ball B(x, r), where
   * x is the point in space, r is the radius of new ball.
   *
   * Further, suppose we also want to rotate the v by angle theta about axis a
   * and scale v by amount s.
   *
   * To do this, we assume that this class was constructed with x, a, theta,
   * and s, i.e. ParticleTransform(x, a, theta, s).
   *
   * Following transformation is applied on vector v
   *
   * 1. Rotation by angle theta about axis a
   *
   * 2. Next, scale the vector
   *
   * 3. Finally, translate the vector
   *
   * @param v Vector v in ball
   * @return vector Transformed vector
   */
  util::Point apply(const util::Point &v) const {

    return d_translation +
           d_scale * util::rotate(v, d_theta, d_axis);

    // return d_translation + d_scale * util::Point(v.d_x, -v.d_y, 0.);
    // return d_translation + v;
  };

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt = 0, int lvl = 0) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- ParticleTransform --------" << std::endl << std::endl;
    oss << tabS << "Scale = " << d_scale << std::endl;
    oss << tabS << "Angle = " << d_theta << std::endl;
    oss << tabS << "Translation = " << d_translation.printStr() << std::endl;
    oss << tabS << "Axis = " << d_axis.printStr() << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
};

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
   * @param z_deck Particle zone deck
   * @param mesh Pointer to mesh
   */
  RefParticle(inp::ParticleZone *z_deck, fe::Mesh * mesh);

  /**
   * @name Accessors
   */
  /**@{*/

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
   * @return x Reference coordinate
   */
  util::Point getNode(const size_t &i) const {
    return d_mesh_p->getNode(i);
  };

  /*!
   * @brief Get nodal volume
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
   * @brief Returns the string containing information of the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
   std::string printStr(int nt = 0, int lvl = 0) const;

  /*!
  * @brief Prints information of the object
  *
  * @param nt Number of tabs to append before printing
  * @param lvl Information level (higher means more information)
  */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }

private:
  /*! @brief Pointer to mesh on reference particle */
  std::unique_ptr<fe::Mesh> d_mesh_p;

  /*! @brief Id of mesh node closest to the particle center */
  size_t d_centerNode;

  /*! @brief Geometrical object defining this particle */
  std::shared_ptr<util::geometry::GeomObject> d_geom_p;

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
