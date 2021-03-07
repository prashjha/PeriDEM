/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INP_P_BCDATA_H
#define INP_P_BCDATA_H

#include "util/geomObjects.h" // geometrical objects
#include "util/io.h"
#include <memory>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief User-input data for particle neighbor search */
struct PBCData {

  /*! @brief Method for applying force
   * e.g.
   *
   * - particle: provides global id of particle to which force should be
   * applied
   * - wall: provides global id of wall to which force should be applied
   * - region: provides area within which all nodes of any particle will get
   * this force
   */
  std::string d_selectionType;

  /*! @brief Region geometry (if any) */
  std::shared_ptr<util::geometry::GeomObject> d_region_p;

  /*! @brief List of particles (if any) */
  std::vector<size_t> d_pList;

  /*! @brief List of walls (if any) */
  std::vector<size_t> d_wList;

  /*!
   * @brief Name of the formula with respect to time
   *
   * List of allowed values are:
   * - "" (none)
   * - \a constant
   * - \a linear
   * - \a linear_step
   * - \a linear_slow_fast
   * - \a rotation
   */
  std::string d_timeFnType;

  /*!
   * @brief Name of the formula of with respect to spatial coordinate
   *
   * List of allowed values are:
   * - "" (none)
   * - \a constant
   * - \a hat_x
   * - \a hat_y
   * - \a sin
   * - \a rotation
   */
  std::string d_spatialFnType;

  /*!
   * @brief List of dofs on which this bc will be applied
   *
   * E.g. if bc is only applied on x-component, d_direction will be 1. If
   * bc is applied on x- and y-component, d_direction will be vector with
   * elements 1 and 2.
   */
  std::vector<size_t> d_direction;

  /*! @brief List of parameters for function wrt time */
  std::vector<double> d_timeFnParams;

  /*! @brief List of parameters for function wrt spatial coordinate */
  std::vector<double> d_spatialFnParams;

  /*! @brief Specify if this bc corresponds to zero displacement condition */
  bool d_isDisplacementZero;

  /*!
   * @brief Constructor
   */
  PBCData() : d_region_p(nullptr), d_isDisplacementZero(false){};

  std::string printStr(int nt = 0, int lvl = 0) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- PBCData --------" << std::endl << std::endl;
    oss << tabS << "Selection type  = " << d_selectionType << std::endl;
    if (d_region_p != nullptr)
      oss << d_region_p->printStr(nt+1, lvl);
    if (!d_pList.empty())
      oss << tabS << "Particle list = ["
          << util::io::printStr<size_t>(d_pList, 0) << "]" << std::endl;
    if (!d_wList.empty())
      oss << tabS << "Wall list = ["
          << util::io::printStr<size_t>(d_wList, 0) << "]" << std::endl;
    oss << tabS << "Time function type = " << d_timeFnType << std::endl;
    oss << tabS << "Time function parameters = ["
        << util::io::printStr<double>(d_timeFnParams, 0) << "]" << std::endl;
    oss << tabS << "Spatial function type = " << d_spatialFnType << std::endl;
    oss << tabS << "Spatial function parameters = ["
        << util::io::printStr<double>(d_spatialFnParams, 0) << "]" << std::endl;
    oss << tabS << "Direction = [" << util::io::printStr<size_t>(d_direction, 0)
        << "]" << std::endl;
    oss << tabS << "Is displacement zero = " << d_isDisplacementZero << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
};

/** @}*/

} // namespace inp

#endif // INP_P_BCDATA_H
