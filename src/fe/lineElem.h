/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */
#ifndef FE_LINEELEM_H
#define FE_LINEELEM_H

#include "baseElem.h"     // base class BaseElem

namespace fe {

/*!
 * @brief A class for mapping and quadrature related operations for linear
 * 2-node line element
 *
 * The reference line element \f$T^0 \f$ is given by vertices at \f$
 * v^1 = -1, v^2 = 1 \f$.
 *
 * 1. The shape functions at point \f$ \xi \in T^0 \f$ are
 * \f[N^0_1(\xi) = \frac{1 - \xi}{2}, \quad N^0_2(\xi) = \frac{1 + \xi}{2}. \f]
 *
 * 2. Derivative of shape functions are constant and are as follows
 * \f[\frac{d N^0_1(\xi)}{d\xi} = \frac{-1}{2}, \, \frac{d N^0_2(\xi)}{d\xi}
 * = \frac{1}{2}. \f]
 *
 * 3. Map \f$ \Phi: T^0 \to T \f$ is given by
 * \f[ x(\xi) = \sum_{i=1}^2 N^0_i(\xi) v^i_x, \f]
 * where \f$ v^1, v^2\f$ are vertices of element \f$ T \f$. For 1-d
 * points, \f$ v^i_x = v^i \f$.
 *
 * 4. The Jacobian of the map \f$ \Phi: T^0 \to T \f$ is given by
 * \f[ J = \frac{dx}{d\xi}. \f]
 * Since it is a 1-d case, Jacobian and its determinant are same. For line
 * element the formula for Jacobian is as follows
 * \f[ J = \frac{dx}{d\xi} = \frac{v^2_x - v^1_x}{2} = \frac{length(T)
 * }{length(T^0)}. \f]
 *
 * 5. Inverse map \f$ \Phi^{-1} \f$ from \f$ x \in T \f$ to \f$ \xi \in T^0\f$
 * is given by
 * \f[ \xi(x) = \frac{2}{v^2_x - v^1_x} (x - \frac{v^2_x + v^1_x}{2}) =
 * \frac{1}{J}(x - \frac{v^2_x + v^1_x}{2}). \f]
 *
 */
class LineElem : public BaseElem {

public:
  /*!
   * @brief Constructor for line element
   * @param order Order of quadrature point approximation
   */
  explicit LineElem(size_t order);

  /*!
   * @brief Returns the length of element
   *
   * If line \f$ T \f$ is given by points \f$ v^1, v^2\f$ then the length is
   * \f[ length(T) = v^2_x - v^1_x. \f]
   *
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  double elemSize(const std::vector<util::Point> &nodes) override;

  /*!
   * @brief Returns the values of shape function at point p
   *
   * Line \f$ T \f$ is given by points \f$ v^1, v^2\f$. We first map
   * the point p in \f$ T \f$ to reference line \f$ T^0 \f$ using
   * fe::LineElem::mapPointToRefElem and then compute shape functions at the
   * mapped point using fe::LineElem::getShapes(const util::Point &).
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
   * Let \f$ x\f$ is the point on line \f$ T \f$ and
   * let \f$ \xi \f$ is the point on reference line \f$ T^0 \f$.
   * Let shape functions on \f$ T\f$ are \f$ N_1, N_2 \f$ and
   * on \f$ T^0 \f$ are \f$ N^0_1, N^0_2 \f$.
   *
   * We are interested in \f$ \frac{\partial N_i(x_p)}{\partial x}\f$. By
   * using the map \f$ \xi \to x \f$ we have \f[ N^0_i(\xi) = N_i(x(\xi)) \f]
   * and therefore we can write
   * \f[ \frac{\partial N^0_i(\xi)}{\partial \xi} = \frac{\partial
   * N_i}{\partial x} \frac{\partial x}{\partial \xi}. \f]
   * Since \f$ \frac{\partial x}{\partial \xi} = J \f$ is a Jacobian of map
   * which follows from \f$ v^1, v^2 \f$, we can
   * invert and obtain the formula
   * \f[ \frac{\partial N_i(\xi)}{\partial x} = \frac{1}{J} \frac{\partial
   * N^0_i(\xi)}{\partial \xi}. \f]
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of derivative of shape functions
   */
  std::vector<std::vector<double>>
  getDerShapes(const util::Point &p,
               const std::vector<util::Point> &nodes) override;

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
   * Let \f$ v^1, v^2\f$ are vertices of element \f$ T\f$ and let
   * \f$T^0 \f$ is the reference element. Map \f$\Phi : T^0 \to T\f$ is given by
   * \f[ \xi(x) = \frac{2}{v^2_x - v^1_x} (x - \frac{v^2_x + v^1_x}{2}) =
   * \frac{1}{J}(x - \frac{v^2_x + v^1_x}{2}). \f]
   *
   * If mapped point \f$ \xi\f$ does not satisfy condition
   * - \f[ -1 \leq \xi \leq 1 \f]
   * then the point \f$ \xi \f$ does not belong to reference line \f$ T^0\f$
   * or equivalently point \f$x  \f$ does not belong to line \f$ T \f$
   * and the method issues error. Otherwise the method returns point \f$ \xi\f$.
   *
   * @param p Location of point
   * @param nodes Vertices of element
   * @return vector Vector of shape functions at point p
   */
  util::Point mapPointToRefElem(const util::Point &p,
                    const std::vector<util::Point> &nodes) override;

  /*!
   * @brief Computes Jacobian of the map \f$ \Phi: T^0 \to T\f$
   *
   * @param p Location of point in reference element
   * @param nodes Vertices of element
   * @param J Matrix to store the Jacobian (if not nullptr)
   * @return det(J) Determinant of the Jacobain (same as Jacobian in 1-d)
   */
  double getJacobian(const util::Point &p,
                     const std::vector<util::Point> &nodes,
                     std::vector<std::vector<double>> *J) override;

  /*!
   * @brief Compute the quadrature points for line element
   */
  void init() override;

};

} // namespace fe

#endif // FE_LINEELEM_H
