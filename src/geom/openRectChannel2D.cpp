/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#include "openRectChannel2D.h"
#include "geomUtilFunctions.h"
#include "util/function.h"
#include "util/io.h"
#include "util/vecMethods.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace geom {

namespace {

void validateParams(double x0, double y0, double x1, double y1, double t) {
  constexpr double eps = 1.0e-12;
  if (!(x1 > x0 + eps && y1 > y0 + eps && t > eps))
    throw std::runtime_error(
        "OpenRectChannel2D: require x0 < x1, y0 < y1, t > 0.");
  if (!(x1 - x0 > 2. * t + eps && y1 - y0 > 2. * t + eps))
    throw std::runtime_error(
        "OpenRectChannel2D: require x1-x0 > 2t and y1-y0 > 2t so the U-channel exists.");
}

double distPointToSegment2D(const util::Point &p, const util::Point &a, const util::Point &b) {
  const util::Point ab = b - a;
  const util::Point ap = p - a;
  const double L2 = ab * ab;
  if (L2 < 1.0e-30)
    return ap.length();
  double s = (ap * ab) / L2;
  s = std::max(0., std::min(1., s));
  const util::Point proj = a + s * ab;
  return (p - proj).length();
}

} // namespace

OpenRectChannel2D::OpenRectChannel2D()
  : GeomObject("open_rect_channel_2d", "") {
}

OpenRectChannel2D::OpenRectChannel2D(double x0, double y0, double x1, double y1, double t,
                                     double z, std::string description)
  : GeomObject("open_rect_channel_2d", std::move(description)),
    d_x0(x0),
    d_y0(y0),
    d_x1(x1),
    d_y1(y1),
    d_t(t),
    d_z(z) {
  validateParams(x0, y0, x1, y1, t);
  buildVertices();
}

OpenRectChannel2D::OpenRectChannel2D(const OpenRectChannel2D &other)
  : GeomObject(other.d_name, other.d_description),
    d_x0(other.d_x0),
    d_y0(other.d_y0),
    d_x1(other.d_x1),
    d_y1(other.d_y1),
    d_t(other.d_t),
    d_z(other.d_z),
    d_vertices(other.d_vertices) {
  d_tags = other.d_tags;
}

OpenRectChannel2D &OpenRectChannel2D::operator=(const OpenRectChannel2D &other) {
  if (this != &other) {
    d_tags = other.d_tags;
    d_x0 = other.d_x0;
    d_y0 = other.d_y0;
    d_x1 = other.d_x1;
    d_y1 = other.d_y1;
    d_t = other.d_t;
    d_z = other.d_z;
    d_vertices = other.d_vertices;
  }
  return *this;
}

void OpenRectChannel2D::buildVertices() {
  const double x0 = d_x0, y0 = d_y0, x1 = d_x1, y1 = d_y1, t = d_t, z = d_z;
  d_vertices.clear();
  d_vertices.reserve(8);
  // CCW U-cavity boundary (open at +y between inner top corners)
  d_vertices.push_back({x0, y0, z});
  d_vertices.push_back({x1, y0, z});
  d_vertices.push_back({x1, y1, z});
  d_vertices.push_back({x1 - t, y1 - t, z});
  d_vertices.push_back({x1 - t, y0 + t, z});
  d_vertices.push_back({x0 + t, y0 + t, z});
  d_vertices.push_back({x0 + t, y1 - t, z});
  d_vertices.push_back({x0, y1, z});
}

bool OpenRectChannel2D::pointInPolygon2D(const util::Point &p, const std::vector<util::Point> &v) {
  if (v.size() < 3)
    return false;
  bool c = false;
  const size_t n = v.size();
  for (size_t i = 0, j = n - 1; i < n; j = i++) {
    const double xi = v[i].d_x, yi = v[i].d_y;
    const double xj = v[j].d_x, yj = v[j].d_y;
    if (((yi > p.d_y) != (yj > p.d_y)) &&
        (p.d_x < (xj - xi) * (p.d_y - yi) / (yj - yi + 1.0e-30) + xi))
      c = !c;
  }
  return c;
}

double OpenRectChannel2D::polygonArea2D(const std::vector<util::Point> &v) {
  if (v.size() < 3)
    return 0.;
  double a = 0.;
  const size_t n = v.size();
  for (size_t i = 0; i < n; ++i) {
    const size_t j = (i + 1) % n;
    a += v[i].d_x * v[j].d_y - v[j].d_x * v[i].d_y;
  }
  return 0.5 * std::abs(a);
}

util::Point OpenRectChannel2D::polygonCentroid2D(const std::vector<util::Point> &v) {
  if (v.size() < 3)
    return {};
  double cx = 0., cy = 0.;
  double a = 0.;
  const size_t n = v.size();
  for (size_t i = 0; i < n; ++i) {
    const size_t j = (i + 1) % n;
    const double cross = v[i].d_x * v[j].d_y - v[j].d_x * v[i].d_y;
    a += cross;
    cx += (v[i].d_x + v[j].d_x) * cross;
    cy += (v[i].d_y + v[j].d_y) * cross;
  }
  if (std::abs(a) < 1.0e-30)
    return {(v[0].d_x + v[1].d_x) * 0.5, (v[0].d_y + v[1].d_y) * 0.5, v[0].d_z};
  a *= 0.5;
  return {cx / (6. * a), cy / (6. * a), v[0].d_z};
}

