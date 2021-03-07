/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "matrix.h"
#include "io.h"

bool util::checkMatrix(const std::vector<std::vector<double>> &m) {

  size_t row_size = m.size();
  if (row_size > 3) {
    std::cerr << "Error: Determinant of matrix above size 3 is not "
                 "implemented\n";
    exit(1);
  }

  size_t col_size = m[0].size();
  bool check = row_size > 1 and col_size != m[1].size();
  if (!check)
    check = row_size > 2 and col_size != m[2].size();

  if (check) {
    std::ostringstream  oss;
    oss << "Error: Row size and col size do not match.\n Matrix = [";
    for (auto a : m)
      oss << util::io::printStr(a) << "\n";
    oss << "].\n";
    std::cerr << oss.str();
    exit(1);
  }
}

std::vector<double> util::dot(const std::vector<std::vector<double>> &m, const
std::vector<double> &v) {

  checkMatrix(m);
  size_t row_size = m.size();
  std::vector<double> r(row_size, 0.);

  for (size_t i=0; i<row_size; i++)
    for (size_t j=0; j<row_size; j++)
      r[i] += m[i][j] * v[j];

  return r;
}

std::vector<std::vector<double>> util::transpose(const
std::vector<std::vector<double>> &m) {

  checkMatrix(m);

  size_t row_size = m.size();

  std::vector<std::vector<double>> n = m;

  if (row_size == 1)
    return n;
  else if (row_size == 2) {

    n[0][1] = m[1][0];
    n[1][0] = m[0][1];
    return n;
  }
  else if (row_size == 3) {

    n[0][1] = m[1][0];
    n[0][2] = m[2][0];

    n[1][0] = m[0][1];
    n[1][2] = m[2][1];

    n[2][0] = m[0][2];
    n[2][1] = m[1][2];
    return n;
  }
}

double util::det(const std::vector<std::vector<double>> &m) {

  //  std::cout << "Matrix = " << util::io::printStr(m, 0) << "\n";

  checkMatrix(m);



  size_t row_size = m.size();
  if (row_size == 1)
    return m[0][0];
  else if (row_size == 2)
    return m[0][0] * m[1][1] - m[0][1] * m[1][0];
  else if (row_size == 3)
    return m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
           m[0][1] * (m[1][0] * m[2][2] - m[2][0] * m[1][2]) +
           m[0][2] * (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
}

std::vector<std::vector<double>>
util::inv(const std::vector<std::vector<double>> &m) {

  checkMatrix(m);

  size_t row_size = m.size();

  std::vector<std::vector<double>> n(row_size);
  for (size_t i =0; i<row_size; i++)
    n[i] = std::vector<double>(row_size, 0.);

  if (row_size == 1) {
    n[0][0] = 1. / m[0][0];

    return n;
  } else if (row_size == 2) {

    auto det_inv = 1. / det(m);

    n[0][0] = det_inv * m[1][1];
    n[1][1] = det_inv * m[0][0];

    n[0][1] = -det_inv * m[0][1];
    n[1][0] = -det_inv * m[1][0];

    return n;
  } else if (row_size == 3) {

    auto det_inv = 1. / det(m);

    n[0][0] = det_inv *
              (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
    n[0][1] = -det_inv *
              (m[0][1] * m[2][2] - m[2][1] * m[0][2]);
    n[0][2] = det_inv *
              (m[0][1] * m[1][2] - m[1][1] * m[0][2]);

    n[1][0] = -det_inv *
              (m[1][0] * m[2][2] - m[2][0] * m[1][2]);
    n[1][1] = det_inv *
              (m[0][0] * m[2][2] - m[2][0] * m[0][2]);
    n[1][2] = -det_inv *
              (m[0][0] * m[1][2] - m[1][0] * m[0][2]);

    n[2][0] = det_inv *
              (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
    n[2][1] = -det_inv *
              (m[0][0] * m[2][1] - m[2][0] * m[0][1]);
    n[2][2] = det_inv *
              (m[0][0] * m[1][1] - m[1][0] * m[0][1]);

    return n;
  }
}
