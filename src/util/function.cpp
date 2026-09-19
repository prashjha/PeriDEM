/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "function.h"
#include "util/io.h"
#include <cmath>                  // definition of sin, cosine etc
#include <iostream>               // cerr
#include <stdexcept>              // invalid_argument

bool util::isGreater(const double &a, const double &b) {
  return (a - b) > ((std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) *
      COMPARE_EPS);
}

bool util::isLess(const double &a, const double &b) {
  return (b - a) > ((std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) *
      COMPARE_EPS);
}

double util::hatFunction(const double &x, const double &x_min,
                                   const double &x_max) {

  if (util::isGreater(x, x_min - 1.0E-12) and
      util::isLess(x, x_max + 1.0E-12)) {

    double x_mid = 0.5 * (x_min + x_max);
    double l = x_mid - x_min;

    // check if this is essentially a point load (dirac)
    if (l < 1.0E-12)
      return 1.0;

    if (util::isLess(x, x_mid))
      return (x - x_min) / l;
    else
      return (x_max - x) / l;
  } else
    return 0.0;
}

double util::hatFunctionQuick(const double &x, const double &x_min,
                                        const double &x_max) {

  double x_mid = 0.5 * (x_min + x_max);
  double l = x_mid - x_min;

  // check if this is essentially a point load (dirac)
  if (l < 1.0E-12)
    return 1.0;

  if (util::isLess(x, x_mid))
    return (x - x_min) / l;
  else
    return (x_max - x) / l;
}

double util::linearStepFunc(const double &x, const double &x1,
                                      const double &x2) {

  //
  // a = floor(x/(x1+x2))
  // xl = a * (x1 + x2), xm = xl + x1, xr = xm + x2
  // fl = a * x1
  //
  // At xl, value of the function is = period number \times x1
  //
  // From xl to xm, the function grows linear with slope 1
  // so the value in between [xl, xm) will be
  // fl + (x - xl) = a*x1 + (x - a*(x1+x2)) = x - a*x2
  //
  // In [xm, xr) function is constant and the value is
  // fl + (xm - xl) = a*x1 + (xl + x1 - xl) = (a+1)*x1

  double period = std::floor(x / (x1 + x2));

  if (util::isLess(x, period * (x1 + x2) + x1))
    return x - period * x2;
  else
    return (period + 1.) * x1;
}

double util::gaussian(const double &r, const double &a,
                                const double &beta) {
  return a * std::exp(-std::pow(r, 2) / beta);
}

double util::gaussian2d(const util::Point &x, const size_t &dof,
                                  const std::vector<double> &params) {

  if (params.size() < 6) {
    throw std::runtime_error(
        util::io::Msg()
        << "Error: Not enough parameters to compute guassian 2-d "
        "function.\n");
  }

  return util::gaussian(
             x.dist(util::Point(params[0], params[1], 0.)), params[5],
             params[4]) *
         params[2 + dof];
}

double util::doubleGaussian2d(const util::Point &x,
                                        const size_t &dof,
                                        const std::vector<double> &params) {

  if (params.size() < 10) {
    throw std::runtime_error(
        util::io::Msg()
        << "Error: Not enough parameters to compute guassian 2-d "
        "function.\n");
  }

  return util::gaussian(
             x.dist(util::Point(params[0], params[1], 0.)), params[9],
             params[8]) *
             params[4 + dof] +
         util::gaussian(
             x.dist(util::Point(params[2], params[3], 0.)), params[9],
             params[8]) *
             params[6 + dof];
}

double util::equivalentMass(const double &m1, const double &m2) {
  return 2. * m1 * m2 / (m1 + m2);
}

double util::harmonicMean(const double &m1, const double &m2) {
  return 2. * m1 * m2 / (m1 + m2);
}

double util::normalContactStiffness(const double &K1, const double &K2,
                                    const double &horizon, int horizonPower) {
  if (!(horizon > 0.))
    throw std::invalid_argument(
        "normalContactStiffness: horizon must be positive");
  return 18. * util::harmonicMean(K1, K2) /
         (M_PI * std::pow(horizon, horizonPower));
}

double util::selfContactStiffness(const double &K, const double &horizon,
                                  int horizonPower) {
  if (!(horizon > 0.))
    throw std::invalid_argument(
        "selfContactStiffness: horizon must be positive");
  // Grouping preserved from BaseParticle for bit-for-bit equality.
  return (18. / (M_PI * std::pow(horizon, horizonPower))) * K;
}
