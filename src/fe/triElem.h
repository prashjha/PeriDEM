/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FE_TRIELEM_H
#define FE_TRIELEM_H

#include "baseElem.h" // base class BaseElem

namespace fe {

/*!
 * @brief A class for mapping and quadrature related operations for linear
 * triangle element
 *
 * The reference triangle element \f$ T^0 \f$ is given by vertices
 * \f$ (0,0), \, (1,0), \, (0,1) \f$.
 *
 * 1. The shape functions at point \f$ (\xi, \eta ) \in T^0 \f$ are
 * \f[N^0_1(\xi, \eta) = 1- \xi - \eta, \quad N^0_2(\xi, \eta) = \xi, \quad
 * N^0_3(\xi, \eta) = \eta. \f]
 *
 * 2. For linear triangle element, derivative of shape functions are constant
 * and are as follows
 * \f[\frac{d N^0_1(\xi, \eta)}{d\xi} = -1, \, \frac{d N^0_1(\xi, \eta)
 * }{d\eta} = -1, \f]
 * \f[\frac{d N^0_2(\xi, \eta)}{d\xi} = 1, \, \frac{d N^0_2(\xi, \eta)}{d\eta}
 * = 0, \f]
 * \f[\frac{d N^0_3(\xi, \eta)}{d\xi} = 0, \, \frac{d N^0_3(\xi, \eta)}{d\eta}
 * = 1. \f]
 *
 * 3. Map \f$ \Phi: T^0 \to T\f$ is given by
 * \f[ x(\xi, \eta) = \sum_{i=1}^3 N^0_i(\xi, \eta) v^i_x, \quad y(\xi, \eta)
 * = \sum_{i=1}^3 N^0_i(\xi, \eta) v^i_y \f]
 * where \f$ v^1, v^2, v^3\f$ are vertices of element \f$ T \f$.
 *
 * 4. The Jacobian of the map \f$ \Phi: T^0 \to T\f$ is given by
 * \f[ J = \left[ {
 * \begin{array}{cc}
 * \frac{dx}{d\xi} &\frac{dy}{d\xi} \\
 * \frac{dx}{d\eta} & \frac{dy}{d\eta} \\
 * \end{array}
 * } \right] \f]
 * and determinant of Jacobian is
 * \f[ det(J) = \frac{dx}{d\xi} \times \frac{dy}{d\eta} -
 * \frac{dy}{d\xi}\times \frac{dx}{d\eta}. \f]
 * For linear triangle element, Jacobian (and so \f$ det(J) \f$) is
 * constant. For linear triangle elements, we simply have
 * \f[ \frac{dx}{d\xi} = v^2_x - v^1_x, \quad \frac{dx}{d\eta} = v^3_x -
 * v^1_x, \f]
 * \f[ \frac{dy}{d\xi} = v^2_y - v^1_y, \quad \frac{dy}{d\eta} = v^3_y -
 * v^1_y \f]
 * and \f$ det(J) = \frac{area(T)}{area(T^0)} = 2\times area(T) \f$.
 *
 * 5. Inverse map \f$ \Phi^{-1} : T \to T^9 \f$ for linear triangle element can
 * be easily derived. The derivation of the map is provided below:
 *
 * From map \f$ (\xi, \eta )\in T^0 \to (x,y) \in T \f$ we have
 * \f[ x = \sum_{i=1}^3 N^0_i(\xi, \eta) v^i_x, \quad y = \sum_{i=1}^3
 * N^0_i(\xi, \eta)  v^i_y. \f] Substituting formula for \f$ N^0_i \f$ in above
 * to get \f[ x = (1 - \xi - \eta) v^1_x + \xi v^2_x + \eta v^3_x, \quad y = (1
 * - \xi - \eta) v^1_y + \xi v^2_y + \eta v^3_y \f] or \f[ x - v^1_x = \xi
 * (v^2_x - v^1_x) + \eta (v^3_x - v^1_x), \quad y - v^1_y = \xi (v^2_y - v^1_y)
 * + \eta (v^3_y - v^1_y). \f] Writing above in matrix form, we have
 * \f[ \left[ {\begin{array}{c} x - v^1_x \\
 * y - v^1_y \end{array}}\right] = \left[ {\begin{array}{cc} v^2_x - v^1_x &
 * v^3_x - v^1_x \\
 * v^2_y - v^1_y & v^3_y - v^1_y \end{array}}\right] \, \left[
 * {\begin{array}{c} \xi \\
 * \eta \end{array}}\right]. \f]
 * Denoting the matrix as \f$ B \f$
 * \f[ B = \left[ {\begin{array}{cc} v^2_x - v^1_x & v^3_x - v^1_x \\
 * v^2_y - v^1_y & v^3_y - v^1_y \end{array}}\right] \f]
 * then
 * \f[ C := B^{-1} = \frac{1}{det(B)} \left[ {\begin{array}{cc} v^3_y - v^1_y
 * & -(v^3_x -
 * v^1_x) \\ -(v^2_y - v^1_y) & v^2_x - v^1_x \end{array}}\right]. \f]
 * With \f$ C \f$ we have inverse map given by
 * \f[ \xi(x,y) = C_{11} (x - v^1_x) + C_{12} (y - v^1_y), \quad \eta(x,y) =
 * C_{21} (x - v^1_x) + C_{22} (y - v^1_y). \f]
 * Note that matrix \f$ B \f$ is a transpose of the Jacobian of map \f$
 * \Phi\f$ and hence \f$ det(B) = det(J) \f$.
 *
 */
class TriElem : public BaseElem {

public:
  /*!
   * @brief Constructor
   * @param order Order of quadrature point approximation
   */
  explicit TriElem(size_t order);

