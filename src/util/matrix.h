/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTIL_MATRIX_H
#define UTIL_MATRIX_H

#include "point.h"

namespace util {

/*! @brief A structure to represent 3d matrices */
struct Matrix3 {

  /**
   * @name Data members
   */
  /**@{*/

  /*! @brief data */
  float d_data[3][3]{};

  /** @}*/

  /*!
   * @brief Constructor
   */
  Matrix3() {

    d_data[0][0] = 0.;
    d_data[0][1] = 0.;
    d_data[0][2] = 0.;

    d_data[1][0] = 0.;
    d_data[1][1] = 0.;
    d_data[1][2] = 0.;

    d_data[2][0] = 0.;
    d_data[2][1] = 0.;
    d_data[2][2] = 0.;
  };

  /*!
   * @brief Constructor
   *
   * @param diagonal Diagonal vector
   */
  Matrix3(const util::Point & diagonal) {

    d_data[0][0] = diagonal.d_x;
    d_data[0][1] = 0.;
    d_data[0][2] = 0.;

    d_data[1][0] = 0.;
    d_data[1][1] = diagonal.d_y;
    d_data[1][2] = 0.;

    d_data[2][0] = 0.;
    d_data[2][1] = 0.;
    d_data[2][2] = diagonal.d_z;
  };

  /*!
   * @brief Constructor
   *
   * @param a1 first row
   * @param a2 second row
   * @param a3 third row
   */
  Matrix3(const util::Point & a1, const util::Point & a2, const util::Point & a3) {

    d_data[0][0] = a1.d_x;
    d_data[0][1] = a1.d_y;
    d_data[0][2] = a1.d_z,
    d_data[1][0] = a2.d_x;
    d_data[1][1] = a2.d_y;
    d_data[1][2] = a2.d_z;
    d_data[2][0] = a3.d_x;
    d_data[2][1] = a3.d_y;
    d_data[2][2] = a3.d_z;
  };

  /*!
   * @brief Constructor
   *
   * @param m Matrix in vector template
   */
  Matrix3(const std::vector<std::vector<double>> &m) {
    for (size_t i=0; i<3; i++)
      for (size_t j=0; j<3; j++)
        d_data[i][j] = m[i][j];
  }

  /*!
   * @brief Constructor
   *
   * @param m Matrix in vector template
   */
  Matrix3(const Matrix3 &m) {
    for (size_t i=0; i<3; i++)
      for (size_t j=0; j<3; j++)
        d_data[i][j] = m(i,j);
  }

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string Formatted string
   */
  std::string printStr(int nt = 0, int lvl = 0) const {

    std::string tabS = "";
    for (int i = 0; i < nt; i++)
      tabS += "\t";

    std::ostringstream oss;
    for (size_t i=0; i<3; i++)
      oss << tabS << "[" << (*this)(i, 0) << ", " << (*this)(i, 1) << ", "
          << (*this)(i, 2) << "]" << std::endl;
    oss << std::endl;

    return oss.str();
  }

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }

  /*!
   * @brief Returns row of matrix
   *
   * @param i Row id
   * @return Row Row
   */
  Point operator()(size_t i) {
    return Point(d_data[i]);
  }

  /*! @copydoc operator()(size_t i) */
  Point operator()(size_t i) const {
    return Point(d_data[i]);
  }

  /*!
   * @brief Returns element of matrix
   *
   * @param i Row id
   * @param j Column id
   * @return Element Element of matrix
   */
  float &operator()(size_t i, size_t j) { return d_data[i][j]; }

  /*! @copydoc operator()(size_t i, size_t j) */
  const float &operator()(size_t i, size_t j) const { return d_data[i][j]; }

  /*!
 * @brief Computes the dot product between matrix and vector
 *
 * @param v vector
 * @return vector Dot product
 */
  util::Point dot(const util::Point &v) {

    return {(*this)(0) * v, (*this)(1) * v, (*this)(2) * v};
  }

  /*! @copydoc dot(const util::Point &v) */
  std::vector<double> dot(const std::vector<double> &v) const {

    auto r = std::vector<double>(3,0.);
    for (size_t i=0; i<3; i++)
      for (size_t j=0; j<3; j++)
        r[i] += (*this)(i,j) * v[j];

    return r;
  }

  /*!
 * @brief Computes the tranpose of matrix
 * @return Matrix Transpose of m
 */
  Matrix3 transpose() const {

    Matrix3 m = Matrix3(*this);

    m(0,1) = (*this)(1,0);
    m(0,2) = (*this)(2,0);

    m(1,0) = (*this)(0,1);
    m(1,2) = (*this)(2,1);

    m(2,0) = (*this)(0,2);
    m(2,1) = (*this)(1,2);

    return m;
  }

