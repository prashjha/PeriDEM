/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FE_BASEELEM_H
#define FE_BASEELEM_H

#include "quadData.h"   // definition of QuadData
#include "util/point.h" // definition of Point

namespace fe {

/*!
 * @brief A base class which provides methods to map points to/from reference
 * element and to compute quadrature data
 *
 * - At present, all types of element employ [isoparametric mapping]
 * (http://www.softeng.rl.ac.uk/st/projects/felib4/Docs/html/Intro/intro
 * -node31.html) to map points on reference element to any other element of
 * same type.
 *
 * For any type of element, such as fe::LineElem, fe::TriElem, fe::QuadElem,
 * we have following important points
 *
 * 1. A simple element with predefined vertices is considered as a reference
 * element \f$ T^0 \f$. E.g., reference element in fe::TriElem is a triangle
 * with vertices at \f$(0,0), (1,0), (0,1) \f$.
 *
 * 2. Points in reference element \f$ T^0 \f$ are described by \f$ (\xi, \eta,
 * \zeta)\f$ (in 3-d), \f$ (\xi, \eta) \f$ (in 2-d), and \f$ \xi\f$ (in 1-d).
 * Points in any other element \f$ T \f$ are described by \f$ (x,y,z)\f$ (in
 * 3-d), \f$ (x,y) \f$ (in 2-d), and \f$ x\f$ (in 1-d). Here, we restrict
 * discussion to 3-d element. For 2-d, one should ignore coordinate \f$\zeta \f$
 * and \f$ z\f$, and for 1-d, one should ignore \f$ \eta, \zeta \f$
 * and \f$ y,z\f$.
 *
 * 3. Associated to vertices of the element \f$ T^0 \f$ we have shape
 * functions \f$ N^0_1, N^0_2, ..., N^0_n \f$, where \f$ n \f$ is the number of
 * vertices in the element. Shape functions \f$ N^0_i \f$ are functions of
 * point \f$ (\xi, \eta, \zeta) \in T^0 \f$. For each type of element, \f$ n
 * \f$ is fixed.
 *
 * 4. For any element \f$ T \f$, shape functions are denoted as \f$ N_1, N_2,
 * ..., N_n \f$ and are functions of point \f$ (x,y,z) \in T \f$.
 *
 * 5. Element \f$ T \f$ is described by vertices \f$ v^1, v^2, ..., v^n \f$.
 *
 * 6. A map \f$\Phi : T^0 \to T \f$ , where \f$ T \f$ is a given
 * element formed by vertices \f$ v^1, v^2, ..., v^n \f$, is defined as follows:
 * \f[x(\xi, \eta, \zeta) = \sum_{i=1}^n N^0_i(\xi, \eta, \zeta) v^i_x, \quad
 * y(\xi, \eta, \zeta) = \sum_{i=1}^n N^0_i(\xi, \eta, \zeta) v^i_y, \quad z
 * (\xi, \eta, \zeta) = \sum_{i=1}^n N^0_i(\xi, \eta, \zeta) v^i_z \f]
 * where \f$ v^i_x, v^i_y, v^i_z \f$ are the x, y, and z component of point
 * \f$ v^i\f$.
 *
 * 7. Jacobian of map \f$ \Phi : T^0 \to T \f$ is given by
 * \f[ J = \left[ {
 * \begin{array}{ccc}
 * \frac{dx}{d\xi} &\frac{dy}{d\xi} & \frac{dz}{d\xi} \\
 * \frac{dx}{d\eta} & \frac{dy}{d\eta} & \frac{dz}{d\eta} \\
 * \frac{dx}{d\zeta} & \frac{dy}{d\zeta} & \frac{dz}{d\zeta} \\
 * \end{array}
 * } \right]. \f]
 * For 2-d element it is
 * \f[ J = \left[ {
 * \begin{array}{cc}
 * \frac{dx}{d\xi} &\frac{dy}{d\xi} \\
 * \frac{dx}{d\eta} & \frac{dy}{d\eta} \\
 * \end{array}
 * } \right]. \f]
 * For 1-d element it is
 * \f[ J = \frac{dx}{d\xi}. \f]
 * Determinant of the Jacobian is an important quantity and is used to
 * compute the quadrature points, and inverse map \f$ \Phi^{-1} : T \to T^0 \f$.
 *
 * @sa fe::LineElem, fe::TriElem, fe::QuadElem
 */
class BaseElem {

public:
  /*!
   * @brief Constructor
   *
   * @param order Order of quadrature point approximation
   * @param element_type Type of element
   */
  BaseElem(size_t order, size_t element_type);

