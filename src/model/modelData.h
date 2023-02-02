/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MODEL_MODELDATA_H
#define MODEL_MODELDATA_H

#include "material/mparticle/material.h"
#include "inp/input.h"
#include "loading/particleFLoading.h"
#include "loading/particleULoading.h"
#include "nsearch/nsearch.h"
#include "geometry/fracture.h"
#include <vector>

typedef nsearch::NFlannSearchKd NSearch;

// forward declare particle and wall
namespace particle {
class BaseParticle;
class RefParticle;
class Particle;
class Wall;
}

namespace model {

/**
 * \defgroup Explicit Explicit
 */
/**@{*/

/*! @brief A class to store model data
 */
class ModelData {

public:
  /*!
   * @brief Constructor
   * @param deck Input deck
   */
  ModelData(inp::Input *deck)
      : d_n(0), d_time(0.), d_infoN(1),
        d_input_p(deck),
        d_modelDeck_p(deck->getModelDeck()),
        d_restartDeck_p(deck->getRestartDeck()),
        d_outputDeck_p(deck->getOutputDeck()),
        d_pDeck_p(deck->getParticleDeck()), d_cDeck_p(deck->getContactDeck()),
        d_stop(false), d_hMax(0.), d_hMin(0.), d_maxContactR(0.),
        d_uLoading_p(nullptr), d_fLoading_p(nullptr),
        d_fracture_p(nullptr), d_nsearch_p(nullptr)  {}

  /*!
   * @brief Get pointer to base particle
   * @param i Location in the particle list
   * @return Pointer Pointer to base particle
   */
  particle::BaseParticle* &getBaseParticle(size_t i) { return
                                                     d_allParticles[i]; };
  const particle::BaseParticle* getBaseParticle(size_t i) const { return
        d_allParticles[i]; };

  /*!
   * @brief Get pointer to particle (excluding wall)
   * @param i Location in the particle list
   * @return Pointer Pointer to particle
   */
  particle::Particle* &getParticle(size_t i) { return
        d_particles[i]; };
  const particle::Particle* getParticle(size_t i) const { return
        d_particles[i]; };

  /*!
   * @brief Get pointer to wall
   * @param i Location in the wall list
   * @return Pointer Pointer to wall
   */
  particle::Wall* &getWall(size_t i) { return
        d_walls[i]; };
  const particle::Wall* getWall(size_t i) const { return
        d_walls[i]; };

  /*!
   * @brief Get density of particle
   * @param i Location in the particle list
   * @return rho Density
   */
  double getDensity(size_t i);

  /*!
   * @brief Get horizon of particle
   * @param i Location in the particle list
   * @return eps Horizon
   */
  double getHorizon(size_t i);

  /*!
   * @brief Get particle id given the location in particle list
   * @param i Location in the particle list
   * @return Id Particle id
   */
  size_t &getPtId(size_t i) { return d_ptId[i]; };
  const size_t &getPtId(size_t i) const { return d_ptId[i]; };

  /*!
   * @brief Set particle id given the location in particle list
   * @param i Location in the particle list
   * @return Id Particle id to set
   */
  void setPtId(size_t i, const size_t &id) { d_ptId[i] = id; };

  /**
   * @name Get and set reference coordinate
   */
  /**@{*/

  /*!
   * @brief Get reference coordinate of the node
   * @param i Global id of node
   * @return x Reference coordinate
   */
  util::Point &getXRef(size_t i) { return d_xRef[i]; };
  const util::Point &getXRef(size_t i) const { return d_xRef[i]; };

  /*!
   * @brief Set reference coordinate of the node
   * @param i Global id of node
   * @param x Reference coordinate to set
   */
  void setXRef(size_t i, const util::Point &x) { d_xRef[i] = x; };

  /*!
   * @brief Add reference coordinate of the node
   * @param i Global id of node
   * @param x Reference coordinate to add
   */
  void addXRef(size_t i, const util::Point &x) { d_xRef[i] += x; };

  /*!
   * @brief Set specific reference coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Reference coordinate to set
   */
  void setXRef(size_t i, int dof, double x) { d_xRef[i][dof] = x; };

  /*!
   * @brief Add specific reference coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Reference coordinate to add
   */
  void addXRef(size_t i, int dof, double x) { d_xRef[i][dof] += x; };

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
  util::Point &getX(size_t i) { return d_x[i]; };
  const util::Point &getX(size_t i) const { return d_x[i]; };

  /*!
   * @brief Set current coordinate of the node
   * @param i Global id of node
   * @param x Current coordinate to set
   */
  void setX(size_t i, const util::Point &x) { d_x[i] = x; };

  /*!
   * @brief Add current coordinate of the node
   * @param i Global id of node
   * @param x Current coordinate to add
   */
  void addX(size_t i, const util::Point &x) { d_x[i] += x; };

