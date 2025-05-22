/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTIL_TRANSFORMATION_H
#define UTIL_TRANSFORMATION_H

#include "point.h"              // definition of Point
#include <vector>

namespace util {

/**
   * @name Rotation
   */
/**@{*/

/*!
 * @brief Rotates a vector in xy-plane in clockwise direction
 * @param x Point
 * @param theta Angle
 * @return Point after rotation
 */
std::vector<double> rotateCW2D(const std::vector<double> &x,
                               const double &theta);

/*!
 * @brief Rotates a vector in xy-plane in clockwise direction
 * @param x Point
 * @param theta Angle
 * @return Point after rotation
 */
util::Point rotateCW2D(const util::Point &x, const double &theta);

/*!
 * @brief Rotates a vector in xy-plane in anti-clockwise direction
 * @param x Point
 * @param theta Angle
 * @return Point after rotation
 */
std::vector<double> rotateACW2D(const std::vector<double> &x,
                                const double &theta);

/*!
 * @brief Rotates a vector in xy-plane in anti-clockwise direction
 * @param x Point
 * @param theta Angle
 * @return Point after rotation
 */
util::Point rotateACW2D(const util::Point &x, const double &theta);

/*!
 * @brief Rotates a vector in xy-plane assuming ACW convention
 * @param x Point
 * @param theta Angle
 * @return Point after rotation
 */
std::vector<double> rotate2D(const std::vector<double> &x, const double &theta);

/*!
 * @brief Rotates a vector in xy-plane assuming ACW convention
 * @param x Point
 * @param theta Angle
 * @return Point after rotation
 */
util::Point rotate2D(const util::Point &x, const double &theta);

/*!
 * @brief Computes derivative of rotation wrt to time
 *
 * If \f$ R(x,t) = Q(at)x \f$ then \f$ dR/dt = a Q' x \f$. This function returns \f$ Q' x \f$.
 *
 * @param x Point
 * @param theta Angle
 * @return Point after rotation
 */
util::Point derRotate2D(const util::Point &x, const double &theta);

/*!
 * @brief Returns the vector after rotating by desired angle
 *
 * @param p Vector
 * @param theta Angle of rotation
 * @param axis Axis of rotation
 * @return x Vector after rotation
 */
util::Point rotate(const util::Point &p, const double &theta, const util::Point &axis);

/*!
 * @brief Computes angle between two vectors
 * @param a Vector 1
 * @param b Vector 2
 * @return angle Angle between vector a and b
 */
double angle(util::Point a, util::Point b);

/*!
 * @brief Computes angle between two vectors
 * @param a Vector 1
 * @param b Vector 2
 * @param axis Axis of rotation
 * @param is_axis If true then axis is the axis of orientation, otherwise
 * axis specifies the +ve side of the plane in which a and b are
 * @return angle Angle between vector a and b
 */
double angle(util::Point a, util::Point b, util::Point axis, bool is_axis = true);

/** @}*/

} // namespace util

#endif // UTIL_TRANSFORMATION_H