  /*!
   * @brief Get element type
   * @return type Type of element
   */
  size_t getElemType() { return d_elemType; }

  /*!
   * @brief Get order of quadrature approximation
   * @return order Order of approximation
   */
  size_t getQuadOrder() { return d_quadOrder; }

  /*!
   * @brief Get number of quadrature points in the data
   * @return N Number of quadrature points
   */
  size_t getNumQuadPoints() { return d_numQuadPts; }

  /*!
   * @brief Returns the size of element (length in 1-d, area in 2-d, volume
   * in 3-d element)
   *
   * @param nodes Vertices of element
   * @return size Size of the element
   */
  virtual double elemSize(const std::vector<util::Point> &nodes) = 0;

  /*!
   * @brief Returns the values of shape function at point p
   *
   * The point p is assumed to be inside an element \f$ T\f$ given by nodes.
   * The idea is to first map point p to the point \f$ p^0\f$ in reference
   * element \f$ T^0 \f$ and then compute shape functions at \f$ p^0 \f$.
   *
   * The map from any element \f$ T \f$ to reference element is \f$ T^0 \f$
   * gets complex for elements like fe::QuadElem. For elements fe:LineElem
   * and fe::TriElem, this is simple to compute. Therefore, this method is
   * only implemented for fe::TriElem and fe::LineElem.
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  virtual std::vector<double>
  getShapes(const util::Point &p, const std::vector<util::Point> &nodes);

  /*!
   * @brief Returns the values of derivative of shape function at point p
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of derivative of shape functions
   */
  virtual std::vector<std::vector<double>>
  getDerShapes(const util::Point &p,
               const std::vector<util::Point> &nodes);

  /*!
   * @brief Get vector of quadrature data
   *
   * Given element vertices, this method returns the list of quadrature point
   * and associated quantities. Here, order of quadrature
   * approximation and element type are set in the constructor. List of data
   * for each quad point:
   * - quad point
   * - quad weight
   * - shape function evaluated at quad point
   * - derivative of shape function evaluated at quad point
   * - Jacobian matrix
   * - determinant of the Jacobian
   *
   * @param nodes Vector of vertices of an element
   * @return vector Vector of QuadData
   */
  virtual std::vector<fe::QuadData>
  getQuadDatas(const std::vector<util::Point> &nodes) = 0;

  /*!
   * @brief Get vector of quadrature data
   *
   * Given element vertices, this method returns the list of quadrature point
   * and essential quantities at quadrature points. Here, order of quadrature
   * approximation and element type are set in the constructor. List of data
   * for each quad point:
   * - quad point
   * - quad weight
   * - shape function evaluated at quad point
   *
   * This function is a lite version of fe::BaseElem::getQuadDatas.
   *
   * @param nodes Vector of vertices of an element
   * @return vector Vector of QuadData
   */
  virtual std::vector<fe::QuadData>
  getQuadPoints(const std::vector<util::Point> &nodes) = 0;

protected:
  /*!
   * @brief Returns the values of shape function at point p on reference
   * element
   *
   * @param p Location of point
   * @return vector Vector of shape functions at point p
   */
  virtual std::vector<double> getShapes(const util::Point &p) = 0;

  /*!
   * @brief Returns the values of derivative of shape function at point p on
   * reference element
   *
   * @param p Location of point
   * @return vector Vector of derivative of shape functions
   */
  virtual std::vector<std::vector<double>>
  getDerShapes(const util::Point &p) = 0;

  /*!
   * @brief Maps point p in a given element to the reference element and
   * returns the mapped point
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  virtual util::Point mapPointToRefElem(const util::Point &p,
                    const std::vector<util::Point> &nodes);

  /*!
   * @brief Computes Jacobian of map from reference element \f$ T^0 \f$ to
   * given element \f$ T \f$
   *
   * @param p Location of point in reference element
   * @param nodes Vertices of element
   * @param J Matrix to store the Jacobian
   * @return det(J) Determinant of the Jacobain
   */
  virtual double getJacobian(const util::Point &p,
                             const std::vector<util::Point> &nodes,
                             std::vector<std::vector<double>> *J) = 0;

  /*!
   * @brief Compute the quadrature points
   *
   * This must be implemented by inheriting classes.
   */
  virtual void init() = 0;

  /*! @brief Order of quadrature point integration approximation */
  size_t d_quadOrder;

  /*! @brief Number of quadrature points for order d_quadOrder */
  size_t d_numQuadPts;

  /*! @brief Element type */
  size_t d_elemType;

  /*! @brief Quadrature data collection */
  std::vector<fe::QuadData> d_quads;
};

} // namespace fe

#endif // FE_BASEELEM_H