void OpenRectChannel2D::transform(const util::Point &center, const double &scale,
                                  const double &angle, const util::Point &axis) {
  util::Point oc((d_x0 + d_x1) * 0.5, (d_y0 + d_y1) * 0.5, d_z);
  d_t *= scale;
  for (auto &v : d_vertices) {
    v = util::rotate((v - oc) * scale, angle, axis) + center;
  }
  d_x0 = d_x1 = d_vertices[0].d_x;
  d_y0 = d_y1 = d_vertices[0].d_y;
  d_z = d_vertices[0].d_z;
  for (const auto &v : d_vertices) {
    d_x0 = std::min(d_x0, v.d_x);
    d_x1 = std::max(d_x1, v.d_x);
    d_y0 = std::min(d_y0, v.d_y);
    d_y1 = std::max(d_y1, v.d_y);
    d_z = v.d_z;
  }
}

double OpenRectChannel2D::volume() const {
  return polygonArea2D(d_vertices);
}

util::Point OpenRectChannel2D::center() const {
  return polygonCentroid2D(d_vertices);
}

std::pair<util::Point, util::Point> OpenRectChannel2D::box() const {
  return box(0.);
}

std::pair<util::Point, util::Point> OpenRectChannel2D::box(const double &tol) const {
  util::Point lo(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                 std::numeric_limits<double>::max());
  util::Point hi(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
                 -std::numeric_limits<double>::max());
  for (const auto &v : d_vertices) {
    lo.d_x = std::min(lo.d_x, v.d_x);
    lo.d_y = std::min(lo.d_y, v.d_y);
    lo.d_z = std::min(lo.d_z, v.d_z);
    hi.d_x = std::max(hi.d_x, v.d_x);
    hi.d_y = std::max(hi.d_y, v.d_y);
    hi.d_z = std::max(hi.d_z, v.d_z);
  }
  return {lo - tol, hi + tol};
}

double OpenRectChannel2D::inscribedRadius() const {
  return 0.5 * d_t;
}

double OpenRectChannel2D::boundingRadius() const {
  const util::Point c = center();
  double r = 0.;
  for (const auto &v : d_vertices)
    r = std::max(r, (v - c).length());
  return r;
}

bool OpenRectChannel2D::isInside(const util::Point &x) const {
  if (std::abs(x.d_z - d_z) > 1.0e-9)
    return false;
  return pointInPolygon2D(x, d_vertices);
}

bool OpenRectChannel2D::isOutside(const util::Point &x) const {
  return !isInside(x);
}

bool OpenRectChannel2D::isNear(const util::Point &x, const double &tol) const {
  auto bbox = box(tol);
  return geom::isPointInsideBox(x, 2, bbox);
}

bool OpenRectChannel2D::isNearBoundary(const util::Point &x, const double &tol,
                                     const bool &within) const {
  if (!isNear(x, within ? 0. : tol))
    return false;
  const size_t n = d_vertices.size();
  for (size_t i = 0; i < n; ++i) {
    const util::Point &a = d_vertices[i];
    const util::Point &b = d_vertices[(i + 1) % n];
    if (util::isLess(distPointToSegment2D(x, a, b), tol))
      return true;
  }
  return false;
}

bool OpenRectChannel2D::doesIntersect(const util::Point &x) const {
  return isNearBoundary(x, 1.0e-8, false);
}

bool OpenRectChannel2D::isInside(const std::pair<util::Point, util::Point> &bx) const {
  for (auto p : geom::getCornerPoints(2, bx))
    if (!this->isInside(p))
      return false;
  return true;
}

bool OpenRectChannel2D::isOutside(const std::pair<util::Point, util::Point> &bx) const {
  bool intersect = false;
  for (auto p : geom::getCornerPoints(2, bx))
    if (!intersect)
      intersect = this->isInside(p);
  return !intersect;
}

bool OpenRectChannel2D::isNear(const std::pair<util::Point, util::Point> &bx,
                               const double &tol) const {
  return geom::areBoxesNear(this->box(), bx, tol, 2);
}

bool OpenRectChannel2D::doesIntersect(const std::pair<util::Point, util::Point> &bx) const {
  for (auto p : geom::getCornerPoints(2, bx))
    if (this->isInside(p))
      return true;
  return false;
}

std::string OpenRectChannel2D::printStr(int nt, int lvl) const {
  auto tabS = util::io::getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "------- OpenRectChannel2D (U-channel, open +y) --------" << std::endl;
  oss << tabS << "Outer box: [" << d_x0 << "," << d_y0 << "] — [" << d_x1 << "," << d_y1 << "], z="
      << d_z << ", t=" << d_t << std::endl;
  if (lvl > 0)
    oss << tabS << "Vertices = " << util::io::printStr(d_vertices, 0) << std::endl;
  return oss.str();
}

} // namespace geom
