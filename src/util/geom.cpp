////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "geom.h"
#include "function.h"
#include "io.h"
#include "transformation.h"
#include <iostream>

std::vector<util::Point> util::getCornerPoints(
    size_t dim, const std::pair<util::Point, util::Point> &box) {

  if (dim == 1)
    return {box.first, box.second};
  else if (dim == 2)
    return {box.first,
            util::Point(box.second.d_x, box.first.d_y, 0.),
            box.second,
            util::Point(box.first.d_x, box.second.d_y, 0.)};
  else if (dim == 3) {
    double a = box.second.d_x - box.first.d_x;
    double b = box.second.d_y - box.first.d_y;
    double c = box.second.d_z - box.first.d_z;
    return {box.first,
            box.first + util::Point(a, 0., 0.),
            box.first + util::Point(a, b, 0.),
            box.first + util::Point(0., b, 0.),
            box.first + util::Point(0., 0., c),
            box.first + util::Point(a, 0., c),
            box.first + util::Point(a, b, c),
            box.first + util::Point(0., b, c)};
  }
  else {
    std::cerr << "Error: Check dimension = " << dim << ".\n";
    exit(1);
  }
}

std::vector<std::pair<util::Point, util::Point>> util::getEdges(size_t dim, const
std::pair<util::Point, util::Point> &box) {

  std::vector<std::pair<util::Point, util::Point>> data;
  if (dim == 1) {
    data.emplace_back(box);
    return data;
  } else if (dim == 2) {

    // points returned by below function is in anti-clockwise order
    auto corner_pts = util::getCornerPoints(dim, box);

    data.emplace_back(corner_pts[0], corner_pts[1]);
    data.emplace_back(corner_pts[1], corner_pts[2]);
    data.emplace_back(corner_pts[2], corner_pts[3]);
    data.emplace_back(corner_pts[3], corner_pts[0]);
    return data;
  } else if (dim == 3) {

    // points returned by below function is in anti-clockwise order
    // first 4 points are on lower z-plane, and remaining are in upper z-plane
    auto corner_pts = util::getCornerPoints(dim, box);

    // edges in lower plane
    data.emplace_back(corner_pts[0], corner_pts[1]);
    data.emplace_back(corner_pts[1], corner_pts[2]);
    data.emplace_back(corner_pts[2], corner_pts[3]);
    data.emplace_back(corner_pts[3], corner_pts[0]);

    // edges in upper plane
    data.emplace_back(corner_pts[4], corner_pts[5]);
    data.emplace_back(corner_pts[5], corner_pts[6]);
    data.emplace_back(corner_pts[6], corner_pts[7]);
    data.emplace_back(corner_pts[7], corner_pts[4]);

    // edges parrallel to z-axis
    data.emplace_back(corner_pts[0], corner_pts[4]);
    data.emplace_back(corner_pts[1], corner_pts[5]);
    data.emplace_back(corner_pts[2], corner_pts[6]);
    data.emplace_back(corner_pts[3], corner_pts[7]);

    return data;
  }
}

util::Point util::getCenter(size_t dim,
                       const std::pair<util::Point, util::Point> &box) {

  if (dim == 1)
    return {0.5 * box.second.d_x + 0.5 * box.first.d_x, 0., 0.};
  else if (dim == 2)
    return {0.5 * box.second.d_x + 0.5 * box.first.d_x,
                        0.5 * box.second.d_y + 0.5 * box.first.d_y, 0.};
  else if (dim == 3)
    return {0.5 * box.second.d_x + 0.5 * box.first.d_x,
                        0.5 * box.second.d_y + 0.5 * box.first.d_y,
                        0.5 * box.second.d_z + 0.5 * box.first.d_z};
  else {
    std::cerr << "Error: Check dimension = " << dim << ".\n";
    exit(1);
  }
}

