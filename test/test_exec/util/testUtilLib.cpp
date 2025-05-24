/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
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