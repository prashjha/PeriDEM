/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "testParticleLib.h"
#include "particle/particle.h"
#include "util/point.h"
#include "util/matrix.h"
#include <bitset>
#include <fstream>
#include <iostream>

void test::testTransform() {

  bool test_result = true;

  // test translation
  double scale = 1.0;
  double theta = 0.;
  auto xt = util::Point(1., 1., 0.);
  auto t1 = particle::ParticleTransform(xt, util::Point(0., 0., 1.), theta, scale);

  auto xold = util::Point(0., 0., 0.);
  auto xnew = t1.apply(xold);
  auto xnew_check = util::Point( xold.d_x + 1., xold.d_y + 1., xold.d_z);

  std::cout << "xold = (" << xold.d_x << ", " << xold.d_y << ", " << xold.d_z
            << "), xnew = (" << xnew.d_x << ", " << xnew.d_y << ", " << xnew.d_z
            << "), distance = " << xnew_check.dist(xnew) << "\n";
  if (xnew_check.dist(xnew) > 1.0E-8) {
    std::cout << "Error\n";
    test_result = false;
  }

  // test rotation
  scale = 1.0;
  theta = M_PI / 6;
  xt = util::Point(0., 0., 0.);
  auto t2 =
      particle::ParticleTransform(xt, util::Point(0., 0., 1.), theta, scale);
  xold = util::Point(0.5, 0.2, 0.);
  xnew = t2.apply(xold);
  xnew_check = util::Point(
      xold.d_x * std::cos(theta) - xold.d_y * std::sin(theta),
      xold.d_x * std::sin(theta) + xold.d_y * std::cos(theta), 0.);

  std::cout << "xold = (" << xold.d_x << ", " << xold.d_y << ", " << xold.d_z
            << "), xnew = (" << xnew.d_x << ", " << xnew.d_y << ", " << xnew.d_z
            << "), distance = " << xnew_check.dist(xnew) << "\n";
  if (xnew_check.dist(xnew) > 1.0E-8) {
    std::cout << "Error\n";
    test_result = false;
  }

  // test scale and rotation
  scale = 0.5;
  theta = M_PI / 3;
  xt = util::Point(0., 0., 0.);
  auto t3 =
      particle::ParticleTransform(xt, util::Point(0., 0., 1.), theta,
          scale);
  xold = util::Point(0.2, 0.4, 0.);
  xnew = t3.apply(xold);
  xnew_check = util::Point(
      scale * xold.d_x * std::cos(theta) - scale * xold.d_y * std::sin(theta),
      scale * xold.d_x * std::sin(theta) + scale * xold.d_y * std::cos(theta), 0.);

  std::cout << "xold = (" << xold.d_x << ", " << xold.d_y << ", " << xold.d_z
            << "), xnew = (" << xnew.d_x << ", " << xnew.d_y << ", " << xnew.d_z
            << "), distance = " << xnew_check.dist(xnew) << "\n";
  if (xnew_check.dist(xnew) > 1.0E-8) {
    std::cout << "Error\n";
    test_result = false;
  }
}