  /*!
 * @brief Computes the determinant of matrix
 *
 * @return det Determinant
 */
  double det() const {
    return (*this)(0,0) * ((*this)(1,1) * (*this)(2,2) - (*this)(2,1) * (*this)(1,2)) -
           (*this)(0,1) * ((*this)(1,0) * (*this)(2,2) - (*this)(2,0) * (*this)(1,2)) +
           (*this)(0,2) * ((*this)(1,0) * (*this)(2,1) - (*this)(2,0) * (*this)(1,1));
  }

  /*!
 * @brief Computes the determinant of matrix
 *
 * @return inv Inverse of m
 */
  Matrix3 inv() const {

    Matrix3 m = Matrix3();

    auto det_inv = 1. / this->det();

    m(0,0) = det_inv *
              ((*this)(1,1) * (*this)(2,2) - (*this)(2,1) * (*this)(1,2));
    m(0,1) = -det_inv *
              ((*this)(0,1) * (*this)(2,2) - (*this)(2,1) * (*this)(0,2));
    m(0,2) = det_inv *
              ((*this)(0,1) * (*this)(1,2) - (*this)(1,1) * (*this)(0,2));

    m(1,0) = -det_inv *
              ((*this)(1,0) * (*this)(2,2) - (*this)(2,0) * (*this)(1,2));
    m(1,1) = det_inv *
              ((*this)(0,0) * (*this)(2,2) - (*this)(2,0) * (*this)(0,2));
    m(1,2) = -det_inv *
              ((*this)(0,0) * (*this)(1,2) - (*this)(1,0) * (*this)(0,2));

    m(2,0) = det_inv *
              ((*this)(1,0) * (*this)(2,1) - (*this)(2,0) * (*this)(1,1));
    m(2,1) = -det_inv *
              ((*this)(0,0) * (*this)(2,1) - (*this)(2,0) * (*this)(0,1));
    m(2,2) = det_inv *
              ((*this)(0,0) * (*this)(1,1) - (*this)(1,0) * (*this)(0,1));

    return m;
  }
};

/*! @brief A structure to represent 3d matrices */
struct SymMatrix3 {

  /**
   * @name Data members
   *
   * 0 - xx component
   * 1 - yy component
   * 2 - zz component
   * 3 - yz component
   * 4 - xz component
   * 5 - xy component
   */
  /**@{*/

  /*! @brief data */
  float d_data[6]{};

  /** @}*/

  /*!
   * @brief Constructor
   */
  SymMatrix3() {

    d_data[0] = 0.;
    d_data[1] = 0.;
    d_data[2] = 0.;
    d_data[3] = 0.;
    d_data[4] = 0.;
    d_data[5] = 0.;
  };

  /*!
   * @brief Constructor
   *
   * @param diagonal Diagonal vector
   */
  SymMatrix3(const util::Point & diagonal) {

    d_data[0] = diagonal.d_x;
    d_data[1] = diagonal.d_y;
    d_data[2] = diagonal.d_z;

    d_data[3] = 0.;
    d_data[4] = 0.;
    d_data[5] = 0.;
  };

  /*!
   * @brief Constructor
   *
   * @param m Matrix in vector template
   */
  SymMatrix3(const std::vector<double> &m) {

    d_data[0] = m[0];
    d_data[1] = m[1];
    d_data[2] = m[2];
    d_data[3] = m[3];
    d_data[4] = m[4];
    d_data[5] = m[5];
  }

  /*!
   * @brief Constructor
   *
   * @param m Matrix
   */
  SymMatrix3(const std::vector<std::vector<double>> &m) {

    d_data[0] = m[0][0];
    d_data[1] = m[1][1];
    d_data[2] = m[2][2];
    d_data[3] = 0.5 * (m[1][2] + m[2][1]);
    d_data[4] = 0.5 * (m[0][2] + m[2][0]);
    d_data[5] = 0.5 * (m[0][1] + m[1][0]);
  }

  /*!
   * @brief Constructor
   *
   * @param m Matrix
   */
  SymMatrix3(const Matrix3 &m) {

    d_data[0] = m(0,0);
    d_data[1] = m(1,1);
    d_data[2] = m(2,2);
    d_data[3] = 0.5 * (m(1,2) + m(2,1));
    d_data[4] = 0.5 * (m(0,2) + m(2,0));
    d_data[5] = 0.5 * (m(0,1) + m(1,0));
  }

  /*!
   * @brief Constructor
   *
   * @param m Matrix
   */
  SymMatrix3(const SymMatrix3 &m) {

    for (size_t i=0; i<6; i++)
      d_data[i] = m.d_data[i];
  }

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
    for (size_t i=0; i<3; i++)
      oss << tabS << "[" << (*this)(i, 0) << ", " << (*this)(i, 1) << ", "
          << (*this)(i, 2) << "]" << std::endl;
    oss << std::endl;

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
   * @brief Returns row of matrix
   *
   * @param i Row id
   * @return Row Row
   */
  Point operator()(size_t i) {
      return {(*this)(i, 0), (*this)(i, 1), (*this)(i, 2)};
  }

