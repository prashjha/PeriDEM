/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_PNEIGHBORDECK_H
#define INP_PNEIGHBORDECK_H

#include "util/io.h"

namespace inp {

/**@{*/

/*! @brief User-input data for particle neighbor search */
struct PNeighborDeck {

  /*! @brief Neighbor search update criteria (if any) */
  std::string d_updateCriteria;

  /*! @brief Neighbor search factor (search length is factor times biggest
   * radius of particle) */
  double d_sFactor;

  /*! @brief Neighbor update time interval (for contact) */
  size_t d_neighUpdateInterval;

  /*! @brief Specify how deep we search for nodes near boundary for contact calculations */
  double d_nearBdNodesTol;

  /*!
   * @brief Constructor
   */
  PNeighborDeck(const json &j = json({}))
      : d_updateCriteria("simple_all"),
        d_sFactor(1.),
        d_neighUpdateInterval(1),
        d_nearBdNodesTol(0.5) {
    readFromJson(j);
  };

  /*!
   * @brief Constructor
   */
  PNeighborDeck(std::string updateCriteria, double sFactor = 1.,
                size_t neighUpdateInterval = 1, double nearBdNodesTol = 0.5)
      : d_updateCriteria(updateCriteria),
        d_sFactor(sFactor),
        d_neighUpdateInterval(neighUpdateInterval),
        d_nearBdNodesTol(nearBdNodesTol) {};

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getExampleJson(std::string updateCriteria = "simple_all", double sFactor = 1.,
                             size_t neighUpdateInterval = 1, double nearBdNodesTol = 0.5) {

    return json({{"Update_Criteria", updateCriteria}, {"Search_Factor", sFactor},
      {"Search_Interval", neighUpdateInterval}, {"Near_Bd_Nodes_Tol", nearBdNodesTol}});
  }

  /*!
 * @brief Reads from json object
 */
  void readFromJson(const json &j) {
    if (j.empty())
      return;

    d_updateCriteria = j.value("Update_Criteria", std::string("simple_all"));
    d_sFactor = j.value("Search_Factor", 1.);
    d_neighUpdateInterval = j.value("Search_Interval", size_t(1));
    d_nearBdNodesTol = j.value("Near_Bd_Nodes_Tol", 0.5);
  }

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
    oss << tabS << "------- PNeighborDeck --------" << std::endl << std::endl;
    oss << tabS << "Update criteria  = " << d_updateCriteria << std::endl;
    oss << tabS << "Search factor = " << d_sFactor << std::endl;
    oss << tabS << "Search update interval = " << d_neighUpdateInterval << std::endl;
    oss << tabS << "Near_Bd_Nodes_Tol = " << d_nearBdNodesTol << std::endl;
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

#endif // INP_PNEIGHBORDECK_H