  /*!
   * @brief Set specific current coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Current coordinate to set
   */
  void setX(size_t i, int dof, double x) { d_x[i][dof] = x; };

  /*!
   * @brief Add to specific current coordinate of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param x Current coordinate to add
   */
  void addX(size_t i, int dof, double x) { d_x[i][dof] += x; };

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
  util::Point &getU(size_t i) { return d_u[i]; };
  const util::Point &getU(size_t i) const { return d_u[i]; };

  /*!
   * @brief Set displacement of the node
   * @param i Global id of node
   * @param u Displacement to set
   */
  void setU(size_t i, const util::Point &u) { d_u[i] = u; };

  /*!
   * @brief Add to displacement of the node
   * @param i Global id of node
   * @param u Displacement to add
   */
  void addU(size_t i, const util::Point &u) { d_u[i] += u; };

  /*!
   * @brief Set displacement of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param u Displacement to set
   */
  void setU(size_t i, int dof, double u) { d_u[i][dof] = u; };

  /*!
  * @brief Add to displacement of the node
  * @param i Global id of node
  * @param dof Direction or degree of freedom to be modified
  * @param u Displacement to add
  */
  void addU(size_t i, int dof, double u) { d_u[i][dof] += u; };

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
  util::Point &getV(size_t i) { return d_v[i]; };
  const util::Point &getV(size_t i) const { return d_v[i]; };

  /*!
   * @brief Set velocity of the node
   * @param i Global id of node
   * @param v Velocity to set
   */
  void setV(size_t i, const util::Point &v) { d_v[i] = v; };

  /*!
   * @brief Add to velocity of the node
   * @param i Global id of node
   * @param v Velocity to add
   */
  void addV(size_t i, const util::Point &v) { d_v[i] += v; };

  /*!
   * @brief Set velocity of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param v Velocity to set
   */
  void setV(size_t i, int dof, double v) { d_v[i][dof] = v; };

  /*!
   * @brief Add to velocity of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param v Velocity to add
   */
  void addV(size_t i, int dof, double v) { d_v[i][dof] += v; };

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
  util::Point &getF(size_t i) { return d_f[i]; };
  const util::Point &getF(size_t i) const { return d_f[i]; };

  /*!
   * @brief Set force of the node
   * @param i Global id of node
   * @param f Force to set
   */
  void setF(size_t i, const util::Point &f) { d_f[i] = f; };

  /*!
   * @brief Add to force of the node
   * @param i Global id of node
   * @param f Force to add
   */
  void addF(size_t i, const util::Point &f) { d_f[i] += f; };

  /*!
   * @brief Set force of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param f Force to set
   */
  void setF(size_t i, int dof, double f) { d_f[i][dof] = f; };

  /*!
   * @brief Add to force of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param f Force to add
   */
  void addF(size_t i, int dof, double f) { d_f[i][dof] += f; };

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
  double &getVol(size_t i) { return d_vol[i]; };
  const double &getVol(size_t i) const { return d_vol[i]; };

  /*!
   * @brief Set volume of the node
   * @param i Global id of node
   * @param vol Volume to set
   */
  void setVol(size_t i, const double &vol) { d_vol[i] = vol; };

  /*!
   * @brief Add to volume of the node
   * @param i Global id of node
   * @param vol Volume to add
   */
  void addVol(size_t i, const double &vol) { d_vol[i] += vol; };

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
  uint8_t &getFix(size_t i) { return d_fix[i]; };
  const uint8_t &getFix(size_t i) const { return d_fix[i]; };