bool util::areBoxesNear(const std::pair<util::Point, util::Point>
    &b1,
                  const std::pair<util::Point, util::Point> &b2,
                  const double &tol,
                  size_t dim) {

  auto cp1 = getCornerPoints(dim, b1);
  auto cp2 = getCornerPoints(dim, b2);

  for (auto p : cp1) {

    // check 1: If any of the corner points of box 1 are inside box 2
    if (isPointInsideBox(p, dim, b2))
      return true;

    // check 2: If any pair of corner points of box 1 and box 2 are at
    // distance smaller than the tolerance
    for (auto pp : cp2) {
      auto dx = pp - p;
      if (util::isLess(dx.length(), tol))
        return true;
    }
  }

  // check 3: Check if distance between centers of box 1 and box 2 are below
  // sum of tolerance, radius of circle inscribed in box 1, and radius of
  // circle inscribed in box 2
  auto dxc = getCenter(dim, b2) - getCenter(dim, b1);
  auto dist = tol + inscribedRadiusInBox(dim, b1) + inscribedRadiusInBox(dim,
      b2);

  if (util::isLess(dxc.length(), dist))
    return true;

  // check 4: Check if distance between centers of box 1 and box 2 are below
  // sum of tolerance, radius of circle inscribed in box 1, and radius of
  // circle circumscribed in box 2
  dist = tol + inscribedRadiusInBox(dim, b1) + circumscribedRadiusInBox(dim,
                                                                         b2);
  if (util::isLess(dxc.length(), dist))
    return true;

  dist = tol + circumscribedRadiusInBox(dim, b1) + inscribedRadiusInBox(dim,
                                                                        b2);
  if (util::isLess(dxc.length(), dist))
    return true;
}

bool util::isPointInsideBox(util::Point x, size_t dim,
                      const std::pair<util::Point, util::Point> &box) {

  if (dim == 1)
    return !(util::isLess(x.d_x, box.first.d_x - 1.0E-12) or
             util::isGreater(x.d_x, box.second.d_x + 1.0E-12));
  else if (dim == 2)
    return !(util::isLess(x.d_x, box.first.d_x - 1.0E-12) or
             util::isLess(x.d_y, box.first.d_y - 1.0E-12) or
             util::isGreater(x.d_x, box.second.d_x + 1.0E-12) or
             util::isGreater(x.d_y, box.second.d_y + 1.0E-12));
  else if (dim == 3)
    return !(util::isLess(x.d_x, box.first.d_x - 1.0E-12) or
             util::isLess(x.d_y, box.first.d_y - 1.0E-12) or
             util::isLess(x.d_z, box.first.d_z - 1.0E-12) or
             util::isGreater(x.d_x, box.second.d_x + 1.0E-12) or
             util::isGreater(x.d_y, box.second.d_y + 1.0E-12) or
             util::isGreater(x.d_z, box.second.d_z + 1.0E-12));
}

double util::inscribedRadiusInBox(size_t dim,
                                            const std::pair<util::Point, util::Point> &box) {

  double r = 0.5 * std::abs(box.second.d_x - box.first.d_x);
  if (dim == 1)
    return r;
  else if (dim == 2) {
    if (util::isGreater(r, 0.5 * std::abs(box.second.d_y - box.first.d_y)))
      return 0.5 * std::abs(box.second.d_y - box.first.d_y);
    else
      return r;
  } else if (dim == 3) {
    if (util::isGreater(r, 0.5 * std::abs(box.second.d_y - box.first.d_y)))
      r = 0.5 * std::abs(box.second.d_y - box.first.d_y);

    if (util::isGreater(r, 0.5 * std::abs(box.second.d_z - box.first.d_z)))
      return 0.5 * std::abs(box.second.d_z - box.first.d_z);
    else
      return r;
  }
}

double util::circumscribedRadiusInBox(size_t dim,
                                            const std::pair<util::Point, util::Point> &box) {

  auto xc = getCenter(dim, box);
  auto cp = getCornerPoints(dim, box);

  auto dx = cp[0] - xc;
  auto r = dx.length();

  if (dim == 1)
    return r;
  else {
    for (auto p : cp) {
      dx = p - xc;
      if (util::isGreater(dx.length(), r))
        r = dx.length();
    }

    return r;
  }
}


bool util::isPointInsideRectangle(util::Point x, double x_min,
                                            double x_max, double y_min,
                                            double y_max) {

  return !(util::isLess(x.d_x, x_min - 1.0E-12) or
           util::isLess(x.d_y, y_min - 1.0E-12) or
           util::isGreater(x.d_x, x_max + 1.0E-12) or
           util::isGreater(x.d_y, y_max + 1.0E-12));
}

