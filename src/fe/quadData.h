/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef FE_QUADDATA_H
#define FE_QUADDATA_H

#include <util/io.h>
#include <util/point.h> // definition of Point
#include <vector>

namespace fe {

/*!
 * @brief A struct to store the quadrature data. List of data are
 */
struct QuadData {

  /*! @brief Quadrature weight */
  double d_w;

  /*! @brief Quadrature point in 1-d, 2-d or 3-d */
  util::Point d_p;

  /*!
   * @brief Value of shape functions at quad point p.
   *
   * Size will be the number of vertices the element has. E.g. for triangle
   * element shapes will have three entries.
   */
  std::vector<double> d_shapes;

  /*!
   * @brief Value of derivative of shape functions at quad point p.
   *
   * Size will be the number of vertices the element has. E.g. for triangle
   * element shapes will have three entries.
   *
   * x-derivative of ith shape function is d_derShapes[i][0]
   *
   * y-derivative of ith shape function is d_derShapes[i][1]
   *
   */
  std::vector<std::vector<double>> d_derShapes;

  /*!
   * @brief Jacobian of the map from reference element to the element
   *
   * In 1-d, size is 1, 2-d size is \f$ 2\times 2\f$, and in 3-d size is \f$
   * 3\times 3\f$.
   */
  std::vector<std::vector<double>> d_J;

  /*!
   * @brief Determinant of the Jacobian of the map from reference element to
   * the element
   */
  double d_detJ;

  /*!
   * @brief Constructor
   */
  QuadData() : d_w(0.), d_p(util::Point()), d_detJ(0.){};

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
    oss << tabS << "------- QuadData --------" << std::endl << std::endl;
    oss << tabS << "Weight = " << d_w << std::endl;
    oss << tabS << "Point = " << d_p.printStr() << std::endl;
    oss << tabS << "Shapes = " << util::io::printStr(d_shapes, 0) << std::endl;
    oss << tabS << "Derivative = " << util::io::printStr(d_derShapes, 0)
        << std::endl;
    oss << tabS << "Jacobian = " << util::io::printStr(d_J, 0)
        << std::endl;
    oss << tabS << "Det(J) = " << d_detJ << std::endl;
    oss << std::endl;

    return oss.str();
  };

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); };
};

} // namespace fe

#endif // FE_QUADDATA_H
