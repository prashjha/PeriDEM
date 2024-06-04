/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PARTICLE_BASEPARTICLE_H
#define PARTICLE_BASEPARTICLE_H

#include "material/mparticle/material.h"
#include "model/modelData.h"

#include <cstring> // size_t type
#include <vector>

/*! @brief Collection of methods and data related to particle object */
namespace particle {

/*! @brief A class to store particle geometry, nodal discretization, and methods
 *
 * This class generates a base particle for peri-dem simulations. It holds
 * the nodal positions, geometry of particle, list of nodes
 */
class BaseParticle {

public:
  /*!
   * @brief Constructor
   *
   * @param type Type of the particle (e.g. particle or wall)
   */
  BaseParticle(std::string type = "none")
      : d_type(type), d_typeIndex(-1), d_id(0), d_typeId(0),
        d_zoneId(0),
        d_dim
                                  (0), d_numNodes
        (0), d_h(0), d_horizon(0), d_density(0),
        d_computeForce(true), d_material_p(nullptr), d_Rc(0.), d_Kn(0.),
        d_globStart(0), d_globEnd(0), d_modelData_p(nullptr) {

    if (type == "particle")
      d_typeIndex = 0;
    if (type == "wall")
        d_typeIndex = 1;
  }

  /*!
   * @brief Constructor
   *
   * @param type Type of the object (e.g. particle or wall)
   * @param id Id of this object in list of all base particle objects
   * @param type_id Id of this object in list of specific type of particles
   * (e.g. id in the list of walls or particles)
   * @param zone_id Zone id this object belongs to
   * @param dim Spatial dimension
   * @param num_nodes Number of nodes
   * @param h Mesh size
   * @param model_data Global model data
   */
  BaseParticle(std::string type, size_t id, size_t type_id, size_t zone_id,
               size_t dim,
               size_t num_nodes, double h,
               std::shared_ptr<model::ModelData> model_data)
      : d_type(type), d_typeIndex(-1), d_id(id), d_typeId(type_id), d_zoneId(zone_id), d_dim(dim),
        d_numNodes(num_nodes), d_h(h), d_horizon(0), d_density(0), d_computeForce(true),
        d_material_p(nullptr), d_Rc(0.), d_Kn(0.), d_globStart(0), d_globEnd(0),
        d_modelData_p(model_data) {

    if (type == "particle")
      d_typeIndex = 0;
    if (type == "wall")
      d_typeIndex = 1;
  }

  /**
   * @name Accessors
   */
  /**@{*/

  /*!
   * @brief Get type of this object
   * @return Type Type
   */
  std::string getType() const { return d_type; };

  /*!
   * @brief Get type (in integer format) of this object
   * @return Type Type
   */
  int getTypeIndex() const { return d_typeIndex; };

  /*!
   * @brief Get id
   * @return id ID of this object
   */
  size_t getId() const { return d_id; };

  /*!
   * @brief Get id among the group of object in the same type as this
   * @return id ID of this object
   */
  size_t getTypeId() const { return d_typeId; };

  /*!
   * @brief Get the dimension of the domain
   * @return N Dimension
   */
  size_t getDimension() const { return d_dim; };

  /*!
   * @brief Get mesh size
   * @return h Mesh size
   */
  double getMeshSize() const { return d_h; };

  /*!
   * @brief Get density
   * @return rho Density
   */
  double getDensity() const { return d_density; };

  /*!
   * @brief Get horizon
   * @return eps Horizon
   */
  double getHorizon() const { return d_horizon; };

  /*!
   * @brief Get pointer to material object
   * @return material Pointer to material object
   */
  material::Material * getMaterial() { return d_material_p.get(); };
  const material::Material * getMaterial() const { return d_material_p.get(); };

  /*!
   * @brief Get the number of nodes
   * @return N number of nodes
   */
  size_t getNumNodes() const { return d_numNodes; };

  /*!
   * @brief Get global id of node given the local id of node in this object
   * @param i_loc Local id of node
   * @return Id Gloabl id of node
   */
  size_t getNodeId(size_t i_loc) const { return i_loc + d_globStart; };