bool util::isPointInsideRectangle(util::Point x, util::Point x_lb,
                                            util::Point x_rt) {
  return !(util::isLess(x.d_x, x_lb.d_x - 1.0E-12) or
           util::isLess(x.d_y, x_lb.d_y - 1.0E-12) or
           util::isGreater(x.d_x, x_rt.d_x + 1.0E-12) or
           util::isGreater(x.d_y, x_rt.d_y + 1.0E-12));
}

bool util::isPointInsideAngledRectangle(util::Point x, double x1,
                                                  double x2, double y1,
                                                  double y2, double theta) {
  // we assume that the rectangle has passed the test

  //
  //                             (x2,y2)
  //                            o
  //
  //
  //
  //
  //
  //        o
  //      (x1,y1)

  // get divisors
  util::Point lam = util::rotateCW2D(
      util::Point(x2 - x1, y2 - y1, 0.0), theta);

  // double lam1 = (x2-x1) * std::cos(theta) + (y2-y1) * std::sin(theta);
  // double lam2 = -(x2-x1) * std::sin(theta) + (y2-y1) * std::cos(theta);

  // get mapped coordinate of x
  util::Point xmap = util::rotateCW2D(
      util::Point(x[0] - x1, x[1] - y1, 0.0), theta);

  // double xmap = (x[0]-x1) * std::cos(theta) + (x[1]-y1) * std::sin(theta);
  // double ymap = -(x[0]-x1) * std::sin(theta) + (x[1]-y1) * std::cos(theta);

  // check if mapped coordinate are out of range [0, lam1] and [0, lam2]
  return !(util::isLess(xmap[0], -1.0E-12) or
           util::isLess(xmap[1], -1.0E-12) or
           util::isGreater(xmap[0], lam[0] + 1.0E-12) or
           util::isGreater(xmap[1], lam[1] + 1.0E-12));
}

bool util::isPointInsideCuboid(util::Point x, util::Point x_lbb,
                                         util::Point x_rtf) {
  return !(util::isLess(x.d_x, x_lbb.d_x - 1.0E-12) or
           util::isLess(x.d_y, x_lbb.d_y - 1.0E-12) or
           util::isLess(x.d_z, x_lbb.d_z - 1.0E-12) or
           util::isGreater(x.d_x, x_rtf.d_x + 1.0E-12) or
           util::isGreater(x.d_y, x_rtf.d_y + 1.0E-12) or
           util::isGreater(x.d_z, x_rtf.d_z + 1.0E-12));
}

bool util::isPointInsideCylinder(const util::Point &p, const double &length, const double
&radius, const util::Point &axis) {

  double p_dot_a = p * axis;
  if (p_dot_a > length or p_dot_a < 0.)
    return false;
  else {

    auto p_parallel = p - p_dot_a * axis;

    return p_parallel.lengthSq() < radius * radius;
  }
}

bool util::isPointInsideCylinder(const util::Point &p, const double &radius,
                              const util::Point &x1, const util::Point &x2) {

  auto p_new = p - x1;
  auto a = x2 - x1;
  double p_dot_a = p_new * a;

  // note here we should 1 if a is not normalized
  if (p_dot_a > 1. or p_dot_a < 0.)
    return false;
  else {

    auto p_parallel = p_new - p_dot_a * a;

    return p_parallel.lengthSq() < radius * radius;
  }
}

bool util::isPointInsideEllipse(const util::Point &p, const util::Point &center, const
std::vector<double> &radius_vec, unsigned int dim) {

  double d = 0.;
  auto x = p - center;
  for (unsigned int i=0; i<dim; i++)
    d += x[i] * x[i] / (radius_vec[i] * radius_vec[i]);

  return d < 1.;
}

bool util::isPointInsideEllipse(const util::Point &p, const util::Point &center, const
std::vector<double> &radius_vec, unsigned int dim, double &d) {

  d = 0.;
  auto x = p - center;
  for (unsigned int i=0; i<dim; i++)
    d += x[i] * x[i] / (radius_vec[i] * radius_vec[i]);

  return d < 1.;
}

util::Point util::getPointOnLine(const util::Point &p1, const
util::Point &p2,
    const double &s) {
  return (1. - s) * p1 + s * p2;
}