  /*! @copydoc operator()(size_t i) */
  Point operator()(size_t i) const {
    return {(*this)(i, 0), (*this)(i, 1), (*this)(i, 2)};
  }

  /*!
   * @brief Returns element of matrix
   *
   * @param i Row id
   * @param j Column id
   * @return Element Element of matrix
   */
  float &operator()(size_t i, size_t j) {
    return d_data[i == j ? i : 6 - i - j];
  }

  /*! @copydoc operator()(size_t i, size_t j) */
  const float &operator()(size_t i, size_t j) const {
    return d_data[i == j ? i : 6 - i - j];
  }

  /*!
   * @brief Returns row of matrix
   *
   * @param i Row id
   * @return Row Row
   */
  float &get(size_t i) {
    return d_data[i];
  }

  /*! @copydoc get(size_t i) */
  const float &get(size_t i) const {
    return d_data[i];
  }

  /*!
 * @brief Copy
 *
 * @param m Symmetric matrix in vector form
 */
  void copy(double m[6]) const {

    for (size_t i=0; i<6; i++)
      m[i] = d_data[i];
  }

  /*!
   * @brief Computes the dot product of this matrix with another vector
   * @param v A vector
   * @return Vector Resulting vector
   */
  util::Point dot(const util::Point &v) {

    return {(*this)(0) * v, (*this)(1) * v, (*this)(2) * v};
  }

  /*! @copydoc dot(const util::Point &v) */
  std::vector<double> dot(const std::vector<double> &v) const {

    auto r = std::vector<double>(3,0.);
    for (size_t i=0; i<3; i++)
      for (size_t j=0; j<3; j++)
        r[i] += (*this)(i,j) * v[j];

    return r;
  }

  /*!
   * @brief Computes the tranpose of matrix
   * @return Matrix Transpose of m
   */
  SymMatrix3 transpose() const {

    return (*this);
  }

  /*!
   * @brief Computes the determinant of matrix
   *
   * @return det Determinant
   */
  double det() const {
    return (*this)(0,0) * ((*this)(1,1) * (*this)(2,2) - (*this)(2,1) * (*this)(1,2)) -
           (*this)(0,1) * ((*this)(1,0) * (*this)(2,2) - (*this)(2,0) * (*this)(1,2)) +
           (*this)(0,2) * ((*this)(1,0) * (*this)(2,1) - (*this)(2,0) * (*this)(1,1));
  }

  /*!
   * @brief Computes the determinant of matrix
   *
   * @return inv Inverse of m
   */
  SymMatrix3 inv() const {

    SymMatrix3 m = SymMatrix3();

    auto det_inv = 1. / this->det();

    m(0,0) = det_inv *
             ((*this)(1,1) * (*this)(2,2) - (*this)(2,1) * (*this)(1,2));
    m(0,1) = -det_inv *
             ((*this)(0,1) * (*this)(2,2) - (*this)(2,1) * (*this)(0,2));
    m(0,2) = det_inv *
             ((*this)(0,1) * (*this)(1,2) - (*this)(1,1) * (*this)(0,2));

    m(1,0) = -det_inv *
             ((*this)(1,0) * (*this)(2,2) - (*this)(2,0) * (*this)(1,2));
    m(1,1) = det_inv *
             ((*this)(0,0) * (*this)(2,2) - (*this)(2,0) * (*this)(0,2));
    m(1,2) = -det_inv *
             ((*this)(0,0) * (*this)(1,2) - (*this)(1,0) * (*this)(0,2));

    m(2,0) = det_inv *
             ((*this)(1,0) * (*this)(2,1) - (*this)(2,0) * (*this)(1,1));
    m(2,1) = -det_inv *
             ((*this)(0,0) * (*this)(2,1) - (*this)(2,0) * (*this)(0,1));
    m(2,2) = det_inv *
             ((*this)(0,0) * (*this)(1,1) - (*this)(1,0) * (*this)(0,1));

    return m;
  }
};

/*!
 * @brief Checks matrix
 *
 * @param m Matrix
 * @return true If matrix is okay
 */
bool checkMatrix(const std::vector<std::vector<double>> &m);

/*!
 * @brief Computes the dot product between matrix and vector
 *
 * @param m Matrix
 * @param v vector
 * @return vector Dot product
 */
std::vector<double> dot(const std::vector<std::vector<double>> &m, const
std::vector<double> &v);

/*!
 * @brief Computes the tranpose of matrix
 *
 * @param m Matrix
 * @return Matrix Transpose of m
 */
std::vector<std::vector<double>> transpose(const
std::vector<std::vector<double>> &m);

/*!
 * @brief Computes the determinant of matrix
 *
 * @param m Matrix
 * @return det Determinant
 */
double det(const std::vector<std::vector<double>> &m);

/*!
 * @brief Computes the determinant of matrix
 *
 * @param m Matrix
 * @return inv Inverse of m
 */
std::vector<std::vector<double>>
inv(const std::vector<std::vector<double>> &m);

} // namespace util

#endif // UTIL_MATRIX_H
