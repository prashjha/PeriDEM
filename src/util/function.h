/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTIL_FUNCTION_H
#define UTIL_FUNCTION_H

#include "point.h"              // definition of Point
#include <vector>

// tolerance for float comparison
#define COMPARE_EPS 1e-5

namespace util {

/*!
 * @brief Returns true if a > b
 * @param a Value a
 * @param b Value b
 * @return True if a is definitely greater than b
 */
bool isGreater(const double &a, const double &b);

/*!
 * @brief Returns true if a < b
 * @param a Value a
 * @param b Value b
 * @return True if a is definitely less than b
 */
bool isLess(const double &a, const double &b);

/*!
 * @brief Computes hat function at given point
 *
 * Hat function:
 *      f ^
 *        |
 *        |
 *     1  o
 *        |           /|\
 *        |         /  |  \
 *        |       /    |    \
 *        |     /      |      \
 *        |   /        |        \
 *        | /          |          \
 *        o____________o____________o______\ x
 *                                         /
 *      x_min                      x_max
 *
 * @param x Point in real line
 * @param x_min Left side point in real line
 * @param x_max Right side point in real line
 * @return value Evaluation of hat function at x
 */
double hatFunction(const double &x, const double &x_min, const double &x_max);

/*!
 * @brief Computes hat function at given point
 *
 * This version does not test if point x is in valid interval.
 *
 * Hat function:
 *      f ^
 *        |
 *        |
 *     1  o
 *        |           /|\
 *        |         /  |  \
 *        |       /    |    \
 *        |     /      |      \
 *        |   /        |        \
 *        | /          |          \
 *        o____________o____________o______\ x
 *                                         /
 *      x_min                      x_max
 *
 * @param x Point in real line
 * @param x_min Left side point in real line
 * @param x_max Right side point in real line
 * @return value Evaluation of hat function at x
 */
double hatFunctionQuick(const double &x, const double &x_min,
                        const double &x_max);

/*!
 * @brief Compute linear step function
 *
 * Step function:
 *
 * f ^
 *   |             __________
 *   |            /
 *   |           /
 *   |   _______/
 *   |  /
 *   | /
 *   |/_________________________ t
 *       x1   x1+x2
 *
 *  - Linear (with slope 1) in [0,l1), constant in [l1,l1+l2)
 *  - Periodic with periodicity l1+l2
 *
 * @param x  Point in real line
 * @param x1 Point such that function is linear with slope 1 in [0, x1)
 * @param x2 Point such that function is constant in [x1, x1 + x2)
 * @return value Evaluation of step function at x
 */
double linearStepFunc(const double &x, const double &x1, const double &x2);

/*!
 * @brief Compute gaussian function in 1-d
 *
 * Guassian (1-d) function: \f$ f(r) = a \exp(-\frac{r^2}{\beta}). \f$
 *
 * Here \f$ a\f$ is the amplitude and \f$ \beta \f$ is the exponential factor.
 *
 * @param r Distance from origin
 * @param a Amplitude
 * @param beta Factor in exponential function
 * @return value Component of guassian 1-d function
 */
double gaussian(const double &r, const double &a, const double &beta);

/*!
 * @brief Compute gaussian function in 2-d
 *
 * Guassian (2-d) function:
 * \f[ f(x,y) = (f_1(x,y), f_2(x,y)), \f]
 * where
 * \f[ f_1(x,y) =  a \exp(-\frac{(x-x_c)^2 + (y-y_c)^2}{\beta}) d_1,
 * \quad f_1(x,y) =  a \exp(-\frac{(x-x_c)^2 + (y-y_c)^2}{\beta}) d_2.
 * \f]
 * Here \f$ (x_c,y_c) \f$ is the center of the pulse, \f$ a\f$ is the
 * amplitude, \f$ \beta \f$ is the exponential factor, and \f$ (d_1,d_2)\f$
 * is the direction of the pulse.
 *
 * @param x  Coordinates of point
 * @param params List of parameters
 * @param dof Component of guassian function
 * @return value Component of guassian 2-d vector function along dof
 */
double gaussian2d(const util::Point &x, const size_t &dof,
                  const std::vector<double> &params);

/*!
 * @brief Compute sum of two gaussian function in 2-d
 *
 * Double guassian (2-d) function:
 * \f[ f(x,y) = (f_1(x,y), f_2(x,y)) + (g_1(x,y), g_2(x,y)), \f]
 * where \f$ (f_1,f_2)\f$ and \f$(g_1, g_2)\f$ are two guassian 2-d function
 * as described in guassian2d() with different values of \f$ (x_c, y_c), a,
 * (d_1, d_2)\f$.
 *
 * @param x  Coordinates of point
 * @param params List of parameters
 * @param dof Component of guassian function
 * @return value Component of guassian 2-d vector function along dof
 */
double doubleGaussian2d(const util::Point &x, const size_t &dof,
                        const std::vector<double> &params);

/*!
 * @brief Compute harmonic mean of m1 and m2
 *
 * @param m1 Mass 1
 * @param m2 Mass 2
 * @return m Harmonic mean
 */
double equivalentMass(const double &m1, const double &m2);

double harmonicMean(const double &m1, const double &m2);

/*!
 * @brief Silling normal contact stiffness from two bulk moduli and the horizon
 *
 * \f[ K_n = \frac{18\,\bar{K}}{\pi \delta^{p}}, \quad
 *     \bar{K} = \mathrm{harmonicMean}(K_1, K_2) \f]
 *
 * The contact force density in PairForce is \f$ K_n V_j \, \mathrm{overlap}
 * \f$, so \f$ K_n \f$ carries one power of the horizon per spatial
 * dimension of the nodal weight: \f$ p = 5 \f$ with 3D weights \f$ h^3 \f$,
 * \f$ p = 4 \f$ with 2D weights \f$ h^2 \cdot 1\,\mathrm{m} \f$.
 *
 * @note The two-dimensional examples and the internal-contact stiffness in
 * BaseParticle use \f$ p = 5 \f$, while the notched-impact driver uses
 * \f$ p = 4 \f$ in two dimensions. That difference is older than this
 * function. The exponent is an argument so that each caller keeps the value it
 * had, and changing the default would change the contact force in every
 * example.
 *
 * @param K1 Bulk modulus of the first body
 * @param K2 Bulk modulus of the second body
 * @param horizon Peridynamic horizon
 * @param horizonPower Power of the horizon in the denominator
 * @return Kn Normal contact stiffness
 */
double normalContactStiffness(const double &K1, const double &K2,
                              const double &horizon, int horizonPower = 5);

/*!
 * @brief Contact stiffness for a body against itself
 *
 * \f[ K_n = \frac{18}{\pi \delta^{p}} K \f]
 *
 * Analytically this equals normalContactStiffness(K, K, horizon), because
 * harmonicMean(K, K) == K. The two are separate functions because the order of
 * operations is not the same in floating point. BaseParticle evaluates
 * \f$ (18 / (\pi \delta^5)) \cdot K \f$. Over the 36 combinations of bulk
 * modulus and horizon that the examples and tests use, the regrouped form
 * \f$ 18 K / (\pi \delta^5) \f$ differs from it in the last bit in 8 cases, by
 * 2e-16 relative. This function keeps the original order of operations so that
 * the internal contact stiffness is unchanged.
 *
 * @param K Bulk modulus
 * @param horizon Peridynamic horizon
 * @param horizonPower Power of the horizon in the denominator
 * @return Kn Normal contact stiffness
 */
double selfContactStiffness(const double &K, const double &horizon,
                            int horizonPower = 5);

} // namespace util

#endif // UTIL_FUNCTION_H
