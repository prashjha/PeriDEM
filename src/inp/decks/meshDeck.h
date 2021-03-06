/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INP_MESHDECK_H
#define INP_MESHDECK_H

#include "util/io.h"
#include <string>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store mesh related input data */
struct MeshDeck {

  /*! @brief Dimension */
  size_t d_dim;

  /*!
   * @brief Tag for spatial discretization
   *
   * List of allowed values are:
   * - \a finite_difference
   * - \a weak_finite_element
   * - \a nodal_finite_element
   * - \a truss_finite_element
   */
  std::string d_spatialDiscretization;

  /*! @brief Filename to read mesh data */
  std::string d_filename;

  /*! @brief Flag which indicates if mesh size is to be computed */
  bool d_computeMeshSize;

  /*! @brief Mesh size */
  double d_h;

  /*!
   * @brief Constructor
   */
  MeshDeck() : d_dim(0), d_computeMeshSize(false), d_h(0.){};

  std::string printStr(int nt = 0, int lvl = 0) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- MeshDeck --------" << std::endl << std::endl;
    oss << tabS << "Dimension = " << d_dim << std::endl;
    oss << tabS << "Spatial discretization type = " << d_spatialDiscretization
        << std::endl;
    oss << tabS << "Filename = " << d_filename << std::endl;
    oss << tabS << "Compute mesh size = " << d_computeMeshSize << std::endl;
    oss << tabS << "Mesh size = " << d_h << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
};

/** @}*/

} // namespace inp

#endif // INP_MESHDECK_H
