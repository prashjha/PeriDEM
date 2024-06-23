/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_MODELDECK_H
#define INP_MODELDECK_H

#include <string>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store model related input data */
struct ModelDeck {

  /*!
   * @brief Simulation type
   *
   * List of allowed values are:
   * - \a explicit
   * - \a implicit
   */
  std::string d_simType;

  /*! @brief Flag indicating if this restart problem */
  bool d_isRestartActive;

  /*!
   * @brief Tag for spatial discretization
   *
   * List of allowed values are:
   * - \a **finite_difference**
   * - \a **weak_finite_element**
   * - \a **nodal_finite_element**
   * - \a **truss_finite_element**
   */
  std::string d_spatialDiscretization;

  /*!
   * @brief Tag for time discretization
   *
   * List of allowed values are:
   * - *empty string*
   * - \a **central_difference**
   * - \a **velocity_verlet**
   */
  std::string d_timeDiscretization;

  /*! @brief Flag to indicate if we should populate element-node connectivity data in meshes */
  bool d_populateElementNodeConnectivity;

  /*! @brief Order of quadrature approximation for strain and stress computation (default is 1) */
  size_t d_quadOrder;

  /*! @brief Specify if this is PeriDEM simulation */
  bool d_isPeriDEM;

  /*! @brief Dimension */
  size_t d_dim;

  /*! @brief Final simulation time */
  double d_tFinal;

  /*! @brief Size of time steps */
  double d_dt;

  /*! @brief Number of time steps */
  size_t d_Nt;

  /*! @brief Horizon */
  double d_horizon;

  /*!
   * @brief Ratio of Horizon to mesh size
   *
   * E.g. ratio = 4 means mesh size is 1/4th of the horizon.
   */
  int d_rh;

  /*! @brief Mesh size */
  double d_h;

  /*! @brief Seed for random calculations (if any) */
  int d_seed;

  /*!
   * @brief Constructor
   */
  ModelDeck()
      : d_dim(0), d_isRestartActive(false), d_populateElementNodeConnectivity(false),
        d_tFinal(0.), d_dt(0.), d_Nt(0),
        d_horizon(0.), d_rh(0), d_h(0.), d_isPeriDEM(false), d_seed(1), d_quadOrder(1) {};

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt = 0, int lvl = 0) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- ModelDeck --------" << std::endl << std::endl;
    oss << tabS << "Simulation type = " << d_simType << std::endl;
    oss << tabS << "Restart active = " << d_isRestartActive << std::endl;
    oss << tabS << "Populate element-node connectivity data = " << d_populateElementNodeConnectivity << std::endl;
    oss << tabS << "Order of quad approximation = " << d_quadOrder << std::endl;
    oss << tabS << "Spatial discretization type = " << d_spatialDiscretization
              << std::endl;
    oss << tabS << "Time discretization type = " << d_timeDiscretization
              << std::endl;
    oss << tabS << "Is it peri-dem simulation = " << d_isPeriDEM << std::endl;
    oss << tabS << "Dimension = " << d_dim << std::endl;
    oss << tabS << "Final time = " << d_tFinal << std::endl;
    oss << tabS << "Time step size = " << d_dt << std::endl;
    oss << tabS << "Number of time step = " << d_Nt << std::endl;
    oss << tabS << "Horizon = " << d_horizon << std::endl;
    oss << tabS << "Horizon to mesh size ratio = " << d_rh << std::endl;
    oss << tabS << "Mesh size = " << d_h << std::endl;
    oss << tabS << "Seed = " << d_seed << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
};

/** @}*/

} // namespace inp

#endif // INP_MODELDECK_H
