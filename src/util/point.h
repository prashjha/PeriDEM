/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef UTIL_POINT_H
#define UTIL_POINT_H

#include <cmath>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>

/*!
 * @brief Collection of methods useful in simulation
 *
 * This namespace provides number of useful functions and struct definition.
 *
 * @sa Point, Matrix3, SymMatrix3, compare, transformation
 */
namespace util {

/*! @brief A structure to represent 3d vectors */
struct Point {

  /*! @brief the x coordinate */
  double d_x;

  /*! @brief the y coordinate */
  double d_y;

  /*! @brief the z coordinate */
  double d_z;

  /*!
   * @brief Constructor
   */
  Point() : d_x(0.), d_y(0.), d_z(0.){};

  /*!
   *  @brief Constructor
   *  @param x The x coordinate
   *  @param y The y coordinate
   *  @param z The z coordinate
   */
  template <class T> Point(T x, T y, T z) : d_x(x), d_y(y), d_z(z){};

  /*!
   *  @brief Constructor
   *  @param x The coordinate vector
   */
  template <class T>
  explicit Point(T x[3]) : d_x(x[0]), d_y(x[1]), d_z(x[2]){};

  /*!
   *  @brief Constructor
   *  @param p Vector
   */
  explicit Point(const std::vector<double> &p) {

    if (p.empty())
      return;
    else if (p.size() == 1)
      d_x = p[0];
    else if (p.size() == 2) {
      d_x = p[0];
      d_y = p[1];
    } else if (p.size() == 3) {
      d_x = p[0];
      d_y = p[1];
      d_z = p[2];
    }
  };

