/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INP_PNEIGHBORDECK_H
#define INP_PNEIGHBORDECK_H

#include "util/io.h"

namespace inp {

/** @}*/

/*! @brief User-input data for particle neighbor search */
struct PNeighborDeck {

  /*! @brief Neighbor search update criteria (if any) */
  std::string d_updateCriteria;

  /*! @brief Neighbor search factor (search length is factor times biggest
   * radius of particle) */
  double d_sFactor;

  /*! @brief Neighbor search tolerance */
  double d_sTol;

  /*!
   * @brief Constructor
   */
  PNeighborDeck()
      : d_updateCriteria("max_distance_travel"), d_sFactor(5.), d_sTol(0.){};

  std::string printStr(int nt = 0, int lvl = 0) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- PNeighborDeck --------" << std::endl << std::endl;
    oss << tabS << "Update criteria  = " << d_updateCriteria << std::endl;
    oss << tabS << "Search factor = " << d_sFactor << std::endl;
    oss << tabS << "Search tolerance = " << d_sTol << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
};

/** @}*/

} // namespace inp

#endif // INP_PNEIGHBORDECK_H