  /**
   * @name Get and set reference coordinate
   */
  /**@{*/

  /*!
   * @brief Get reference coordinate of the node
   * @param i Global id of node
   * @return x Reference coordinate
   */
  util::Point &getXRef(size_t i) { return d_modelData_p->getXRef(i); };
  const util::Point &getXRef(size_t i) const { return d_modelData_p->getXRef(i); };

  /*!
   * @brief Set reference coordinate of the node
   * @param i Global id of node
   * @param x Reference coordinate to set
   */
  void setXRef(size_t i, const util::Point &x) { d_modelData_p->setXRef(i, x); };

  /*!
   * @brief Add reference coordinate of the node
   * @param i Global id of node
   * @param x Reference coordinate to add
   */
  void addXRef(size_t i, const util::Point &x) { d_modelData_p->addXRef(i, x); };

  /*!
   * @brief Set specific reference coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Reference coordinate to set
   */
  void setXRef(size_t i, int dof, double x) {
    d_modelData_p->setXRef(i, dof, x);
  };

  /*!
   * @brief Add specific reference coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Reference coordinate to add
   */
  void addXRef(size_t i, int dof, double x) {
    d_modelData_p->addXRef(i, dof, x);
  };

  /*!
   * @brief Get reference coordinate of the node given node's local id
   * @param i Local id of node
   * @return x Reference coordinate
   */
  util::Point &getXRefLocal(size_t i) { return d_modelData_p->getXRef(i+d_globStart); };
  const util::Point &getXRefLocal(size_t i) const { return d_modelData_p->getXRef(i+d_globStart); };

  /*!
   * @brief Set reference coordinate of the node given node's local id
   * @param i Local id of node
   * @param x Reference coordinate to set
   */
  void setXRefLocal(size_t i, const util::Point &x) { d_modelData_p->setXRef(i+d_globStart, x); };

  /*!
   * @brief Add to reference coordinate of the node given node's local id
   * @param i Local id of node
   * @param x Reference coordinate to add
   */
  void addXRefLocal(size_t i, const util::Point &x) { d_modelData_p->addXRef(i+d_globStart, x); };

  /*!
   * @brief Set specific reference coordinate of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Reference coordinate to set
   */
  void setXRefLocal(size_t i, int dof, double x) { d_modelData_p->setXRef
        (i+d_globStart, dof, x); };

  /*!
   * @brief Add to specific reference coordinate of the node given node's
   * local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Reference coordinate to add
   */
  void addXRefLocal(size_t i, int dof, double x) { d_modelData_p->addXRef
        (i+d_globStart, dof, x); };

  /** @}*/

  /**
   * @name Get and set current coordinate
   */
  /**@{*/

  /*!
   * @brief Get current coordinate of the node
   * @param i Global id of node
   * @return x Current coordinate
   */
  util::Point &getX(size_t i) { return d_modelData_p->getX(i); };
  const util::Point &getX(size_t i) const { return d_modelData_p->getX(i); };

  /*!
   * @brief Set current coordinate of the node
   * @param i Global id of node
   * @param x Current coordinate to set
   */
  void setX(size_t i, const util::Point &x) { d_modelData_p->setX(i, x); };

  /*!
   * @brief Add current coordinate of the node
   * @param i Global id of node
   * @param x Current coordinate to add
   */
  void addX(size_t i, const util::Point &x) { d_modelData_p->addX(i, x); };

  /*!
   * @brief Set specific current coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Current coordinate to set
   */
  void setX(size_t i, int dof, double x) { d_modelData_p->setX(i, dof, x); };

  /*!
   * @brief Add to specific current coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Current coordinate to add
   */
  void addX(size_t i, int dof, double x) { d_modelData_p->addX(i, dof, x); };