bool util::doLinesIntersect(const std::pair<util::Point, util::Point> &line_1,
                      const std::pair<util::Point, util::Point> &line_2) {

  // change of variable so that first point of line_1 is at origin
  // After change of variables:
  // a is the second point of line_1
  // b is the first point of line_2
  // c is the difference of second and first point of line_2
  auto a = line_1.second - line_1.first;
  auto b = line_2.first - line_1.first;
  auto c = line_2.second - line_2.first;

  // check if the two lines are parallel
  if (util::angle(a / a.length(), c / c.length()) < 1.0E-8)
    return false;

  double a_dot_a = a.lengthSq();
  double a_dot_b = a * b;
  double a_dot_c = a * c;
  double b_dot_c = b * c;
  double c_dot_c = c.lengthSq();

  double r = (a_dot_a * b_dot_c - a_dot_b * a_dot_c) /
             (a_dot_c * a_dot_c - c_dot_c * a_dot_a);

  // if r is in (0,1) then this gives the intersection point
  // otherwise b + r c gives the point where two vectors originating from
  // a and originating from b would intersect
  return r > 0. and r < 1.;
}

double util::distanceBetweenLines(const std::pair<util::Point, util::Point> &line_1,
                            const std::pair<util::Point, util::Point>
                                &line_2) {

  // let line 1 is l1(s) = p + s u
  // and line 2 is l2(r) = q + r v
  // and let normal to the plane containing line 1 and 2 is
  // n = u x v / |u x v|

  auto u = line_1.second - line_1.first;
  auto v = line_2.second - line_2.first;
  auto w0 = line_1.first - line_2.first;

  double a = u * u;
  double b = u * v;
  double c = v * v;
  double d = u * w0;
  double e = v * w0;

  auto dp = w0 + ((b * e - c * d) * u + (a * e - b * d) * v) / (a * c - b * b);

  return dp.length();
}

double
util::distanceBetweenSegments(const std::pair<util::Point, util::Point> &line_1,
                        const std::pair<util::Point, util::Point> &line_2) {

  // let line 1 is l1(s) = p + s u
  // and line 2 is l2(r) = q + r v
  // and let normal to the plane containing line 1 and 2 is
  // n = u x v / |u x v|

  auto u = line_1.second - line_1.first;
  auto v = line_2.second - line_2.first;
  auto w0 = line_1.first - line_2.first;

  double a = u * u;
  double b = u * v;
  double c = v * v;
  double d = u * w0;
  double e = v * w0;
  double D = a * c - b * b;
  double sc, sN, sD = D;
  double tc, tN, tD = D;

  // compute line parameters of two closest points
  if (D < 1.0E-12) {

    sN = 0.;
    sD = 1.;
    tN = e;
    tD = c;
  } else {

    sN = b * e - c * d;
    tN = a * e - b * d;

    if (sN < 0.) {
      sN = 0.;
      tN = e;
      tD = c;
    } else if (sN > sD) {
      sN = sD;
      tN = e + b;
      tD = c;
    }
  }

  if (tN < 0.) {

    tN = 0.;

    if (-d < 0.)
      sN = 0.;
    else if (-d > a)
      sN = sD;
    else {
      sN = -d;
      sD = a;
    }
  } else if (tN > tD) {

    tN = tD;

    if (-d + b < 0.)
      sN = 0.;
    else if (-d + b > a)
      sN = sD;
    else {
      sN = -d + b;
      sD = a;
    }
  }

  sc = std::abs(sN) < 1.0E-12 ? 0. : sN / sD;
  tc = std::abs(tN) < 1.0E-12 ? 0. : tN / tD;

  auto dp = w0 + sc * u - tc * v;

  return dp.length();
}

double util::distanceBetweenPlanes(const std::pair<util::Point, util::Point> &plane_1,
                             const std::pair<util::Point, util::Point>
                                 &plane_2) {

  // check if planes are parallel
  if (util::angle(plane_1.first, plane_2.first) < 1.0E-8)
    return 0.;

  return std::abs(plane_1.first * (plane_1.second - plane_2.second)) /
         plane_1.first.length();
}

double util::pointDistanceLine(const util::Point &p,
                         const std::pair<util::Point, util::Point> &line) {

  // line vector
  auto v = line.second - line.first;

  // vector from 1st point of line to p
  auto w = p - line.first;

  // project w onto v and add 1st point to get projected point's location
  auto w_on_line = line.first + (w * v) * v / v.lengthSq();

  return (p - w_on_line).length();
}