  /*!
   * @brief Set fixity of the node
   * @param i Global id of node
   * @param dof Direction or degree of freedom to be modified
   * @param flag Fixity to set
   */
  void setFix(size_t i, const unsigned int &dof,
              const bool &flag) {
    // to set i^th bit as true of integer a,
    // a |= 1UL << (i % 8)

    // to set i^th bit as false of integer a,
    // a &= ~(1UL << (i % 8))

    flag ? (d_fix[i] |= 1UL << dof) : (d_fix[i] &= ~(1UL << dof));
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
  double &getMx(size_t i) { return d_mX[i]; };
  const double &getMx(size_t i) const { return d_mX[i]; };

  /*!
   * @brief Set weighted-volume (mx) of the node
   * @param i Global id of node
   * @param mx Weighted-volume to set
   */
  void setMx(size_t i, const double &mx) { d_mX[i] = mx; };

  /*!
   * @brief Add to weighted-volume (mx) of the node
   * @param i Global id of node
   * @param mx Weighted-volume to add
   */
  void addMx(size_t i, const double &mx) { d_mX[i] += mx; };

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
  double &getThetax(size_t i) { return d_thetaX[i]; };
  const double &getThetax(size_t i) const { return d_thetaX[i]; };

  /*!
   * @brief Set volumetric deformation (thetax) of the node
   * @param i Global id of node
   * @param thetax Volumetric deformation to set
   */
  void setThetax(size_t i, const double &thetax) { d_thetaX[i] = thetax; };

  /*!
   * @brief Add to volumetric deformation (thetax) of the node
   * @param i Global id of node
   * @param thetax Volumetric deformation to add
   */
  void addThetax(size_t i, const double &thetax) { d_thetaX[i] += thetax; };

  /** @}*/

public:
  /*! @brief Current time step */
  size_t d_n;

  /*! @brief Current time */
  double d_time;

  /*! @brief Print log step interval */
  size_t d_infoN;

  /*! @brief Pointer to Input object */
  inp::Input *d_input_p;

  /*! @brief Model deck */
  std::shared_ptr<inp::ModelDeck> d_modelDeck_p;

  /*! @brief Restart deck */
  std::shared_ptr<inp::RestartDeck> d_restartDeck_p;

  /*! @brief Output deck */
  std::shared_ptr<inp::OutputDeck> d_outputDeck_p;

  /*! @brief Particle deck */
  std::shared_ptr<inp::ParticleDeck> d_pDeck_p;

  /*! @brief Contact deck */
  std::shared_ptr<inp::ContactDeck> d_cDeck_p;

  /*! @brief flag to stop the simulation midway */
  bool d_stop;

  /*! @brief Minimum mesh over all particles and walls */
  double d_hMax;

  /*! @brief Maximum mesh over all particles and walls */
  double d_hMin;

  /*! @brief Maximum contact radius between over pairs of particles and walls */
  double d_maxContactR;

  /*! @brief Pointer to reference particle */
  std::vector<std::shared_ptr<particle::RefParticle>> d_rParticles;

  /*! @brief List of particles + walls */
  std::vector<particle::BaseParticle*> d_allParticles;

  /*! @brief List of particles */
  std::vector<particle::Particle*> d_particles;

  /*! @brief List of particles */
  std::vector<particle::Wall*> d_walls;

  /*! @brief Zone information of particles */
  std::vector<std::vector<size_t>> d_zInfo;

  /*! @brief Pointer to displacement Loading object */
  std::unique_ptr<loading::ParticleULoading> d_uLoading_p;

  /*! @brief Pointer to force Loading object */
  std::unique_ptr<loading::ParticleFLoading> d_fLoading_p;

  /*! @brief Fracture state of bonds */
  std::unique_ptr<geometry::Fracture> d_fracture_p;

  /*! @brief Pointer to nsearch */
  std::unique_ptr<NSearch> d_nsearch_p;

  /*! @brief reference positions of the nodes */
  std::vector<util::Point> d_xRef;

  /*! @brief Current positions of the nodes */
  std::vector<util::Point> d_x;

  /*! @brief Displacement of the nodes */
  std::vector<util::Point> d_u;

  /*! @brief Velocity of the nodes */
  std::vector<util::Point> d_v;

  /*! @brief Total force on the nodes */
  std::vector<util::Point> d_f;

  /*! @brief Nodal volumes */
  std::vector<double> d_vol;

  /*! @brief Global node to particle id (walls are assigned id after last
   * particle id) */
  std::vector<size_t> d_ptId;

  /*! @brief Neighbor data for contact forces */
  std::vector<std::vector<size_t>> d_neighC;

  /*! @brief Neighbor data for peridynamic forces */
  std::vector<std::vector<size_t>> d_neighPd;

  /*! @brief Square distance neighbor data for peridynamic forces */
  std::vector<std::vector<float>> d_neighPdSqdDist;

  /*! @brief Vector of fixity mask of each node
   *
   * First bit represents x-dof, second represents y-dof, and third
   * represents z-dof. 0 represents free dof and 1 represents fixed dof.
   *
   * We store data in uint8_t type which can hold 8 bit. At present we only
   * use first 3 bits.
   */
  std::vector<uint8_t> d_fix;

  /*! @brief Vector of fixity mask of each node for force */
  std::vector<uint8_t> d_forceFixity;

  /*! @brief Dilation
   *
   * In case of nonlinear peridynamic state based model, this will be the
   * spherical (hydrostatic) strain
   */
  std::vector<double> d_thetaX;

  /*! @brief Weighted volume
   *
   * In case of nonlinear peridynamic state based model, this data is not required
   */
  std::vector<double> d_mX;

  /*! @brief List of global nodes on which force (peridynamic/internal) is to
   * be computed */
  std::vector<size_t> d_fPdCompNodes;

  /*! @brief List of global nodes on which force (contact) is to be computed */
  std::vector<size_t> d_fContCompNodes;

  /*! @brief Damage at nodes */
  std::vector<float> d_Z;
};

/** @}*/

} // namespace model

#endif // MODEL_MODELDATA_H
