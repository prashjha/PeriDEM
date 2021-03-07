/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INP_P_ICDECK_H
#define INP_P_ICDECK_H

#include "util/io.h"

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief User-input data for particle neighbor search */
struct PICDeck {

  /*! @brief Specify if particle initial condition is active */
  bool d_icActive;

  /*! @brief Initial velocity vector */
  util::Point d_icVec;

  /*! @brief List of particles (if any) */
  std::vector<size_t> d_pList;

  /*! @brief List of walls (if any) */
  std::vector<size_t> d_wList;

  /*!
   * @brief Constructor
   */
  PICDeck() : d_icActive(false), d_icVec(){};

  std::string printStr(int nt = 0, int lvl = 0) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- PICDeck --------" << std::endl << std::endl;
    oss << tabS << "IC active  = " << d_icActive << std::endl;
    oss << tabS << "IC vector = " << d_icVec.printStr(0, 0) << std::endl;
    if (!d_pList.empty())
      oss << tabS << "Particle list = ["
          << util::io::printStr<size_t>(d_pList, 0) << "]" << std::endl;
    if (!d_wList.empty())
      oss << tabS << "Wall list = ["
          << util::io::printStr<size_t>(d_wList, 0) << "]" << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
};

/** @}*/

} // namespace inp

#endif // INP_P_ICDECK_H
