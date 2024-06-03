/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "transformation.h"
#include <cmath>              // definition of sin, cosine etc

std::vector<double>
util::rotateCW2D(const std::vector<double> &x,
                                 const double &theta) {

  return std::vector<double>{x[0] * std::cos(theta) + x[1] * std::sin(theta),
                             -x[0] * std::sin(theta) + x[1] * std::cos(theta),
                             0.0};
}

util::Point util::rotateCW2D(const util::Point &x,
                                              const double &theta) {

  return {x.d_x * std::cos(theta) + x.d_y * std::sin(theta),
          -x.d_x * std::sin(theta) + x.d_y * std::cos(theta), 0.0};
}

std::vector<double>
util::rotateACW2D(const std::vector<double> &x,
                                  const double &theta) {

  return rotateCW2D(x, -theta);
}

util::Point util::rotateACW2D(const util::Point &x,
                                               const double &theta) {

  return rotateCW2D(x, -theta);
}


std::vector<double>
util::rotate2D(const std::vector<double> &x,
                                 const double &theta) {

  return std::vector<double>{x[0] * std::cos(theta) - x[1] * std::sin(theta),
                             x[0] * std::sin(theta) + x[1] * std::cos(theta),
                             0.0};
}

util::Point util::rotate2D(const util::Point &x,
                                              const double &theta) {

  return {x.d_x * std::cos(theta) - x.d_y * std::sin(theta),
          x.d_x * std::sin(theta) + x.d_y * std::cos(theta), 0.0};
}

util::Point util::derRotate2D(const util::Point &x,
                                            const double &theta) {

  return {-x.d_x * std::sin(theta) - x.d_y * std::cos(theta),
          x.d_x * std::cos(theta) - x.d_y * std::sin(theta), 0.0};
}

util::Point util::rotate(const util::Point &p, const double &theta, const util::Point &axis) {

  auto ct = std::cos(theta);
  auto st = std::sin(theta);

  // dot
  double p_dot_n = p * axis;

  // cross
  util::Point n_cross_p = axis.cross(p);

  return (1. - ct) * p_dot_n * axis + ct * p + st * n_cross_p;
}

double util::angle(util::Point a, util::Point b) {

  if ((a - b).lengthSq() < 1.0E-12)
    return 0.;

  // since we do not know which side of plane given by normal
  // a x b / |a x b| is +ve, we compute the angle using cosine
  return std::acos(b * a / (b.length() * a.length()));
}

double util::angle(util::Point a, util::Point b, util::Point axis, bool is_axis) {

  if ((a - b).lengthSq() < 1.0E-12)
    return 0.;

  if (is_axis) {

    // normal to plane of rotation
    util::Point n = axis / axis.length();

    util::Point na = n.cross(a);

    double theta = std::atan(b * na / (a * b - (b * n) * (a * n)));
    if (theta < 0.)
      theta += M_PI;

    if (b * na < 0.)
      theta = M_PI + theta;

    return theta;
  } else {

    auto theta = angle(a, b);

    // TODO below only works in specific cases such as when vectors in xy
    //  plane and vector x gives the positive plane direction, i.e. whether
    //  (0, 0, 1) is +ve or (0, 0, -1) is +ve. Similar is true for yz, zx
    //  planes.

    // normal to a and b
    util::Point n_ab = a.cross(b);

    double orient = axis * n_ab;
    if (orient < 0.)
      return 2. * M_PI - theta;
    else
      return theta;
  }
}
