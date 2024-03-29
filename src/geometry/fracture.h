/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef GEOM_FRACTURE_H
#define GEOM_FRACTURE_H

#include "util/point.h" // definition of Point
#include <stdint.h> // uint8_t type
#include <string.h> // size_t type
#include <vector>

/*! @brief Collection of methods and data related to geometry */
namespace geometry {

/*! @brief A class for fracture state of bonds
 *
 * This class provides method to read and modify fracture state of bonds
 */
class Fracture {

public:
  /*!
   * @brief Constructor
   *
   * If neighbor list is null, then it assumes all nodes interact with all
   * other nodes (PeriDEM implementation).
   *
   * @param nodes Pointer to nodal coordinates
   * @param neighbor_list Pointer to neighbor list
   */
  Fracture(const std::vector<util::Point> *nodes,
           const std::vector<std::vector<size_t>> *neighbor_list = nullptr);

  /*!
   * @brief Constructor
   */
  Fracture();

  /*!
   * @brief Sets the bond state
   *
   * @param i Nodal id
   * @param j Local id of bond in neighbor list of i
   * @param state State which is applied to the bond
   */
  void setBondState(const size_t &i, const size_t &j, const bool &state);

  /*!
   * @brief Read bond state
   *
   * @param i Nodal id
   * @param j Local id of bond in neighbor list of i
   * @return bool True if bond is fractured otherwise false
   */
  bool getBondState(const size_t &i, const size_t &j) const;

  /*!
   * @brief Returns the list of bonds of node i
   *
   * @param i Nodal id
   * @return list Bonds of node i
   */
  const std::vector<uint8_t> &getBonds(const size_t &i) const;
  std::vector<uint8_t> &getBonds(const size_t &i);

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt = 0, int lvl = 0) const;
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }

private:

  /*! @brief Vector which stores the state of bonds
   *
   * Given node i, vector d_fracture[i] is the list of state of bonds of node
   * i.
   *
   * We only use 1 bit per bond of node to store the state.
   */
  std::vector<std::vector<uint8_t>> d_fracture;
};

} // namespace geometry

#endif // GEOM_FRACTURE_H
