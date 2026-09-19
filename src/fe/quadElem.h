/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef FE_QUADELEM_H
#define FE_QUADELEM_H

#include "baseElem.h"     // base class BaseElem

namespace fe {

/*!
 * @brief A class for mapping and quadrature related operations for bi-linear
 * quadrangle element
 *
 * The reference quadrangle element \f$ T^0 \f$ is given by vertices
 * \f$ (-1, -1), \, (1,-1), \, (1,1), \, (-1,1) \f$.
 *
 * 1. The shape functions at point \f$ (\xi, \eta ) \in T^0 \f$ are
 * \f[N^0_1(\xi, \eta) = \frac{(1- \xi)(1 - \eta)}{4}, \quad N^0_2(\xi, \eta) =
 * \frac{(1+ \xi)(1 - \eta)}{4}, \f]
 * \f[
 * N^0_3(\xi, \eta) = \frac{(1+ \xi)(1 + \eta)}{4},\quad N^0_4(\xi, \eta) =
 * \frac{(1- \xi)(1 + \eta)}{4}. \f]
 *
 * 2. Derivative of shape functions at point \f$ (\xi, \eta ) \in T^0 \f$ are
 * as follows
 * \f[\frac{d N^0_1(\xi, \eta)}{d\xi} = \frac{-(1 - \eta)}{4}, \quad \frac{d
 * N^0_1(\xi, \eta)}{d\eta} = \frac{-(1 - \xi)}{4}, \f]
 * \f[\frac{d N^0_2(\xi, \eta)}{d\xi} = \frac{(1 - \eta)}{4}, \quad \frac{d
 * N^0_2
 * (\xi, \eta)}{d\eta} = \frac{-(1 + \xi)}{4}, \f]
 * \f[\frac{d N^0_3(\xi, \eta)}{d\xi} = \frac{(1 + \eta)}{4}, \quad \frac{d
 * N^0_3
 * (\xi, \eta)}{d\eta} = \frac{(1 + \xi)}{4}, \f]
 * \f[\frac{d N^0_4(\xi, \eta)}{d\xi} = \frac{-(1 + \eta)}{4}, \quad \frac{d
 * N^0_4
 * (\xi, \eta)}{d\eta} = \frac{(1 - \xi)}{4}. \f]
 *
 * 3. Map \f$ \Phi: T^0 \to T\f$ is given by
 * \f[ x(\xi, \eta) = \sum_{i=1}^4 N^0_i(\xi, \eta) v^i_x, \quad y(\xi, \eta)
 * = \sum_{i=1}^4 N^0_i(\xi, \eta) v^i_y \f]
 * where \f$ v^1, v^2, v^3, v^4\f$ are vertices of element \f$ T \f$.
 *
 * 4. Jacobian of the map \f$ \Phi: T^0 \to T\f$ is
 * given by
 * \f[ J = \left[ {
 * \begin{array}{cc}
 * \frac{dx}{d\xi} &\frac{dy}{d\xi} \\
 * \frac{dx}{d\eta} & \frac{dy}{d\eta} \\
 * \end{array}
 * } \right] \f]
 * and determinant of Jacobian is
 * \f[ det(J) = \frac{dx}{d\xi} \times \frac{dy}{d\eta} -
 * \frac{dy}{d\xi}\times \frac{dx}{d\eta}. \f]
 *
 */
class QuadElem : public BaseElem {

public:
  /*!
   * @brief Constructor for quadrangle element
   * @param order Order of quadrature point approximation
   */
  explicit QuadElem(size_t order);

  /*!
   * @brief Returns the area of element
   *
   * If quadrangle \f$ T \f$ is given by points \f$ v^1, v^2, v^3, v^4 \f$ then
   * the area is
   * \f[ area(T) = \frac{(-v^1_1 + v^2_1 + v^3_1 - v^4_1) (-v^1_2 - v^2_2 +
   * v^3_2 + v^4_2) - (-v^1_1 - v^2_1 + v^3_1 + v^4_1) (-v^1_2 + v^2_2 + v^3_2
   * - v^4_2)}{4}, \f]
   * where \f$ v^i_1, v^i_2 \f$ are the x and y component of point \f$ v^i \f$.
   *
   * Note that area and Jacobian of map \f$ \Phi: T^0 \to T \f$ are related as
   * \f[ area(T) = area(T^0) \times det(J(\xi = 0, \eta = 0)), \f]
   * where \f$ area(T^0) = 4 \f$ and \f$ J(\xi = 0, \eta = 0) \f$ is the
   * Jacobian of map at point \f$ (0,0) \in T^0 \f$.
   *
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  double elemSize(const std::vector<util::Point> &nodes) override;

private:
  /*!
   * @brief Returns the values of shape function at point p on reference element
   *
   * @param p Location of point
   * @return vector Vector of shape functions at point p
   */
  std::vector<double> getShapes(const util::Point &p) override;

  /*!
   * @brief Returns the values of derivative of shape function at point p on
   * reference element
   *
   * @param p Location of point
   * @return vector Vector of derivative of shape functions
   */
  std::vector<std::vector<double>> getDerShapes(const util::Point &p) override;

  /*!
   * @brief Computes the Jacobian of map \f$ \Phi: T^0 \to T \f$
   *
   * @param p Location of point in reference element
   * @param nodes Vertices of element
   * @param J Matrix to store the Jacobian (if not nullptr)
   * @return det(J) Determinant of the Jacobain
   */
  double getJacobian(const util::Point &p,
                     const std::vector<util::Point> &nodes,
                     std::vector<std::vector<double>> *J) override;

  /*!
   * @brief Compute the quadrature points for quadrangle element
   */
  void init() override;

};

} // namespace fe

#endif // FE_QUADELEM_H
