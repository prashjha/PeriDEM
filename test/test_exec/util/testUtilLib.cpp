/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "testUtilLib.h"
#include "geom/geomIncludes.h"
#include "util/vecMethods.h"
#include "util/transformationFunctions.h"
#include "util/function.h"
#include <cmath>
#include <fstream>
#include <format>
#include <string>

namespace {

void errExit(std::string msg){
  std::cerr << msg;
  exit(EXIT_FAILURE);
}
}

void test::testUtilMethods() {

  const double tol = 1.e-10;

  //
  {
    std::pair<util::Point, util::Point> box = {util::Point(),
                                               util::Point(1., 1., 1.)};
    auto corner_pts = geom::getCornerPoints(3, box);
    auto edges = geom::getEdges(3, box);
    auto xc = geom::getCenter(3, box);

    for (size_t i=0; i<2; i++)
      for (size_t j=0; j<2; j++)
        for (size_t k=0; k<2; k++) {

          auto p = util::Point(double(i), double(j), double(k));
          bool found_p = false;
          for (auto q : corner_pts) {
            if (q.dist(p) < tol)
              found_p = true;
          }
          if (!found_p)
            errExit(std::format("Error: Can not find corner point {}\n", p.printStr()));
        }

    if (xc.dist(util::Point(0.5, 0.5, 0.5)) > tol)
      errExit("Error: getCenter()\n");
  }

  //
  {
    if (std::abs(geom::triangleArea(util::Point(0., 0., 0.), util::Point(2., 0., 0.), util::Point(1., 1., 0.)) - 1.) > tol)
      errExit("Error: triangleArea()\n");
  }

  //
  {
    std::vector<double> x = {1., 0., 0.};
    std::vector<double> y_check = {1./std::sqrt(2.), -1./std::sqrt(2.), 0.};
    auto y = util::rotateCW2D(x, M_PI * 0.25);
    if (util::methods::l2Dist(y_check, y) > tol)
      errExit("Error: rotateCW2D()\n");

    if (util::Point(y_check).dist(util::rotateCW2D(util::Point(x), M_PI * 0.25)) > tol)
      errExit("Error: rotateCW2D()\n");

    y_check = {1./std::sqrt(2.), 1./std::sqrt(2.), 0.};
    y = util::rotateACW2D(x, M_PI * 0.25);
    if (util::methods::l2Dist(y_check, y) > tol)
      errExit("Error: rotateACW2D()\n");
  }

  //
  {
    auto x = util::Point(1., 0., 0.);
    auto a = util::Point(0., 0., 1.);
    auto y_check = util::Point(0., 1., 0.);
    auto y = util::rotate(x, M_PI * 0.5, a);
    if (y_check.dist(y) > tol)
      errExit(std::format("Error: rotate(). y_check = {}, y = {}\n", y_check.printStr(), y.printStr()));

    x = util::Point(1., 1., 1.);
    y_check = util::Point(-1., 1., 1.);
    y = util::rotate(x, M_PI * 0.5, a);
    if (y_check.dist(y) > tol)
      errExit(std::format("Error: rotate(). y_check = {}, y = {}\n", y_check.printStr(), y.printStr()));
  }

  //
  {
    auto x1 = util::Point(1., 1., 0.);
    auto x2 = util::Point(1., 0., 0.);
    if (std::abs(M_PI*0.25 - util::angle(x1, x2)) > tol)
      errExit("Error: angle()\n");

    x2 = util::Point(0., 0., 1.);
    if (std::abs(M_PI*0.5 - util::angle(x1, x2)) > tol)
      errExit("Error: angle()\n");

    x2 = util::Point(0., 1., 1.);
    if (std::abs(M_PI/3. - util::angle(x1, x2)) > tol)
      errExit("Error: angle()\n");
  }
}

void test::testContactStiffness() {

  // Bulk moduli and horizons that actually appear in the examples and tests.
  const std::vector<double> Ks = {2.16e7, 1.0e4,     1.0e5,
                                  159.2e9, 216000.0, 2.0e9, 1.23e9};
  const std::vector<double> hs = {6.0e-4, 2.0e-4, 4.0e-4,
                                  3.0e-3, 3.2e-4, 1.25e-3, 3.75e-3};

  size_t n_checked = 0;
  for (auto K1 : Ks) {
    for (auto K2 : Ks) {
      for (auto h : hs) {
        // The form the example and test drivers used, at both exponents.
        for (int p : {4, 5}) {
          const double expected =
              18.0 * util::harmonicMean(K1, K2) / (M_PI * std::pow(h, p));
          const double got = util::normalContactStiffness(K1, K2, h, p);
          if (got != expected)
            errExit(std::format(
                "normalContactStiffness({}, {}, {}, {}) = {:.20g}, expected "
                "{:.20g}: the replaced expression gave a different value\n",
                K1, K2, h, p, got, expected));
          ++n_checked;
        }
      }
    }
    for (auto h : hs) {
      // The form BaseParticle uses for internal contact. The grouping is
      // (18 / (pi h^5)) * K and not 18 K / (pi h^5).
      const double expected = (18. / (M_PI * std::pow(h, 5))) * K1;
      const double got = util::selfContactStiffness(K1, h);
      if (got != expected)
        errExit(std::format(
            "selfContactStiffness({}, {}) = {:.20g}, expected {:.20g}: the "
            "order of operations differs from the one in BaseParticle\n",
            K1, h, got, expected));
      ++n_checked;
    }
  }

  // The two forms are the same quantity because harmonicMean(K, K) == K.
  for (auto K : Ks)
    if (util::harmonicMean(K, K) != K)
      errExit(std::format("harmonicMean({0}, {0}) != {0}\n", K));

  // A horizon of zero divides by zero, so it is rejected instead.
  bool threw = false;
  try {
    util::normalContactStiffness(1., 1., 0.);
  } catch (const std::exception &) {
    threw = true;
  }
  if (!threw)
    errExit("normalContactStiffness accepted a zero horizon\n");

  std::cout << std::format(
      "testContactStiffness: {} values equal to the replaced expressions\n",
      n_checked);
}
