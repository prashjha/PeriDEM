////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "geomObjects.h"
#include "function.h"
#include "geom.h"
#include "methods.h"
#include <vector>

//
// Line
//
double util::geometry::Line::volume() const {
  return (d_x2 - d_x1).length();
}

util::Point util::geometry::Line::center() const {
  return 0.5 * (d_x1 + d_x2);
}

std::pair<util::Point, util::Point> util::geometry::Line::box() const {

  return box(0.);
}
std::pair<util::Point, util::Point> util::geometry::Line::box(const
                                                                     double &tol) const {

  return {d_x1 - tol, d_x2 + tol};
}

double util::geometry::Line::inscribedRadius() const {

  return 0.5 * this->volume();
}

double util::geometry::Line::boundingRadius() const {

  return 0.5 * this->volume();
}

bool util::geometry::Line::isInside(const util::Point &x) const {

  double l = this->volume();

  auto da = (d_x2 - d_x1) / l;
  auto db = x - d_x1;
  double dot = db * da;

  if (util::isLess(dot, 0.) or util::isGreater(dot, l))
    return false;


  auto dx = db - dot * da;

  return util::isLess(dx.length(), 1.0E-10);
}

bool util::geometry::Line::isOutside(const util::Point &x) const {

  return !isInside(x);
}

bool util::geometry::Line::isNear(const util::Point &x, const double &tol) const {

  double l = this->volume();

  auto da = (d_x2 - d_x1) / l;
  auto db = x - d_x1;
  double dot = db * da;

  if (util::isLess(dot, 0.) or util::isGreater(dot, l))
    return false;

  auto dx = db - dot * da;

  return util::isLess(dx.length(), tol);
}

bool util::geometry::Line::isNearBoundary(const util::Point &x,
                                               const double &tol, const bool
                                               &within) const {

  // check if particle is inside the object
  if (!isNear(x, within ? 0. : tol))
    return false;

  double l = this->volume();

  auto da = (d_x2 - d_x1) / l;
  auto db = x - d_x1;
  double dot = db * da;

  if (util::isLess(dot, 0.) or util::isGreater(dot, tol) or util::isGreater(dot, l) or util::isLess(dot, l - tol))
    return false;

  auto dx = db - dot * da;

  return util::isLess(dx.length(), tol);
}

bool util::geometry::Line::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Line::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  return true;
}

bool util::geometry::Line::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  return true;
}

bool util::geometry::Line::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  return true;
}

bool util::geometry::Line::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  return false;
}

std::string util::geometry::Line::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Line --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Point 1 = " << d_x1.printStr(0, lvl) << std::endl;
  oss << tabS << "Point 2 = " << d_x2.printStr(0, lvl) << std::endl;
  oss << std::endl;

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Triangle
//
double util::geometry::Triangle::volume() const {
  return d_r * d_r * (1. + std::sin(M_PI/3));
}

util::Point util::geometry::Triangle::center() const {
  return d_x;
}

std::pair<util::Point, util::Point> util::geometry::Triangle::box() const {

  return box(0.);
}
std::pair<util::Point, util::Point> util::geometry::Triangle::box(const
                                                                     double &tol) const {

  // find lower and upper coordinates
  auto p1 = util::Point(d_x.d_x - d_r, d_x.d_y - d_r, d_x.d_z);
  auto p2 = util::Point(d_x.d_x + d_r, d_x.d_y + d_r, d_x.d_z);

  return {p1 - tol, p2 + tol};
}

double util::geometry::Triangle::inscribedRadius() const {

  return d_r * std::sin(M_PI/3);
}

double util::geometry::Triangle::boundingRadius() const {

  return d_r;
}

bool util::geometry::Triangle::isInside(const util::Point &x) const {

  if ((x - d_x).length() > d_r)
    return false;

  if ((x - d_x).length() < this->inscribedRadius())
    return true;

  double a = this->volume();
  double a1 =
      std::abs(util::triangleArea(x, d_vertices[1], d_vertices[2]));
  double a2 =
      std::abs(util::triangleArea(d_vertices[0], x, d_vertices[2]));
  double a3 =
      std::abs(util::triangleArea(d_vertices[0], d_vertices[1], x));

  return (a1+a2+a3 < a);
}

bool util::geometry::Triangle::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Triangle::isNear(const util::Point &x,
                                       const double &tol) const {

  // get a bigger box containing this object
  auto bbox = box(tol);

  return util::isPointInsideBox(x, 2, bbox);
}

bool util::geometry::Triangle::isNearBoundary(const util::Point &x,
                                               const double &tol, const bool
                                               &within) const {

  // check if particle is inside the object
  if (!isNear(x, within ? 0. : tol))
    return false;

  double a = this->volume();
  double l = 0.5 * std::sqrt(a);

  double a1 =
      std::abs(util::triangleArea(x, d_vertices[1], d_vertices[2]));
  if (a1 < tol*l)
    return true;

  double a2 =
      std::abs(util::triangleArea(d_vertices[0], x, d_vertices[2]));
  if (a2 < tol*l)
    return true;

  double a3 =
      std::abs(util::triangleArea(d_vertices[0], d_vertices[1], x));
  if (a3 < tol*l)
    return true;

  return false;
}