  /*!
   * @brief Get current coordinate of the node given node's local id
   * @param i Local id of node
   * @return x Current coordinate
   */
  util::Point &getXLocal(size_t i) { return d_modelData_p->getX(i+d_globStart); };
  const util::Point &getXLocal(size_t i) const { return d_modelData_p->getX(i+d_globStart); };

  /*!
   * @brief Set current coordinate of the node given node's local id
   * @param i Local id of node
   * @param x Current coordinate to set
   */
  void setXLocal(size_t i, const util::Point &x) { d_modelData_p->setX(i+d_globStart, x); };

  /*!
   * @brief Add to current coordinate of the node given node's local id
   * @param i Local id of node
   * @param x Current coordinate to add
   */
  void addXLocal(size_t i, const util::Point &x) { d_modelData_p->addX(i+d_globStart, x); };

  /*!
   * @brief Set specific current coordinate of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Current coordinate to set
   */
  void setXLocal(size_t i, int dof, double x) { d_modelData_p->setX
                                                (i+d_globStart, dof, x); };

  /*!
   * @brief Add to specific current coordinate of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Current coordinate to add
   */
  void addXLocal(size_t i, int dof, double x) { d_modelData_p->addX
                                                (i+d_globStart, dof, x); };

  /** @}*/

  /**
   * @name Get and set displacement
   */
  /**@{*/

  /*!
   * @brief Get displacement of the node
   * @param i Global id of node
   * @return u Displacement
   */
  util::Point &getU(size_t i) { return d_modelData_p->getU(i); };
  const util::Point &getU(size_t i) const { return d_modelData_p->getU(i); };

  /*!
   * @brief Set displacement of the node
   * @param i Global id of node
   * @param u Displacement to set
   */
  void setU(size_t i, const util::Point &u) { d_modelData_p->setU(i, u); };

  /*!
   * @brief Add to displacement of the node
   * @param i Global id of node
   * @param u Displacement to add
   */
  void addU(size_t i, const util::Point &u) { d_modelData_p->addU(i, u); };

  /*!
   * @brief Set displacement of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param u Displacement to set
   */
  void setU(size_t i, int dof, double u) { d_modelData_p->setU(i, dof, u); };

  /*!
   * @brief Add to displacement of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param u Displacement to add
   */
  void addU(size_t i, int dof, double u) { d_modelData_p->addU(i, dof, u); };

  /*!
   * @brief Get displacement of the node given node's local id
   * @param i Local id of node
   * @return u Displacement
   */
  util::Point &getULocal(size_t i) { return d_modelData_p->getU(i+d_globStart); };

  /*! @copydoc getULocal(size_t i) */
  const util::Point &getULocal(size_t i) const { return d_modelData_p->getU(i+d_globStart); };

  /*!
   * @brief Set displacement of the node given node's local id
   * @param i Local id of node
   * @param u Displacement to set
   */
  void setULocal(size_t i, const util::Point &u) { d_modelData_p->setU(i+d_globStart, u); };

  /*!
   * @brief Add to displacement of the node given node's local id
   * @param i Local id of node
   * @param u Displacement to add
   */
  void addULocal(size_t i, const util::Point &u) { d_modelData_p->addU(i+d_globStart, u); };

  /*!
   * @brief Set displacement of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param u Displacement to set
   */
  void setULocal(size_t i, int dof, double u) { d_modelData_p->setU
                                                (i+d_globStart, dof, u); };

  /*!
   * @brief Add to displacement of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param u Displacement to add
   */
  void addULocal(size_t i, int dof, double u) { d_modelData_p->addU
                                                (i+d_globStart, dof, u); };

  /** @}*/

  /**
   * @name Get and set velocity
   */
  /**@{*/

  /*!
   * @brief Get velocity of the node
   * @param i Global id of node
   * @return v Velocity
   */
  util::Point &getV(size_t i) { return d_modelData_p->getV(i); };
  const util::Point &getV(size_t i) const { return d_modelData_p->getV(i); };

  /*!
   * @brief Set velocity of the node
   * @param i Global id of node
   * @param v Velocity to set
   */
  void setV(size_t i, const util::Point &v) { d_modelData_p->setV(i, v); };