  /*!
   * @brief Returns the area of element
   *
   * If triangle \f$ T \f$ is given by points \f$ v^1, v^2, v^3 \f$ then the
   * area is
   * \f[ area(T) = \frac{(v^2_x - v^1_x) (v^3_y - v^1_y) - (v^3_x - v^1_x)
   * (v^2_y - v^1_y)}{2}, \f]
   * where \f$ v^i_x, v^i_y \f$ are the x and y component of point \f$ v^i \f$.
   *
   * Note that area and Jacobian of map \f$ \Phi: T^0 \to T \f$ are related as
   * \f[ area(T) = area (T^0) \times det(J). \f]
   * Here, \f$ area(T^0) = 0.5 \f$.
   *
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  double elemSize(const std::vector<util::Point> &nodes) override;

  /*!
   * @brief Returns the values of shape function at point p
   *
   * We first map the point p in \f$ T \f$ to reference triangle \f$ T^0 \f$
   * using fe::TriElem::mapPointToRefElem and then compute shape functions at
   * the mapped point using fe::TriElem::getShapes(const util::Point &).
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  std::vector<double>
  getShapes(const util::Point &p,
            const std::vector<util::Point> &nodes) override;

  /*!
   * @brief Returns the values of derivative of shape function at point p
   *
   * Below, we present the derivation of the formula:
   *
   * We are interested in \f$ \frac{\partial N_i(x_p, y_p)}{\partial x}\f$
   * and \f$  \frac{\partial N_i(x_p, y_p)}{\partial y}\f$. By using the map
   * \f$ \Phi : T^0 \to T\f$  we have
   * \f[ N^0_i(\xi, \eta) = N_i(x(\xi,\eta), y(\xi, \eta)) \f]
   * and therefore we can write
   * \f[ \frac{\partial N^0_i(\xi, \eta)}{\partial \xi} = \frac{\partial
   * N_i}{\partial x} \frac{\partial x}{\partial \xi} + \frac{\partial
   * N_i}{\partial y} \frac{\partial y}{\partial \xi} \f]
   * and
   * \f[ \frac{\partial N^0_i(\xi, \eta)}{\partial \eta} = \frac{\partial
   * N_i}{\partial x} \frac{\partial x}{\partial \eta} + \frac{\partial
   * N_i}{\partial y} \frac{\partial y}{\partial \eta} \f]
   * which can be written in the matrix form as
   * \f[ \left[ {\begin{array}{c} \frac{\partial N^0_i}{\partial \xi} \\
   * \frac{\partial N^0_i}{\partial \eta} \end{array}}\right] =
   * \left[ {\begin{array}{cc} \frac{\partial x}{\partial \xi} &
   * \frac{\partial y}{\partial \xi} \\
   * \frac{\partial x}{\partial \eta} & \frac{\partial y}{\partial \eta}
   * \end{array}}\right] \, \left[ {\begin{array}{c} \frac{\partial
   * N_i}{\partial x} \\
   * \frac{\partial N_i}{\partial y} \end{array}}\right]. \f]
   * The matrix is the Jacobian matrix \f$ J \f$ and can be computed easily
   * if vertices of elements are known. Inverse \f$ J^{-1} \f$ is given by
   * \f[ J^{-1} = \frac{1}{det(J)} \left[ {\begin{array}{cc} \frac{\partial
   * y}{\partial \eta} & -\frac{\partial y}{\partial \xi} \\
   * -\frac{\partial x}{\partial \eta} & \frac{\partial x}{\partial \xi}
   * \end{array}}\right]. \f]
   * Using \f$ J^{-1} \f$ we have following formula for derivatives of the
   * shape function
   * \f[ \left[ {\begin{array}{c} \frac{\partial
   * N_i}{\partial x} \\
   * \frac{\partial N_i}{\partial y} \end{array}}\right] = J^{-1}
   * \left[ {\begin{array}{c} \frac{\partial N^0_i}{\partial \xi} \\
   * \frac{\partial N^0_i}{\partial \eta} \end{array}}\right]. \f]
   * Here, derivatives \f$ \frac{\partial
   * N^0_i}{ \partial \xi} \f$ and \f$ \frac{\partial N^0_i} {\partial \eta }
   * \f$ correspond to reference element and are easy to compute.
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of derivative of shape functions
   */
  std::vector<std::vector<double>>
  getDerShapes(const util::Point &p,
               const std::vector<util::Point> &nodes) override;

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
   * Let \f$ T \f$ is the given triangle with vertices \f$ v^1, v^2, v^3 \f$
   * and let \f$ T^0 \f$ is the reference triangle.
   *
   * 1. To compute quadrature points, we first compute the quadrature points
   * on reference triangle \f$ T^0 \f$, and then we use the map \f$ \Phi: T^0 \to
   * T\f$ to map the points on reference triangle to the given triangle \f$ T
   * \f$.
   *
   * 2. To compute the quadrature weight, we compute the quadrature weight
   * associated to the quadrature point in reference triangle \f$ T^0 \f$.
   * Suppose \f$ w^0_q \f$ is the quadrature weight associated to quadrature
   * point \f$ (\xi_q, \eta_q) \in T^0 \f$, then the quadrature point \f$
   * w_q \f$ associated to the mapped point \f$ (x(\xi_q, \eta_q), y(\xi_q,
   * \eta_q)) \in T \f$ is given by
   * \f[ w_q = w^0_q * det(J) \f]
   * where \f$ det(J) \f$ is the determinant of the Jacobian of map \f$ \Phi \f$.
   *
   * 3. We compute shape functions \f$ N_1, N_2, N_3\f$ associated to \f$ T
   * \f$ at the quadrature point \f$ (x(\xi_q, \eta_q), y(\xi_q,\eta_q)) \f$
   * using formula
   * \f[ N_i(x(\xi_q, \eta_q), y(\xi_q, \eta_q)) = N^0_i(\xi_q, \eta_q). \f]
   *
   * 5. To compute derivative of shape functions such as \f$ \frac{\partial
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
   * This function is lite version of fe::TriElem::getQuadDatas.
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
   * @brief Maps point p in a given element to the reference element
   *
   * Let \f$ v^1, v^2, v^3\f$ are three vertices of triangle \f$ T\f$ and let
   * \f$T^0 \f$ is the reference triangle. Following the introduction to
   * fe::TriElem, the map \f$ (x,y) \in T \f$ to \f$ (\xi, \eta) \in T^0 \f$ is
   * given by
   * \f[ \xi = C_{11} (x - v^1_x) + C_{12} (y - v^1_y), \quad \eta = C_{21} (x -
   * v^1_x) + C_{22} (y - v^1_y). \f]
   * \f$ C\f$ is the inverse of matrix
   * \f[ B = \left[ {\begin{array}{cc} v^2_x - v^1_x & v^3_x - v^1_x \\
   * v^2_y - v^1_y & v^3_y - v^1_y \end{array}}\right], \f]
   * i.e.
   * \f[ C := B^{-1} = \frac{1}{(v^2_x - v^1_x)(v^3_y - v^1_y) - (v^3_x - v^1_x)
   * (v^2_y - v^1_y)} \left[ {\begin{array}{cc} v^3_y - v^1_y & -(v^3_x -
   * v^1_x) \\ -(v^2_y - v^1_y) & v^2_x - v^1_x \end{array}}\right]. \f]
   *
   * If mapped point \f$ (\xi, \eta)\f$ does not satisfy following
   * conditions
   * - \f[ 0\leq \xi, \eta \f]
   * - \f[ \xi \leq 1 - \eta \quad (or\, equivalently) \quad \eta \leq 1 -
   * \xi \f]
   * then the point \f$ (\xi,\eta) \f$ does not belong to the reference triangle
   * or equivalently point \f$(x,y) \f$ does not belong to the triangle \f$ T
   * \f$ and the method issues error. Otherwise the method returns point \f$
   * (\xi, \eta)\f$.
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  util::Point mapPointToRefElem(const util::Point &p,
                    const std::vector<util::Point> &nodes) override;

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
   * @brief Compute the quadrature points for triangle element
   */
  void init() override;
};

} // namespace fe

#endif // FE_TRIELEM_H
