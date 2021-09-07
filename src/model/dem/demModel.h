/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MODEL_FASTDEMMODEL_H
#define MODEL_FASTDEMMODEL_H

#include <hpx/config.hpp>

#include "../modelData.h"

namespace model {

/**
 * \defgroup Explicit Explicit
 */
/**@{*/

/*! @brief A class for *discrete element particle simulation* with
 * *peridynamic model*
 *
 * We consider *central difference* for time integration.
 *
 * This class acts as a holder of lower rank classes, such as set of
 * particles, neighborlist, etc., and uses the methods and data of the
 * lower rank classes to perform calculation.
 */
class DEMModel : public ModelData {

public:
  /*!
   * @brief Constructor
   * @param deck The input deck
   */
  explicit DEMModel(inp::Input *deck);

  /*! logging 
   * 
   * Prints message if any of these two conditions are true
   * 1. if check_condition == true and dbg_lvl > priority
   * OR
   * 2. dbg_lvl > override_priority
   */
  void log(std::ostringstream &oss, int priority = 0, bool check_condition = true, int override_priority = -1, bool screen_out = false);
  void log(const std::string &str, int priority = 0, bool check_condition = true, int override_priority = -1, bool screen_out = false);

  /*!
   * @brief Main driver to simulate
   * @param deck Input deck
   */
  virtual void run(inp::Input *deck);

  /**
   * @name Methods to initialize and run model
   */
  /**@{*/

  /*!
   * @brief Restarts the simulation from previous state
   * @param deck Input deck
   */
  virtual void restart(inp::Input *deck);

  /*!
   * @brief Initialize remaining data members
   */
  virtual void init();

  /** @}*/

  /**
   * @name Methods to implement explicit time integration
   */
  /**@{*/

  /*!
   * @brief Perform time integration
   */
  virtual void integrate();

  /*!
   * @brief Performs one time step
   *
   * Depending on the time-stepping scheme specified in the input file, this
   * will either call integrateCD or integrateVerlet.
   */
  virtual void integrateStep();

  /*!
   * @brief Perform time integration using central-difference scheme
   *
   * Central difference scheme
   * \f[ u_{new} = \Delta t^2 (f_{int} + f_{ext}) / \rho  +
   * \Delta t v_{old} + u_{old} \f]
   * \f[ v_{new} = \frac{u_{new} - u_{old}}{\Delta t}. \f]
   */
  virtual void integrateCD();

  virtual void integrateVerlet();

  /** @}*/

  /**
   * @name Compute methods
   */
  /**@{*/

  /*!
   * @brief Computes peridynamic forces and contact forces
   */
  virtual void computeForces();

  virtual void computePeridynamicForces();

  virtual void computeExternalForces();

  virtual void computeExternalDisplacementBC();

  /*!
   * @brief Computes peridynamic forces and contact forces
   */
  virtual void computeContactForces();

  virtual void applyInitialCondition();

  /** @}*/

  /**
   * @name Setup methods
   */
  /**@{*/

  /*!
   * @brief Creates particles in a given container
   */
  virtual void createParticles();

  /*!
   * @brief Creates particles in a Hexagonal arrangement
   *
   * How the generation works:
   * 1. We translate the reference particle to location given in the file.
   * 2. We scale the reference particle such that the bounding radius of
   * scaled particle is same as radius in the data. So scaling factor is
   * radius/radius_ref_particle
   * 3. We rotate the particle by amount "orient". In case of "loc_rad", we
   * apply random orientation.
   */
  virtual void createParticlesFromFile(size_t z,
                                       std::shared_ptr<particle::RefParticle> ref_p);

  /*!
   * @brief Creates walls
   */
  virtual void createWalls();

  /*!
   * @brief Creates neighborlist of particles
   */
  virtual void updateContactNeighborlist();
  virtual void updatePeridynamicNeighborlist();
  virtual void updateNeighborlistCombine();

  /*!
   * @brief Creates particles in a given container
   */
  virtual void setupContact();

  /** @}*/

  /**
   * @name Methods to handle output and debug
   */
  /**@{*/

  /*!
   * @brief Output the snapshot of data at current time step
   */
  virtual void output();

  std::string ppTwoParticleTest();

  std::string ppCompressiveTest();

  virtual void checkStop();

  /** @}*/
};

/** @}*/

} // namespace model

#endif // MODEL_FASTDEMMODEL_H