  /*!
   * @brief Add to velocity of the node
   * @param i Global id of node
   * @param v Velocity to add
   */
  void addV(size_t i, const util::Point &v) { d_modelData_p->addV(i, v); };

  /*!
   * @brief Set velocity of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param v Velocity to set
   */
  void setV(size_t i, int dof, double v) { d_modelData_p->setV(i, dof, v); };

  /*!
   * @brief Add to velocity of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param v Velocity to add
   */
  void addV(size_t i, int dof, double v) { d_modelData_p->addV(i, dof, v); };

  /*!
   * @brief Get velocity of the node given node's local id
   * @param i Local id of node
   * @return v Velocity
   */
  util::Point &getVLocal(size_t i) { return d_modelData_p->getV(i+d_globStart); };
  const util::Point &getVLocal(size_t i) const { return d_modelData_p->getV(i+d_globStart); };

  /*!
   * @brief Set velocity of the node given node's local id
   * @param i Local id of node
   * @param v Velocity to set
   */
  void setVLocal(size_t i, const util::Point &v) { d_modelData_p->setV(i+d_globStart, v); };

  /*!
   * @brief Add to velocity of the node given node's local id
   * @param i Local id of node
   * @param v Velocity to add
   */
  void addVLocal(size_t i, const util::Point &v) { d_modelData_p->addV(i+d_globStart, v); };

  /*!
   * @brief Set velocity of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param v Velocity to set
   */
  void setVLocal(size_t i, int dof, double v) { d_modelData_p->setV
                                                (i+d_globStart, dof, v); };

  /*!
   * @brief Add to velocity of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param v Velocity to add
   */
  void addVLocal(size_t i, int dof, double v) { d_modelData_p->addV
                                                (i+d_globStart, dof, v); };

  /** @}*/

  /**
   * @name Get and set force
   */
  /**@{*/

  /*!
   * @brief Get force of the node
   * @param i Global id of node
   * @return f Force
   */
  util::Point &getF(size_t i) { return d_modelData_p->getF(i); };
  const util::Point &getF(size_t i) const { return d_modelData_p->getF(i); };

  /*!
   * @brief Set force of the node
   * @param i Global id of node
   * @param f Force to set
   */
  void setF(size_t i, const util::Point &f) { d_modelData_p->setF(i, f); };

  /*!
   * @brief Add to force of the node
   * @param i Global id of node
   * @param f Force to add
   */
  void addF(size_t i, const util::Point &f) { d_modelData_p->addF(i, f); };

  /*!
   * @brief Set force of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param f Force to set
   */
  void setF(size_t i, int dof, double f) { d_modelData_p->setF(i, dof, f); };

  /*!
   * @brief Add to force of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param f Force to add
   */
  void addF(size_t i, int dof, double f) { d_modelData_p->addF(i, dof, f); };

  /*!
   * @brief Get force of the node given node's local id
   * @param i Local id of node
   * @return f Force
   */
  util::Point &getFLocal(size_t i) { return d_modelData_p->getF(i+d_globStart); };
  const util::Point &getFLocal(size_t i) const { return d_modelData_p->getF(i+d_globStart); };

  /*!
   * @brief Set force of the node given node's local id
   * @param i Local id of node
   * @param f Force to set
   */
  void setFLocal(size_t i, const util::Point &f) { d_modelData_p->setF(i+d_globStart, f); };

  /*!
   * @brief Add to force of the node given node's local id
   * @param i Local id of node
   * @param f Force to add
   */
  void addFLocal(size_t i, const util::Point &f) { d_modelData_p->addF(i+d_globStart, f); };

  /*!
   * @brief Set force of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param f Force to set
   */
  void setFLocal(size_t i, int dof, double f) { d_modelData_p->setF
                                                (i+d_globStart, dof, f); };

  /*!
   * @brief Add to force of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param f Force to add
   */
  void addFLocal(size_t i, int dof, double f) { d_modelData_p->addF
                                                (i+d_globStart, dof, f); };