double util::pointDistanceSegment(const util::Point &p,
                                         const std::pair<util::Point, util::Point> &line) {

  // line vector
  auto v = line.second - line.first;

  // vector from 1st point of line to p
  auto w = p - line.first;

  // determine if w is on left side or right side of line
  double w_dot_v = w * v;
  if (w_dot_v < 1.0E-12)
    return (p - line.first).length();

  if (w_dot_v > v.lengthSq() - 1.0E-12)
    return (p - line.second).length();

  // project w onto v and add 1st point to get projected point's location
  auto w_on_line = line.first + w_dot_v * v / v.lengthSq();

  return (p - w_on_line).length();
}

double util::pointDistancePlane(const util::Point &p,
                          const std::pair<util::Point, util::Point> &plane) {

  // if plane is given by unit normal n and a point a which is contained in it
  // then the distance of point p from plane is
  // |(p - a) dot n| / |n|

  auto pa = p - plane.second;
  return std::abs(pa * plane.first) / plane.first.length();
}

double util::computeMeshSize(const std::vector<util::Point> &nodes) {

  double guess = 0.;
  if (nodes.size() < 2)
    return guess;

  guess = (nodes[0] - nodes[1]).length();
  for (size_t i = 0; i < nodes.size(); i++)
    for (size_t j = 0; j < nodes.size(); j++)
      if (i != j) {
        double val = nodes[i].dist(nodes[j]);

        if (util::isLess(val, 1.0E-12)) {

          std::cout << "Check nodes are too close = "
                    << util::io::printStr<util::Point>({nodes[i],
                                                         nodes[j]})
                    << "\n";
          std::cout << "Distance = " << val << ", guess = " << guess << "\n";
        }
        if (util::isLess(val, guess))
          guess = val;
      }

  return guess;
}

double util::computeMeshSize(const std::vector<util::Point> &nodes, size_t start,
                                       size_t end) {

  double guess = 0.;
  if (nodes.size() < 2 or (end - start) < 2)
    return guess;

  guess = (nodes[start] - nodes[start + 1]).length();
  for (size_t i = start; i < end; i++)
    for (size_t j = start; j < end; j++)
      if (i != j) {
        double val = nodes[i].dist(nodes[j]);

        if (util::isLess(val, 1.0E-12)) {

          std::cout << "Check nodes are too close = "
                    << util::io::printStr<util::Point>({nodes[i],
                                                         nodes[j]})
                    << "\n";
          std::cout << "Distance = " << val << ", guess = " << guess << "\n";
        }
        if (util::isLess(val, guess))
          guess = val;
      }

  return guess;
}

std::pair<util::Point, util::Point> util::computeBBox(const std::vector<util::Point> &nodes) {

  auto p1 = util::Point();
  auto p2 = util::Point();
  for (const auto& x : nodes) {
    if (util::isLess(x.d_x, p1.d_x))
      p1.d_x = x.d_x;
    if (util::isLess(x.d_y, p1.d_y))
      p1.d_y = x.d_y;
    if (util::isLess(x.d_z, p1.d_z))
      p1.d_z = x.d_z;
    if (util::isLess(p2.d_x, x.d_x))
      p2.d_x = x.d_x;
    if (util::isLess(p2.d_y, x.d_y))
      p2.d_y = x.d_y;
    if (util::isLess(p2.d_z, x.d_z))
      p2.d_z = x.d_z;
  }

  return {p1, p2};
}

double util::computeInscribedRadius(const std::pair<util::Point, util::Point>
                               &box) {

  return 0.5 * (box.first - box.second).length();
}

std::pair<util::Point, util::Point> util::toPointBox(const std::vector<double>
                                                 &p1, const std::vector<double>
                                                 &p2) {
  auto q1 = util::Point(p1[0], p1[1], p1[2]);
  auto q2 = util::Point(p2[0], p2[1], p2[2]);
  return {q1, q2};
}

double util::triangleArea(const util::Point &x1, const util::Point &x2,
                    const util::Point &x3) {
  return 0.5 * ((x2.d_x - x1.d_x) * (x3.d_y - x1.d_y) -
                (x3.d_x - x1.d_x) * (x2.d_y - x1.d_y));
}