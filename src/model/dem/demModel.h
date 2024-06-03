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

  /*! @brief Prints message if any of these two conditions are true
   * 1. if check_condition == true and dbg_lvl > priority
   * OR
   * 2. dbg_lvl > override_priority
   *
   * @param oss Stream
   * @param priority Priority of message
   * @param check_condition Specify whether condition for logging/printing message is to be checked
   * @param override_priority Specify new priority
   * @param screen_out Specify whether to print the message also to screen (using std::cout)
   */
  void log(std::ostringstream &oss, int priority = 0, bool check_condition = true, int override_priority = -1, bool screen_out = false);

  /*! @brief Prints message if any of these two conditions are true
   * 1. if check_condition == true and dbg_lvl > priority
   * OR
   * 2. dbg_lvl > override_priority
   *
   * @param str Message string
   * @param priority Priority of message
   * @param check_condition Specify whether condition for logging/printing message is to be checked
   * @param override_priority Specify new priority
   * @param screen_out Specify whether to print the message also to screen (using std::cout)
   */
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

  /*! @brief Initialize remaining data members */
  virtual void init();

  /** @}*/

  /**
   * @name Methods to implement explicit time integration
   */
  /**@{*/

  /*! @brief Perform time integration */
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

  /*! @brief Perform time integration using velocity verlet scheme */
  virtual void integrateVerlet();

  /** @}*/

  /**
   * @name Compute methods
   */
  /**@{*/

  /*! @brief Computes peridynamic forces and contact forces */
  virtual void computeForces();

  /*! @brief Computes peridynamic forces */
  virtual void computePeridynamicForces();

  /*! @brief Computes external/boundary condition forces */
  virtual void computeExternalForces();

  /*! @brief Applies displacement boundary conditions */
  virtual void computeExternalDisplacementBC();

  /*! @brief Computes contact forces */
  virtual void computeContactForces();

  /*! @brief Applies initial condition */
  virtual void applyInitialCondition();

  /** @}*/

  /**
   * @name Setup methods
   */
  /**@{*/

  /*! @brief Creates particles in a given container */
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
   *
   * @param z Zone id
   * @param ref_p Shared pointer to reference particle from which we need to create new particles
   */
  virtual void createParticlesFromFile(size_t z,
                                       std::shared_ptr<particle::RefParticle> ref_p);

  /*! @brief Creates walls */
  virtual void createWalls();

  /*! @brief Update neighborlist for contact */
  virtual void updateContactNeighborlist();

  /*! @brief Update neighborlist for peridynamics force */
  virtual void updatePeridynamicNeighborlist();

  /*! @brief Update neighborlist for contact and peridynamics force*/
  virtual void updateNeighborlistCombine();

  /*! @brief Creates particles in a given container */
  virtual void setupContact();

  /** @}*/

  /**
   * @name Methods to handle output and debug
   */
  /**@{*/

  /*! @brief Output the snapshot of data at current time step */
  virtual void output();

  /*!
   * @brief Function that handles post-processing for two particle collision test and returns maximum vertical displacement of particle
   *
   * @return string Reports maximum vertical displacement
   */
  std::string ppTwoParticleTest();

  /*!
   * @brief Function that handles post-processing for compressive test of particulate media by rigid wall and returns wall penetration and reaction force on wall
   *
   * @return string Reports wall penetration and reaction force
   */
  std::string ppCompressiveTest();

  /*! @brief Checks if simulation should be stopped due to abnormal state of system */
  virtual void checkStop();

  /** @}*/
};

/** @}*/

} // namespace model

#endif // MODEL_FASTDEMMODEL_H