  /** @}*/

  /**
   * @name Get and set volume
   */
  /**@{*/

  /*!
   * @brief Get volume of the node
   * @param i Global id of node
   * @return vol Volume
   */
  double &getVol(size_t i) { return d_modelData_p->getVol(i); };
  const double &getVol(size_t i) const { return d_modelData_p->getVol(i); };

  /*!
   * @brief Set volume of the node
   * @param i Global id of node
   * @param vol Volume to set
   */
  void setVol(size_t i, const double &vol) { d_modelData_p->setVol(i, vol); };

  /*!
   * @brief Add to volume of the node
   * @param i Global id of node
   * @param vol Volume to add
   */
  void addVol(size_t i, const double &vol) { d_modelData_p->addVol(i, vol); };

  /*!
   * @brief Get volume of the node given node's local id
   * @param i Local id of node
   * @return vol Volume
   */
  double &getVolLocal(size_t i) { return d_modelData_p->getVol(i+d_globStart); };
  const double &getVolLocal(size_t i) const { return d_modelData_p->getVol(i+d_globStart); };

  /*!
   * @brief Set volume of the node given node's local id
   * @param i Local id of node
   * @param vol Volume to set
   */
  void setVolLocal(size_t i, const double &vol) { d_modelData_p->setVol(i+d_globStart, vol); };

  /*!
   * @brief Add to volume of the node given node's local id
   * @param i Local id of node
   * @param vol Volume to add
   */
  void addVolLocal(size_t i, const double &vol) { d_modelData_p->addVol(i+d_globStart, vol); };

  /** @}*/

  /**
   * @name Get and set fixity
   */
  /**@{*/

  /*!
   * @brief Get fixity of the node
   * @param i Global id of node
   * @return flag Fixity
   */
  uint8_t &getFix(size_t i) { return d_modelData_p->getFix(i); };
  const uint8_t &getFix(size_t i) const { return d_modelData_p->getFix(i); };

  /*!
   * @brief Set fixity of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param flag Fixity to set
   */
  void setFix(size_t i, const unsigned int &dof, const bool &flag) {
    d_modelData_p->setFix(i, dof, flag);
  };

  /*!
   * @brief Get fixity of the node given node's local id
   * @param i Local id of node
   * @return flag Fixity
   */
  uint8_t &getFixLocal(size_t i) { return d_modelData_p->getFix(i+d_globStart); };
  const uint8_t &getFixLocal(size_t i) const { return d_modelData_p->getFix(i+d_globStart); };

  /*!
   * @brief Set fixity of the node given node's local id
   * @param i Local id of node
   * @param dof Direction or degree of freedom to be modified
   * @param flag Fixity to set
   */
  void setFixLocal(size_t i, const unsigned int &dof, const bool &flag) {
    d_modelData_p->setFix(i+d_globStart, dof, flag);
  };

  /** @}*/

  /**
   * @name Get and set mx
   */
  /**@{*/

  /*!
   * @brief Get weighted-volume (mx) of the node
   * @param i Global id of node
   * @return mx Weighted-volume
   */
  double &getMx(size_t i) { return d_modelData_p->getMx(i); };
  const double &getMx(size_t i) const { return d_modelData_p->getMx(i); };

  /*!
   * @brief Set weighted-volume (mx) of the node
   * @param i Global id of node
   * @param mx Weighted-volume to set
   */
  void setMx(size_t i, const double &mx) { d_modelData_p->setMx(i, mx); };

  /*!
   * @brief Add to weighted-volume (mx) of the node
   * @param i Global id of node
   * @param mx Weighted-volume to add
   */
  void addMx(size_t i, const double &mx) { d_modelData_p->addMx(i, mx); };

  /*!
   * @brief Get weighted-volume (mx) of the node given node's local id
   * @param i Local id of node
   * @return mx Weighted-volume
   */
  double &getMxLocal(size_t i) { return d_modelData_p->getMx(i+d_globStart); };
  const double &getMxLocal(size_t i) const { return d_modelData_p->getMx(i+d_globStart); };

