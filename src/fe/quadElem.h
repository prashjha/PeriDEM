/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
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

  /*!
   * @brief Get vector of quadrature data
   *
   * Given element vertices, this method returns the list of quadrature point
   * and essential quantities at quadrature points. Here, order of quadrature
   * approximation is set in the constructor. List of data
   * - quad point
   * - quad weight
   * - shape function evaluated at quad point
   * - derivative of shape function evaluated at quad point
   * - Jacobian matrix
   * - determinant of the Jacobian
   *
   * Let \f$ T \f$ is the given element with vertices \f$ v^1, v^2, v^3, v^4 \f$
   * and let \f$ T^0 \f$ is the reference element.
   *
   * 1. To compute quadrature point, we first compute the quadrature points
   * on \f$ T^0 \f$, and then map the point on \f$ T^0 \f$ to the point on \f$ T
   * \f$ using the map \f$ \Phi: T^0 \to T\f$.
   *
   * 2. To compute the quadrature weight, we compute the quadrature weight
   * associated to the quadrature point in reference triangle \f$ T^0 \f$.
   * Suppose \f$ w^0_q \f$ is the quadrature weight associated to quadrature
   * point \f$ (\xi_q, \eta_q) \in T^0 \f$, then the quadrature point \f$
   * w_q \f$ associated to the mapped point \f$ (x(\xi_q, \eta_q), y(\xi_q,
   * \eta_q)) \in T \f$ is given by
   * \f[ w_q = w^0_q * det(J(\xi_q, \eta_q)) \f]
   * where \f$ det(J) \f$ is the determinant of the Jacobian \f$ J \f$ of map
   * \f$ \Phi: T^0 \to T \f$. \f$ J(\xi_q,
   * \eta_q)\f$ is the value of \f$ J \f$ at point \f$ (\xi_q, \eta_q)
   * \in T^0 \f$.
   *
   * 3. We compute shape functions \f$ N_1, N_2, N_3, N_4\f$ associated to \f$ T
   * \f$ at the quadrature point \f$ (x(\xi_q, \eta_q), y(\xi_q,\eta_q)) \f$
   * using formula
   * \f[ N_i(x(\xi_q, \eta_q), y(\xi_q, \eta_q)) = N^0_i(\xi_q, \eta_q). \f]
   *
   * 5. To compute the derivatives of shape functions such as \f$ \frac{\partial
   * N_i}{\partial x}, \frac{\partial N_i}{\partial y}\f$ associated to \f$ T
   * \f$, we use the relation between derivatives of shape function in \f$ T
   * \f$ and \f$ T^0 \f$ described in fe::TriElem::getDerShapes.
   *
   * @param nodes Vector of vertices of an element
   * @return vector Vector of QuadData
   */
  std::vector<fe::QuadData>
  getQuadDatas(const std::vector<util::Point> &nodes) override;

  /*!
   * @brief Get vector of quadrature data
   *
   * Given element vertices, this method returns the list of quadrature point
   * and essential quantities at quadrature points. Here, order of quadrature
   * approximation is set in the constructor. List of data
   * - quad point
   * - quad weight
   * - shape function evaluated at quad point
   *
   * This function is lite version of QuadElem::getQuadDatas.
   *
   * @param nodes Vector of vertices of an element
   * @return vector Vector of QuadData
   */
  std::vector<fe::QuadData>
  getQuadPoints(const std::vector<util::Point> &nodes) override;

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