  /*!
   *  @brief Copy constructor
   *  @param p Another Point object
   */
  Point(const Point &p) : d_x(p.d_x), d_y(p.d_y), d_z(p.d_z) {};

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt = 0, int lvl = 0) const {

    std::string tabS = "";
    for (int i = 0; i < nt; i++)
      tabS += "\t";

    std::ostringstream oss;
    oss << tabS << "(" << d_x << ", " << d_y << ", " << d_z << ")";

    return oss.str();
  }

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }

  /*!
   * @brief Computes the Euclidean length of the vector
   * @return Length Euclidean length of the vector
   */
  double length() const { return std::sqrt(d_x * d_x + d_y * d_y + d_z * d_z); }

  /*!
   * @brief Computes the Euclidean length of the vector
   * @return Length Euclidean length of the vector
   */
  double lengthSq() const { return d_x * d_x + d_y * d_y + d_z *
  d_z; }

  /*!
   * @brief Computes the dot product of this vector with another point
   * @param b Another vector
   * @return Value a dot product
   */
  double dot(const Point &b) const { return d_x * b.d_x + d_y * b.d_y + d_z * b
  .d_z; }

  /*!
   * @brief Computes the distance between a given point from this point
   * @param b Another point
   * @return Value Distance between the two points
   */
  double dist(const Point &b) const {
    return std::sqrt((d_x - b.d_x) * (d_x - b.d_x) +
                     (d_y - b.d_y) * (d_y - b.d_y) +
                     (d_z - b.d_z) * (d_z - b.d_z));
  }

  /*!
   * @brief Computes the cross product between this vector and given vector
   * @param b Another vector
   * @return Vector Cross product
   */
  Point cross(const Point &b) const {
    return {-d_z * b.d_y + d_y * b.d_z,
            d_z * b.d_x - d_x * b.d_z,
            -d_y * b.d_x + d_x * b.d_y};
  }

  /*!
   * @brief Computes projection of vector on this vector
   * @param b Another vector
   * @param is_unit Specify if this is a unit vector
   * @return Vector Projection vector
   */
  Point project(const Point &b, bool is_unit = false) const {
    auto l_sq = (is_unit ? 1. : this->length() * this->length());
    auto dot = this->dot(b);
    return {dot * d_x / l_sq, dot * d_y / l_sq, dot * d_z / l_sq};
  }

  /*!
   * @brief Computes projection of vector on plane with normal as this vector
   * @param b Another vector
   * @param is_unit Specify if this is a unit vector
   * @return Vector Projection vector
   */
  Point projectNormal(const Point &b, bool is_unit = false) const {
    auto l_sq = (is_unit ? 1. : this->length() * this->length());
    auto dot = this->dot(b);
    return b - Point(dot * d_x / l_sq, dot * d_y / l_sq, dot * d_z / l_sq);
  }

  /**
   * @name Group operators
   */
  /**@{*/

  /*!
   * @brief Addition operator
   * @param lhs lhs (to be modified)
   * @param rhs rhs (to be added to lhs)
   * @return point lhs after adding rhs to it
   */
  friend Point operator+(Point lhs, const Point &rhs) {
    lhs += rhs;
    return lhs;
  }

  /*!
   * @brief Subtraction operator
   * @param lhs lhs (to be modified)
   * @param rhs rhs (to be subtracted from lhs)
   * @return point lhs after subtracting rhs from it
   */
  friend Point operator-(Point lhs, const Point &rhs) {
    lhs -= rhs;
    return lhs;
  }

  /*!
   * @brief Multiplication (dot) operator
   * @param lhs lhs
   * @param rhs rhs
   * @return product Dot product of two vectors
   */
  friend double operator*(Point lhs, const Point rhs) {
    return lhs.d_x * rhs.d_x + lhs.d_y * rhs.d_y + lhs.d_z * rhs.d_z;
  }

  /*!
   * @brief Scalar product operator
   * @param lhs lhs (to be modified)
   * @param rhs rhs scalar (to be multiplied to lhs)
   * @return point lhs after multiplying rhs to it
   */
  friend Point operator*(Point lhs, const double rhs) {
    lhs *= rhs;
    return lhs;
  }

  /*!
   * @brief Addition by scalar operator
   * @param lhs lhs (to be modified)
   * @param rhs rhs (to be added to lhs)
   * @return point lhs after adding rhs to it
   */
  friend Point operator+(Point lhs, const double rhs) {
    return {lhs.d_x + rhs, lhs.d_y + rhs, lhs.d_z + rhs};
  }

  /*!
   * @brief Addition by scalar operator
   * @param lhs lhs scalar
   * @param rhs rhs (to be added to lhs)
   * @return point After adding lhs to rhs
   */
  friend Point operator+(const double lhs, Point rhs) {
    return {lhs + rhs.d_x, lhs + rhs.d_y, lhs + rhs.d_z};
  }

  /*!
   * @brief Subtraction by scalar operator
   * @param lhs lhs (to be modified)
   * @param rhs rhs (to be subtracted from lhs)
   * @return point lhs after subtracting rhs from it
   */
  friend Point operator-(Point lhs, const double rhs) {
    return {lhs.d_x - rhs, lhs.d_y - rhs, lhs.d_z - rhs};
  }

  /*!
   * @brief Subtraction by scalar operator
   * @param lhs lhs scalar
   * @param rhs rhs (to be subtracted from lhs)
   * @return point After subtracting rhs from lhs
   */
  friend Point operator-(const double lhs, Point rhs) {
    return {lhs - rhs.d_x, lhs - rhs.d_y, lhs - rhs.d_z};
  }

  /*!
   * @brief Multiplication by scalar operator
   * @param lhs lhs scalar
   * @param rhs rhs vector
   * @return point Vector after multiplying lhs to rhs pointwise
   */
  friend Point operator*(const double lhs, Point rhs) {
    rhs *= lhs;
    return rhs;
  }

  /*!
   * @brief Division by scalar operator
   * @param lhs lhs scalar
   * @param rhs rhs vector
   * @return point Vector after dividing lhs to rhs pointwise
   */
  friend Point operator/(Point lhs, const double rhs) {
    lhs /= rhs;
    return lhs;
  }

  /*!
   * @brief Addition by scalar operator
   * @param b Scalar
   * @return point Vector after adding b to this point
   */
  Point &operator+=(const double b) {

    d_x += b;
    d_y += b;
    d_z += b;
    return *this;
  }

  /*!
   * @brief Subtraction by scalar operator
   * @param b Scalar
   * @return point Vector after subtracting b from this point
   */
  Point &operator-=(const double b) {

    d_x -= b;
    d_y -= b;
    d_z -= b;
    return *this;
  }

  /*!
   * @brief Multiplication by scalar operator
   * @param b Scalar
   * @return point Vector after multiplying b to this point
   */
  Point &operator*=(const double b) {

    d_x *= b;
    d_y *= b;
    d_z *= b;
    return *this;
  }

  /*!
   * @brief Addition operator
   * @param b Vector to be added
   * @return point Vector after adding b to this point
   */
  Point &operator+=(const Point &b) {

    d_x += b.d_x;
    d_y += b.d_y;
    d_z += b.d_z;
    return *this;
  }

  /*!
   * @brief Subtraction operator
   * @param b Vector to subtract
   * @return point Vector after subtracting b from this point
   */
  Point &operator-=(const Point &b) {

    d_x -= b.d_x;
    d_y -= b.d_y;
    d_z -= b.d_z;
    return *this;
  }

  /*!
   * @brief Multiplication (pointwise/elementwise) operator
   * @param b Vector
   * @return point Vector after elementwise multiplication of b with this point
   */
  Point &operator*=(const Point &b) {

    d_x *= b.d_x;
    d_y *= b.d_y;
    d_z *= b.d_z;
    return *this;
  }

  /*!
   * @brief Division by scalar operator
   * @param b Scalar
   * @return point Vector after dividing b elementwise from this point
   */
  Point &operator/=(const double b) {

    d_x /= b;
    d_y /= b;
    d_z /= b;
    return *this;
  }

  /*!
   * @brief Access operator
   * @param i Index of element in vector to access
   * @return value Value of ith element in vector
   */
  double &operator[](size_t i) {

    if (i == 0)
      return d_x;
    else if (i == 1)
      return d_y;
    else
      return d_z;
  }

  /*! @copydoc operator[](size_t i) */
  const double &operator[](size_t i) const {

    if (i == 0)
      return d_x;
    else if (i == 1)
      return d_y;
    else
      return d_z;
  }

  /** @}*/
};

} // namespace util

#endif