bool util::geometry::Triangle::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Triangle::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(2, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Triangle::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(2, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Triangle::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  return util::areBoxesNear(this->box(), box, tol, 2);
}

bool util::geometry::Triangle::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(2, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::Triangle::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Triangle --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
  oss << tabS << "Radius = " << d_r << std::endl;
  oss << tabS << "Vertices = " << util::io::printStr(d_vertices) << std::endl;
  oss << std::endl;

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Rectangle
//
double util::geometry::Rectangle::volume() const {
  return std::abs((d_x2.d_x - d_x1.d_x) * (d_x2.d_y - d_x1.d_y));
}

util::Point util::geometry::Rectangle::center() const {
  return d_x1 * 0.5 + d_x2 * 0.5;
}

std::pair<util::Point, util::Point> util::geometry::Rectangle::box() const {

  return box(0.);
}
std::pair<util::Point, util::Point> util::geometry::Rectangle::box(const
                                                                     double &tol) const {

  return {util::Point(d_x1.d_x - tol, d_x1.d_y - tol,
                       0.),
          util::Point(d_x2.d_x + tol, d_x2.d_y + tol,
                       0.)};
}

double util::geometry::Rectangle::inscribedRadius() const {

  double r = 0.5 * std::abs(d_x2.d_x - d_x1.d_x);
  if (util::isGreater(r, 0.5 * std::abs(d_x2.d_y - d_x1.d_y)))
    return 0.5 * std::abs(d_x2.d_y - d_x1.d_y);
  else
    return r;
}

double util::geometry::Rectangle::boundingRadius() const {

  return 0.5 * (d_x2 - d_x1).length();
}

bool util::geometry::Rectangle::isInside(const util::Point &x) const {
  return util::isPointInsideRectangle(x, d_x1, d_x2);
}

bool util::geometry::Rectangle::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Rectangle::isNear(const util::Point &x,
                                       const double &tol) const {

  // get a bigger box containing this object
  auto bbox = box(tol);

  return util::isPointInsideBox(x, 2, bbox);
}

bool util::geometry::Rectangle::isNearBoundary(const util::Point &x,
                                               const double &tol, const bool
                                               &within) const {

  // check if particle is inside the object
  if (!isNear(x, within ? 0. : tol))
    return false;

  bool near_x_edge = util::isLess(std::abs(x.d_x - d_x1.d_x), tol) or
      util::isLess(std::abs(x.d_x - d_x2.d_x), tol);

  bool near_y_edge = util::isLess(std::abs(x.d_y - d_x1.d_y), tol) or
      util::isLess(std::abs(x.d_y - d_x2.d_y), tol);

  return near_x_edge || near_y_edge;
}

bool util::geometry::Rectangle::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Rectangle::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(2, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Rectangle::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(2, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Rectangle::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  return util::areBoxesNear(this->box(), box, tol, 2);
}

bool util::geometry::Rectangle::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(2, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::Rectangle::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Rectangle --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Point 1 = " << d_x1.printStr(0, lvl) << std::endl;
  oss << tabS << "Point 2 = " << d_x2.printStr(0, lvl) << std::endl;
  oss << std::endl;

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Hexagon
//
double util::geometry::Hexagon::volume() const {
  // https://en.wikipedia.org/wiki/Hexagon
  double r_small = this->inscribedRadius();
  return 2. * std::sqrt(3.) * r_small * r_small;
}

util::Point util::geometry::Hexagon::center() const {
  return d_x;
}

std::pair<util::Point, util::Point> util::geometry::Hexagon::box() const {

  return box(0.);
}
std::pair<util::Point, util::Point> util::geometry::Hexagon::box(const
                                                                    double &tol) const {

  auto p1 = d_x - util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
  auto p2 = d_x + util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
  return {p1, p2};
}

double util::geometry::Hexagon::inscribedRadius() const {

  return d_r * 0.5 * std::sqrt(3.);
}

double util::geometry::Hexagon::boundingRadius() const {

  return d_r;
}

bool util::geometry::Hexagon::isInside(const util::Point &x) const {

  if ((x-d_x).length() > d_r)
    return false;

  if ((x-d_x).length() < inscribedRadius())
    return true;
}

bool util::geometry::Hexagon::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Hexagon::isNear(const util::Point &x,
                                      const double &tol) const {

  // get a bigger box containing this object
  auto bbox = box(tol);

  return util::isPointInsideBox(x, 2, bbox);
}

bool util::geometry::Hexagon::isNearBoundary(const util::Point &x,
                                              const double &tol, const bool
                                              &within) const {

  if ((x-d_x).length() > d_r + tol)
    return false;

  if ((x-d_x).length() < inscribedRadius() - tol)
    return false;

  return true;
}

bool util::geometry::Hexagon::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Hexagon::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(2, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Hexagon::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(2, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Hexagon::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  return util::areBoxesNear(this->box(), box, tol, 2);
}

bool util::geometry::Hexagon::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(2, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::Hexagon::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Hexagon --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Radius = " << d_r << std::endl;
  oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
  oss << tabS << "Axis = " << d_a.printStr(0, lvl) << std::endl;
  oss << tabS << "Vertices = " << util::io::printStr(d_vertices, lvl) <<
      std::endl;
  oss << std::endl;

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Drum2D
//
double util::geometry::Drum2D::volume() const {

  return (2.*d_r*d_r - d_r*(d_r - 2.*d_w))*std::sin(M_PI/3.);
}

util::Point util::geometry::Drum2D::center() const {
  return d_x;
}

std::pair<util::Point, util::Point> util::geometry::Drum2D::box() const {

  return box(0.);
}
std::pair<util::Point, util::Point> util::geometry::Drum2D::box(const
                                                                   double &tol) const {

  auto p1 = d_x - util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
  auto p2 = d_x + util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
  return {p1, p2};
}

double util::geometry::Drum2D::inscribedRadius() const {

  return d_w;
}

double util::geometry::Drum2D::boundingRadius() const {

  return d_r;
}

bool util::geometry::Drum2D::isInside(const util::Point &x) const {

  if ((x-d_x).length() > d_r)
    return false;

  if ((x-d_x).length() < inscribedRadius())
    return true;

  // rotate axis to get orthogonal axis
  auto ortho_axis = util::rotate(d_a, M_PI*0.5, util::Point(0., 0., 1.));

  //
  //                                   + v2
  //                                  /
  //                                /    x
  //                              /
  //                            /
  //                     o----+v1
  //
  auto ox = x - d_x;
  double angle_ox_ov1 = std::acos(std::abs(d_a.dot(ox))/ox.length());
  double max_length = d_w + angle_ox_ov1 * (d_r - d_w)/(M_PI/3.);

  return ox.length() <= max_length;
}

bool util::geometry::Drum2D::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Drum2D::isNear(const util::Point &x,
                                     const double &tol) const {

  // get a bigger box containing this object
  auto bbox = box(tol);

  return util::isPointInsideBox(x, 2, bbox);
}

bool util::geometry::Drum2D::isNearBoundary(const util::Point &x,
                                             const double &tol, const bool
                                             &within) const {

  if ((x-d_x).length() > d_r + tol)
    return false;

  if ((x-d_x).length() < this->inscribedRadius() - tol)
    return false;

  return true;
}

bool util::geometry::Drum2D::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Drum2D::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(2, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Drum2D::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(2, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Drum2D::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  return util::areBoxesNear(this->box(), box, tol, 2);
}

bool util::geometry::Drum2D::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(2, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::Drum2D::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Drum2D --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Radius = " << d_r << std::endl;
  oss << tabS << "Neck half-width = " << d_w << std::endl;
  oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
  oss << tabS << "Axis = " << d_a.printStr(0, lvl) << std::endl;
  oss << tabS << "Vertices = " << util::io::printStr(d_vertices, lvl) <<
      std::endl;
  oss << std::endl;

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Cuboid
//
double util::geometry::Cuboid::volume() const {
  return std::abs((d_x2.d_x - d_x1.d_x) * (d_x2.d_y - d_x1.d_y) * (d_x2.d_z -
                                                                   d_x1.d_z));
}

util::Point util::geometry::Cuboid::center() const {
  return d_x1 * 0.5 + d_x2 * 0.5;
}

std::pair<util::Point, util::Point> util::geometry::Cuboid::box() const {

  return box(0.);
}
std::pair<util::Point, util::Point> util::geometry::Cuboid::box(const
                                                                  double &tol) const {
  return {util::Point(d_x1.d_x - tol, d_x1.d_y - tol,
                       d_x1.d_z - tol),
          util::Point(d_x2.d_x + tol, d_x2.d_y + tol,
                       d_x2.d_z + tol)};
}

double util::geometry::Cuboid::inscribedRadius() const {

  double r = 0.5 * std::abs(d_x2.d_x - d_x1.d_x);
  if (util::isGreater(r, 0.5 * std::abs(d_x2.d_y - d_x1.d_y)))
    r = 0.5 * std::abs(d_x2.d_y - d_x1.d_y);

  if (util::isGreater(r, 0.5 * std::abs(d_x2.d_z - d_x1.d_z)))
    return 0.5 * std::abs(d_x2.d_z - d_x1.d_z);
  else
    return r;
}

double util::geometry::Cuboid::boundingRadius() const {

  return 0.5 * (d_x2 - d_x1).length();
}

bool util::geometry::Cuboid::isInside(const util::Point &x) const {
  return util::isPointInsideCuboid(x, d_x1, d_x2);
}

bool util::geometry::Cuboid::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Cuboid::isNear(const util::Point &x,
                                    const double &tol) const {

  // get a bigger box containing this object
  auto bbox = box(tol);

  return util::isPointInsideBox(x, 3, bbox);
}

bool util::geometry::Cuboid::isNearBoundary(const util::Point &x,
                                            const double &tol, const bool
                                            &within) const {

  // check if particle is within the tolerance distance
  if (!isNear(x, within ? 0. : tol))
    return false;

  bool near_x_edge = util::isLess(std::abs(x.d_x - d_x1.d_x), tol) or
      util::isLess(std::abs(x.d_x - d_x2.d_x), tol);

  bool near_y_edge = util::isLess(std::abs(x.d_y - d_x1.d_y), tol) or
      util::isLess(std::abs(x.d_y - d_x2.d_y), tol);

  bool near_z_edge = util::isLess(std::abs(x.d_z - d_x1.d_z), tol) or
      util::isLess(std::abs(x.d_z - d_x2.d_z), tol);

  return near_x_edge || near_y_edge || near_z_edge;
}

bool util::geometry::Cuboid::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Cuboid::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(3, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Cuboid::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(3, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Cuboid::isNear(
    const std::pair<util::Point, util::Point> &bbox, const double &tol)
const {

  return util::areBoxesNear(box(), bbox, tol, 3);
}

bool util::geometry::Cuboid::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(3, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::Cuboid::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Cuboid --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Point 1 = " << d_x1.printStr(0, lvl) << std::endl;
  oss << tabS << "Point 2 = " << d_x2.printStr(0, lvl) << std::endl;
  oss << std::endl;

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Circle
//
double util::geometry::Circle::volume() const {
  return M_PI * d_r * d_r;
}

util::Point util::geometry::Circle::center() const {
  return d_xc;
}

std::pair<util::Point, util::Point> util::geometry::Circle::box() const {

  return box(0.);
}

std::pair<util::Point, util::Point> util::geometry::Circle::box(const
                                                                  double &tol) const {
  double r = d_r + tol;
  return {
      util::Point(d_xc.d_x - r, d_xc.d_y - r, 0.),
      util::Point(d_xc.d_x + r, d_xc.d_y + r, 0.)
  };
}

double util::geometry::Circle::inscribedRadius() const {

  return d_r;
}

double util::geometry::Circle::boundingRadius() const {

  return d_r;
}

bool util::geometry::Circle::isInside(const util::Point &x) const {

  return util::isLess(d_xc.dist(x), d_r + 1.0E-12);
}

bool util::geometry::Circle::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Circle::isNear(const util::Point &x,
                                    const double &tol) const {

  // translate to origin
  auto x0 = x - d_xc;

  return util::isLess(x0.length(), d_r + tol);
}

bool util::geometry::Circle::isNearBoundary(const util::Point &x,
                                            const double &tol, const bool
                                            &within) const {

  // check if particle is within the tolerance distance
  if (!isNear(x, within ? 0. : tol))
    return false;

  // check if it is close enough to circumference
  auto x0 = x - d_xc;

  return util::isLess(x0.length(), d_r + tol) ||
         util::isLess(x0.length(), d_r - tol);
}

bool util::geometry::Circle::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Circle::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(2, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Circle::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(2, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Circle::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  if (this->isInside(box))
    return true;

  // get corner points of box
  auto cp = getCornerPoints(2, box);

  for (auto p: cp) {

    // check the distance of corner point with the center
    auto dx = p - d_xc;
    if(util::isLess(dx.length(), d_r + tol))
      return true;
  }

  // check center to center distance
  auto dxc = util::getCenter(2, box) - d_xc;

  // check wrt inscribed circle
  auto r = util::inscribedRadiusInBox(2, box);
  if (util::isLess(dxc.length(), d_r + r + tol))
    return true;

  // check wrt circumscribed circle
  r = util::circumscribedRadiusInBox(2, box);
  return util::isLess(dxc.length(), d_r + r + tol);
}

bool util::geometry::Circle::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(2, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::Circle::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Circle --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Center = " << d_xc.printStr(0, lvl) << std::endl;
  oss << tabS << "Radius = " << d_r << std::endl;

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Cylinder
//
double util::geometry::Cylinder::volume() const {
  return M_PI * d_r * d_r * d_l;
}

util::Point util::geometry::Cylinder::center() const {
  return d_xBegin + 0.5 * d_l * d_xa;
}

std::pair<util::Point, util::Point> util::geometry::Cylinder::box() const {

  return box(0.);
}

std::pair<util::Point, util::Point>
util::geometry::Cylinder::box(const double &tol) const {

  if (d_xa.length() < 1.0E-10)
    return {util::Point(), util::Point()};

  auto xb = d_xBegin - tol * d_xa;
  auto xt = d_xBegin + (d_l + tol) * d_xa;

  double r = d_r + tol;

  return {xb - r, xt + r};
}

double util::geometry::Cylinder::inscribedRadius() const {

  auto box = this->box();

  return 0.5 * (box.second - box.first).length();
}

double util::geometry::Cylinder::boundingRadius() const {

  return 0.5 * std::sqrt(d_l*d_l + 4.*d_r*d_r);
}

bool util::geometry::Cylinder::isInside(const util::Point &x) const {

  auto dx = x - d_xBegin;

  if (dx.length() < 1.0E-10)
    return true;

  double dx_dot_xa = dx * d_xa;
  if (util::isLess(dx_dot_xa, 0.) or
      util::isGreater(dx_dot_xa, d_l))
    return false;
  else {

    // project dx onto cross-section plane of cylinder
    auto dx_project = dx - dx_dot_xa * d_xa;

    return !util::isGreater(dx_project.length(), d_r + 1.0E-12);
  }
}

bool util::geometry::Cylinder::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Cylinder::isNear(const util::Point &x, const double
&tol) const {

  auto dx = x - d_xBegin;

  if (dx.length() < tol)
    return true;

  double dx_dot_xa = dx * d_xa;
  if (util::isLess(dx_dot_xa, -tol) or
      util::isGreater(dx_dot_xa, d_l + tol))
    return false;
  else {

    // project dx onto cross-section plane of cylinder
    auto dx_project = dx - dx_dot_xa * d_xa;

    return !util::isGreater(dx_project.length(), d_r + tol);
  }
}

bool util::geometry::Cylinder::isNearBoundary(const util::Point &x,
                                              const double &tol, const bool
                                              &within) const {

  auto dx = x - d_xBegin;

  if (dx.length() < tol)
    return true;

  double dx_dot_xa = dx * d_xa;
  if (util::isLess(dx_dot_xa, -tol) or
      util::isGreater(dx_dot_xa, tol) or
      util::isGreater(dx_dot_xa, d_l + tol) or
      util::isLess(dx_dot_xa, d_l - tol))
    return false;
  else {

    // project dx onto cross-section plane of cylinder
    auto dx_project = dx - dx_dot_xa * d_xa;

    return !(util::isLess(dx_project.length(), d_r - tol) or
             util::isGreater(dx_project.length(), d_r + tol));
  }
}

bool util::geometry::Cylinder::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Cylinder::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(3, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Cylinder::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(3, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Cylinder::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  return util::areBoxesNear(this->box(), box, tol, 3);
}

bool util::geometry::Cylinder::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(3, box))
    if (this->isInside(p))
      return true;

  return false;
}


std::string util::geometry::Cylinder::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Cylinder --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Center = " << d_xBegin.printStr(0, lvl) << std::endl;
  oss << tabS << "Axis = " << d_xa.printStr(0, lvl) << std::endl;
  oss << tabS << "Radius = " << d_r << std::endl;

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// Sphere
//
double util::geometry::Sphere::volume() const {
  return 4. * M_PI * d_r * d_r * d_r / 3.;
}

util::Point util::geometry::Sphere::center() const {
  return d_xc;
}

std::pair<util::Point, util::Point> util::geometry::Sphere::box() const {

  return box(0.);
}
std::pair<util::Point, util::Point> util::geometry::Sphere::box(const
                                                                  double &tol) const {
  double r = d_r + tol;

  return {
      util::Point(d_xc.d_x - r, d_xc.d_y - r, d_xc.d_z - r),
      util::Point(d_xc.d_x + r, d_xc.d_y + r, d_xc.d_z + r)
  };
}

double util::geometry::Sphere::inscribedRadius() const {

  return d_r;
}

double util::geometry::Sphere::boundingRadius() const {

  return d_r;
}

bool util::geometry::Sphere::isInside(const util::Point &x) const {

  return util::isLess(d_xc.dist(x), d_r + 1.0E-12);
}

bool util::geometry::Sphere::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::Sphere::isNear(const util::Point &x,
                                    const double &tol) const {

  // translate to origin
  auto x0 = x - d_xc;

  return util::isLess(x0.length(), d_r + tol);
}

bool util::geometry::Sphere::isNearBoundary(const util::Point &x,
                                            const double &tol, const bool
                                            &within) const {

  // check if particle is within the tolerance distance
  if (!isNear(x, within ? 0. : tol))
    return false;

  // check if it is close enough to circumference
  auto x0 = x - d_xc;

  return util::isLess(x0.length(), d_r + tol) ||
         util::isLess(x0.length(), d_r - tol);
}

bool util::geometry::Sphere::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::Sphere::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(3, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::Sphere::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(3, box))
    if (!intersect)
      intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::Sphere::isNear(
    const std::pair<util::Point, util::Point> &box, const double &tol) const {

  if (this->isInside(box))
    return true;

  // get corner points of box
  auto cp = getCornerPoints(3, box);

  for (auto p: cp) {

    // check the distance of corner point with the center
    auto dx = p - d_xc;
    if(util::isLess(dx.length(), d_r + tol))
      return true;
  }

  // check center to center distance
  auto dxc = util::getCenter(3, box) - d_xc;

  // check wrt inscribed circle
  auto r = util::inscribedRadiusInBox(3, box);
  if (util::isLess(dxc.length(), d_r + r + tol))
    return true;

  // check wrt circumscribed circle
  r = util::circumscribedRadiusInBox(3, box);
  return util::isLess(dxc.length(), d_r + r + tol);
}

bool util::geometry::Sphere::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(3, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::Sphere::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- Sphere --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Center = " << d_xc.printStr(0, lvl) << std::endl;
  oss << tabS << "Radius = " << d_r << std::endl;

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// AnnulusGeomObject
//
double util::geometry::AnnulusGeomObject::volume() const {
  return d_outObj_p->volume() - d_inObj_p->volume();
}

util::Point util::geometry::AnnulusGeomObject::center() const {

  return d_outObj_p->center();
}

std::pair<util::Point, util::Point> util::geometry::AnnulusGeomObject::box
    () const {
  return d_outObj_p->box();
}

std::pair<util::Point, util::Point> util::geometry::AnnulusGeomObject::box
    (const double &tol) const {
  return d_outObj_p->box(tol);
}

double util::geometry::AnnulusGeomObject::inscribedRadius() const {

  auto box = d_outObj_p->box();
  return 0.5 * (box.first - box.second).length();
}

double util::geometry::AnnulusGeomObject::boundingRadius() const {

  auto box = d_outObj_p->box();
  return 0.5 * (box.first - box.second).length();
}

bool util::geometry::AnnulusGeomObject::isInside(const util::Point &x) const {

  // should be outside inner object and inside outer object
  return d_inObj_p->isInside(x) ? false : d_outObj_p->isInside(x);
}

bool util::geometry::AnnulusGeomObject::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::AnnulusGeomObject::isNear(const util::Point &x,
                                    const double &tol) const {

  return d_outObj_p->isNear(x, tol) || d_inObj_p->isNear(x, tol);
}

bool util::geometry::AnnulusGeomObject::isNearBoundary(const util::Point &x,
                                            const double &tol, const bool
                                            &within) const {

  return d_outObj_p->isNearBoundary(x, tol, within) ||
         d_inObj_p->isNearBoundary(x, tol, within);
}

bool util::geometry::AnnulusGeomObject::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::AnnulusGeomObject::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(d_dim, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::AnnulusGeomObject::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(d_dim, box))
    intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::AnnulusGeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                                               const double &tol) const {

  return d_outObj_p->isNear(box, tol) || d_inObj_p->isNear(box, tol);
}

bool util::geometry::AnnulusGeomObject::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(d_dim, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::AnnulusGeomObject::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- AnnulusGeomObject --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Center = " << center().printStr() << std::endl;
  oss << tabS << "Inner object info:" << std::endl;
  oss << d_inObj_p->printStr(nt+1, lvl);
  oss << tabS << "Outer object info:" << std::endl;
  oss << d_outObj_p->printStr(nt+1, lvl);

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}

//
// ComplexGeomObject
//
double util::geometry::ComplexGeomObject::volume() const {

  double volume = 0.;
  for (size_t i=0; i<d_objFlag.size(); i++)
    volume += d_obj[i]->volume() * d_objFlagInt[i];

  return volume;
}

util::Point util::geometry::ComplexGeomObject::center() const {

  auto center = util::Point();
  for (size_t i=0; i<d_objFlag.size(); i++)
    center += d_obj[i]->center();

  return center;
}

std::pair<util::Point, util::Point> util::geometry::ComplexGeomObject::box
    () const {

  return box(0.);
}

std::pair<util::Point, util::Point> util::geometry::ComplexGeomObject::box
    (const double &tol) const {

  auto p1 = d_obj[0]->box(tol).first;
  auto p2 = d_obj[0]->box(tol).second;

  for (size_t i=1; i<d_objFlag.size(); i++) {

    auto q1 = d_obj[i]->box(tol).first;
    auto q2 = d_obj[i]->box(tol).second;

    for (size_t i=0; i<3; i++) {
      if (q1[i] < p1[i])
        p1[i] = q1[i];
      if (q2[i] > p2[i])
        p2[i] = q2[i];
    }
  }

  return std::make_pair(p1, p2);
}

double util::geometry::ComplexGeomObject::inscribedRadius() const {

  auto box = this->box();
  return 0.5 * (box.first - box.second).length();
}

double util::geometry::ComplexGeomObject::boundingRadius() const {

  auto box = this->box();
  return 0.5 * (box.first - box.second).length();
}

bool util::geometry::ComplexGeomObject::isInside(const util::Point &x) const {

  // point inside means x should be inside in the object with plus flag and
  // outside in the object with minus flag
  bool point_inside = d_obj[0]->isInside(x);
  for (size_t i=1; i<d_objFlag.size(); i++) {

    const auto &obj_i = d_obj[i];
    if (d_objFlagInt[i] < 0)
      point_inside = point_inside and !obj_i->isInside(x);
    else
      point_inside = point_inside or obj_i->isInside(x);
  }

  return point_inside;
}

bool util::geometry::ComplexGeomObject::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool util::geometry::ComplexGeomObject::isNear(const util::Point &x,
                                               const double &tol) const {

  bool is_near = d_obj[0]->isNear(x, tol);
  for (size_t i=1; i<d_objFlag.size(); i++) {

    const auto &obj_i = d_obj[i];
    is_near = is_near or obj_i->isNear(x, tol);
  }

  return is_near;
}

bool util::geometry::ComplexGeomObject::isNearBoundary(const util::Point &x,
                                                       const double &tol, const bool
                                                       &within) const {

  bool is_near = d_obj[0]->isNearBoundary(x, tol, within);
  for (size_t i=1; i<d_objFlag.size(); i++) {

    const auto &obj_i = d_obj[i];
    is_near = is_near or obj_i->isNearBoundary(x, tol, within);
  }

  return is_near;
}

bool util::geometry::ComplexGeomObject::doesIntersect(const util::Point &x) const {

  return isNearBoundary(x, 1.0E-8, false);
}

bool util::geometry::ComplexGeomObject::isInside(
    const std::pair<util::Point, util::Point> &box) const {

  for (auto p : util::getCornerPoints(d_dim, box))
    if(!this->isInside(p))
      return false;

  return true;
}

bool util::geometry::ComplexGeomObject::isOutside(
    const std::pair<util::Point, util::Point> &box) const {

  bool intersect = false;
  for (auto p : util::getCornerPoints(d_dim, box))
    intersect = this->isInside(p);

  return !intersect;
}

bool util::geometry::ComplexGeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                                               const double &tol) const {

  bool is_near = d_obj[0]->isNear(box, tol);
  for (size_t i=1; i<d_objFlag.size(); i++) {

    const auto &obj_i = d_obj[i];
    is_near = is_near or obj_i->isNear(box, tol);
  }

  return is_near;
}

bool util::geometry::ComplexGeomObject::doesIntersect(
    const std::pair<util::Point, util::Point> &box) const {

  // need to check all four corner points
  for (auto p : util::getCornerPoints(d_dim, box))
    if (this->isInside(p))
      return true;

  return false;
}

std::string util::geometry::ComplexGeomObject::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);

  std::ostringstream oss;

  oss << tabS << "------- ComplexGeomObject --------" << std::endl << std::endl;
  oss << tabS << "Name = " << d_name << std::endl;
  oss << tabS << "Center = " << center().printStr() << std::endl;
  oss << tabS << "Object info:" << std::endl;
  auto ocount = 0;
  for (const auto &p: d_obj) {
    oss << tabS << "Object id: " << ocount << std::endl;
    oss << tabS << "Object flag: " << d_objFlag[ocount] << std::endl;
    oss << tabS << "Object int flag: " << d_objFlagInt[ocount] << std::endl;
    oss << p->printStr(nt+1, lvl);
    ocount++;
  }

  if (lvl > 0)
    oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

  if (lvl == 0)
    oss << std::endl;

  return oss.str();
}


//
// Utility functions
//
int util::geometry::getNumParamsNRequired(std::string geom_type) {

  if (geom_type == "circle")
    return 4;
  else if (geom_type == "sphere")
    return 4;
  else if (geom_type == "rectangle")
    return 6;
  else if (geom_type == "angled_rectangle")
    return 6;
  else if (geom_type == "cuboid")
    return 6;
  else if (geom_type == "cylinder")
    return 7;
  else if (geom_type == "hexagon")
    return 7;
  else if (geom_type == "triangle")
    return 7;
  else if (geom_type == "drum2d")
    return 8;
  else if (geom_type == "rectangle_minus_rectangle")
    return 12;
  else if (geom_type == "cuboid_minus_cuboid")
    return 12;
  else {
    std::cerr << "Error: Invalid geometry type: " << geom_type << std::endl;
    exit(1);
  }
}

bool util::geometry::checkParamForGeometry(size_t n, std::string geom_type) {

  return getNumParamsNRequired(geom_type) != n;
}

bool util::geometry::checkParamForComplexGeometry(size_t n, std::string geom_type,
    std::vector<std::string> vec_type) {

  int num_params = 0;
  for (const auto &s: vec_type)
    num_params += getNumParamsNRequired(s);

  return n != num_params;
}

void util::geometry::createGeomObject(const std::string &type, const std::vector<double> &params,
                      const std::vector<std::string> &vec_type,
                      const std::vector<std::string> &vec_flag,
                      std::shared_ptr<util::geometry::GeomObject> &obj,
                                      const size_t &dim, bool perform_check) {

  // for any of the objects below, issue error if number of parameters not
  // sufficient regardless of perform_check value
  std::vector<std::string> no_default_obj = {"rectangle", "cuboid",
                                             "cylinder", "complex",
      "rectangle_minus_rectangle", "cuboid_minus_cuboid"};

  bool check = false;
  if (type != "complex")
    check = checkParamForGeometry(params.size(), type);
  else
    check = checkParamForComplexGeometry(params.size(), type, vec_type);

  std::ostringstream oss;
  if (check) {
    oss << "Error: Data maybe invalid. Can not create geometrical object: "
        << type << " with params: " << util::io::printStr(params)
        << ", vec type: " << util::io::printStr(vec_type)
        << ", vec flag: " << util::io::printStr(vec_flag) << std::endl;
  }

  // issue error
  if (check) {
    if (perform_check || util::methods::isTagInList(type, no_default_obj)) {
      std::cerr << oss.str();
      exit(1);
    }
  }

  // create object
  if (type == "circle") {
    if (!check)
      obj = std::make_shared<util::geometry::Circle>(
          params[0], util::Point(params[1], params[2], params[3]));
    else {
      if (params.size() < 1) {
        std::cerr << "Error: need at least " << 1
                  << " parameters for creating circle. "
                     "Number of params provided = "
                  << params.size()
                  << ", params = "
                  << util::io::printStr(params) << " \n";
        exit(1);
      }

      obj = std::make_shared<util::geometry::Circle>(params[0], util::Point());
    }
  }
  else if (type == "rectangle") {
    if (!check)
      obj = std::make_shared<util::geometry::Rectangle>(
          util::Point(params[0], params[1], params[2]),
          util::Point(params[3], params[4], params[5]));
  }
  else if (type == "triangle") {
    if (!check)
      obj = std::make_shared<util::geometry::Triangle>(
          params[0], util::Point(params[1], params[2], params[3]),
          util::Point(params[4], params[5], params[6]));
    else {
      if (params.size() < 4) {
        std::cerr << "Error: need at least " << 4
                  << " parameters for creating triangle. "
                     "Number of params provided = "
                  << params.size()
                  << ", params = "
                  << util::io::printStr(params) << " \n";
        exit(1);
      }

      obj = std::make_shared<util::geometry::Triangle>(
          params[0], util::Point(params[1], params[2], params[3]));
    }
  }
  else if (type == "hexagon") {
    if (!check)
      obj = std::make_shared<util::geometry::Hexagon>(
        params[0], util::Point(params[1], params[2], params[3]),
        util::Point(params[4], params[5], params[6]));
    else {
      if (params.size() < 4) {
        std::cerr << "Error: need at least " << 4
                  << " parameters for creating hexagon. "
                     "Number of params provided = "
                  << params.size()
                  << ", params = "
                  << util::io::printStr(params) << " \n";
        exit(1);
      }

      obj = std::make_shared<util::geometry::Hexagon>(
          params[0], util::Point(params[1], params[2], params[3]));
    }
  }
  else if (type == "drum2d") {
    if (!check)
      obj = std::make_shared<util::geometry::Drum2D>(
          params[0], params[1],
          util::Point(params[2], params[3], params[4]),
          util::Point(params[5], params[6], params[7]));
    else {
      if (params.size() < 5) {
        std::cerr << "Error: need at least " << 5
                  << " parameters for creating dum2d. "
                     "Number of params provided = "
                  << params.size()
                  << ", params = "
                  << util::io::printStr(params) << " \n";
        exit(1);
      }

      obj = std::make_shared<util::geometry::Drum2D>(
          params[0], params[1], util::Point(params[2], params[3], params[4]));
    }
  }
  else if (type == "sphere") {
    if (!check)
      obj = std::make_shared<util::geometry::Sphere>(
        params[0], util::Point(params[1], params[2], params[3]));
    else {
      if (params.size() < 1) {
        std::cerr << "Error: need at least " << 1
                  << " parameters for creating sphere. "
                     "Number of params provided = "
                  << params.size()
                  << ", params = "
                  << util::io::printStr(params) << " \n";
        exit(1);
      }

      obj = std::make_shared<util::geometry::Sphere>(params[0], util::Point());
    }
  }
  else if (type == "cuboid") {
    if (!check)
      obj = std::make_shared<util::geometry::Cuboid>(
          util::Point(params[0], params[1], params[2]),
          util::Point(params[3], params[4], params[5]));
  }
  else if (type == "cylinder") {
    if (!check)
      obj = std::make_shared<util::geometry::Cylinder>(
          params[0], util::Point(params[1], params[2], params[3]),
          util::Point(params[4], params[5], params[6]));
  }
  else if (type == "rectangle_minus_rectangle") {
    auto rin = new util::geometry::Rectangle(
        util::Point(params[0], params[1],
                     params[2]),
        util::Point(params[3], params[4], params[5]));
    auto rout = new util::geometry::Rectangle(
        util::Point(params[6], params[7],
                     params[8]),
        util::Point(params[9], params[10],
                     params[11]));

    obj = std::make_shared<util::geometry::AnnulusGeomObject>
        (rin, rout, 2);
  } else if (type == "cuboid_minus_cuboid") {
    auto rin = new util::geometry::Cuboid(
        util::Point(params[0], params[1],
                     params[2]),
        util::Point(params[3], params[4],
                     params[5]));
    auto rout = new util::geometry::Cuboid(
        util::Point(params[6], params[7],
                     params[8]),
        util::Point(params[9], params[10],
                     params[11]));

    obj = std::make_shared<util::geometry::AnnulusGeomObject>
        (rin, rout, 3);
  } else if (type == "complex") {

    std::vector<std::shared_ptr<util::geometry::GeomObject>> vec_obj(vec_type.size());

    size_t param_start = 0;
    for (size_t i=0; i<vec_type.size(); i++) {
      auto geom_type = vec_type[i];
      auto geom_flag = vec_flag[i];
      auto num_params = getNumParamsNRequired(geom_type);

      // get slice of full param vector
      auto p1 = params.begin() + param_start;
      auto p2 = params.begin() + param_start + num_params;
      auto geom_param = std::vector<double>(p1, p2);

      // create geom object
      createGeomObject(geom_type, geom_param, std::vector<std::string>(),
                       std::vector<std::string>(), vec_obj[i], dim);

      param_start += num_params;
    }

    // create complex geom object
    ///std::cout << "creating complex object\n";
    obj = std::make_shared<util::geometry::ComplexGeomObject>(vec_obj, vec_flag, dim);
    //obj->print();
  }
}