  /*!
   * @brief Set weighted-volume (mx) of the node given node's local id
   * @param i Local id of node
   * @param mx Weighted-volume to set
   */
  void setMxLocal(size_t i, const double &mx) { d_modelData_p->setMx(i+d_globStart, mx); };

  /*!
   * @brief Add to weighted-volume (mx) of the node given node's local id
   * @param i Local id of node
   * @param mx Weighted-volume to add
   */
  void addMxLocal(size_t i, const double &mx) { d_modelData_p->addMx(i+d_globStart, mx); };

  /** @}*/

  /**
   * @name Get and set thetax
   */
  /**@{*/

  /*!
   * @brief Get volumetric deformation (thetax) of the node
   * @param i Global id of node
   * @return thetax Volumetric deformation
   */
  double &getThetax(size_t i) { return d_modelData_p->getThetax(i); };
  const double &getThetax(size_t i) const { return d_modelData_p->getThetax(i)
                                                ; };

  /*!
   * @brief Set volumetric deformation (thetax) of the node
   * @param i Global id of node
   * @param thetax Volumetric deformation to set
   */
  void setThetax(size_t i, const double &thetax) { d_modelData_p->setThetax
                                                   (i, thetax); };

  /*!
   * @brief Add to volumetric deformation (thetax) of the node
   * @param i Global id of node
   * @param thetax Volumetric deformation to add
   */
  void addThetax(size_t i, const double &thetax) { d_modelData_p->addThetax
                                                   (i, thetax); };

  /*!
   * @brief Get volumetric deformation (thetax) of the node given node's local id
   * @param i Local id of node
   * @return thetax Volumetric deformation
   */
  double &getThetaxLocal(size_t i) { return d_modelData_p->getThetax(i+d_globStart); };
  const double &getThetaxLocal(size_t i) const { return d_modelData_p->getThetax(i+d_globStart)
        ; };

  /*!
   * @brief Set volumetric deformation (thetax) of the node given node's local id
   * @param i Local id of node
   * @param thetax Volumetric deformation to set
   */
  void setThetaxLocal(size_t i, const double &thetax) { d_modelData_p->setThetax
        (i+d_globStart, thetax); };

  /*!
   * @brief Add to volumetric deformation (thetax) of the node given node's local id
   * @param i Local id of node
   * @param thetax Volumetric deformation to add
   */
  void addThetaxLocal(size_t i, const double &thetax) { d_modelData_p->addThetax
        (i+d_globStart, thetax); };

  /** @}*/

public:

  /*! @brief particle type */
  std::string d_type;

  /*! String to integer map for particle type */
  int d_typeIndex; // string - int map

  /*! @brief Id of this particle */
  size_t d_id;

  /*! @brief Id of this particle in the category (for example if this is a
   * wall, what is its id in vector of walls) */
  size_t d_typeId;

  /*! @brief Specify zone to which this particle belongs to */
  size_t d_zoneId;

  /*! @brief Dimension of this particle */
  size_t d_dim;

  /*! @brief Number of nodes in this particle */
  size_t d_numNodes;

  /*! @brief mesh size */
  double d_h;

  /*! @brief Specify if we compute force */
  bool d_computeForce;

  /*! @brief horizon */
  double d_horizon;

  /*! @brief density */
  double d_density;

  /*! @brief Pointer to peridynamic material object */
  std::unique_ptr<material::Material> d_material_p;

  /*! @brief Contact radius for contact between internal nodes of particle */
  double d_Rc;

  /*! @brief Normal contact coefficient for internal contact */
  double d_Kn;

  /*! @brief Id of first node of this object in global node list */
  size_t d_globStart;

  /*! @brief Id of last node of this object in global node list */
  size_t d_globEnd;

  /*! @brief Reference to model class */
  std::shared_ptr<model::ModelData> d_modelData_p;
};

} // namespace particle

#endif // PARTICLE_BASEPARTICLE_H
