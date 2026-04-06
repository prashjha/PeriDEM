/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#include "openCuboidChannel3D.h"
#include "geomUtilFunctions.h"
#include "util/io.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace geom {

namespace {

void validateParams(double x0, double y0, double z0, double x1, double y1, double z1, double t,
                    int open_face) {
  constexpr double eps = 1.0e-12;
  if (!(x1 > x0 + eps && y1 > y0 + eps && z1 > z0 + eps && t > eps))
    throw std::runtime_error(
        "OpenCuboidChannel3D: require x0<x1, y0<y1, z0<z1, t>0.");
  if (!(x1 - x0 > 2. * t + eps && y1 - y0 > 2. * t + eps && z1 - z0 > 2. * t + eps))
    throw std::runtime_error(
        "OpenCuboidChannel3D: each outer span must exceed 2*t so the inner cavity exists.");
  if (open_face < 0 || open_face > 5)
    throw std::runtime_error("OpenCuboidChannel3D: open_face must be in 0..5 (±x,±y,±z).");
}

void cornersFromAabb(const util::Point &lo, const util::Point &hi, util::Point *out8) {
  const double x0 = lo.d_x, y0 = lo.d_y, z0 = lo.d_z;
  const double x1 = hi.d_x, y1 = hi.d_y, z1 = hi.d_z;
  out8[0] = {x0, y0, z0};
  out8[1] = {x1, y0, z0};
  out8[2] = {x1, y1, z0};
  out8[3] = {x0, y1, z0};
  out8[4] = {x0, y0, z1};
  out8[5] = {x1, y0, z1};
  out8[6] = {x1, y1, z1};
  out8[7] = {x0, y1, z1};
}

void aabbFromCorners(const util::Point *c, util::Point &lo, util::Point &hi) {
  lo = hi = c[0];
  for (int i = 1; i < 8; ++i) {
    lo.d_x = std::min(lo.d_x, c[i].d_x);
    lo.d_y = std::min(lo.d_y, c[i].d_y);
    lo.d_z = std::min(lo.d_z, c[i].d_z);
    hi.d_x = std::max(hi.d_x, c[i].d_x);
    hi.d_y = std::max(hi.d_y, c[i].d_y);
    hi.d_z = std::max(hi.d_z, c[i].d_z);
  }
}

} // namespace

OpenCuboidChannel3D::OpenCuboidChannel3D()
  : GeomObject("open_cuboid_channel_3d", "") {}

OpenCuboidChannel3D::OpenCuboidChannel3D(double x0, double y0, double z0, double x1, double y1, double z1,
                                   double t, int open_face, std::string description)
  : GeomObject("open_cuboid_channel_3d", std::move(description)),
    d_lo(x0, y0, z0),
    d_hi(x1, y1, z1),
    d_t(t),
    d_openFace(open_face) {
  validateParams(x0, y0, z0, x1, y1, z1, t, open_face);
  validateAndRefreshCenter();
}

OpenCuboidChannel3D::OpenCuboidChannel3D(const OpenCuboidChannel3D &other)
  : GeomObject(other.d_name, other.d_description),
    d_lo(other.d_lo),
    d_hi(other.d_hi),
    d_t(other.d_t),
    d_openFace(other.d_openFace),
    d_x(other.d_x) {
  d_tags = other.d_tags;
}

OpenCuboidChannel3D &OpenCuboidChannel3D::operator=(const OpenCuboidChannel3D &other) {
  if (this != &other) {
    d_tags = other.d_tags;
    d_lo = other.d_lo;
    d_hi = other.d_hi;
    d_t = other.d_t;
    d_openFace = other.d_openFace;
    d_x = other.d_x;
  }
  return *this;
}

void OpenCuboidChannel3D::validateAndRefreshCenter() { d_x = computeCenter(); }

bool OpenCuboidChannel3D::inClosedShell(const util::Point &p) const {
  const auto &L = d_lo;
  const auto &H = d_hi;
  const double t = d_t;
  const bool in_outer =
      p.d_x >= L.d_x && p.d_x <= H.d_x && p.d_y >= L.d_y && p.d_y <= H.d_y && p.d_z >= L.d_z &&
      p.d_z <= H.d_z;
  const double xi0 = L.d_x + t, xi1 = H.d_x - t;
  const double yi0 = L.d_y + t, yi1 = H.d_y - t;
  const double zi0 = L.d_z + t, zi1 = H.d_z - t;
  const bool in_inner =
      p.d_x >= xi0 && p.d_x <= xi1 && p.d_y >= yi0 && p.d_y <= yi1 && p.d_z >= zi0 && p.d_z <= zi1;
  return in_outer && !in_inner;
}

bool OpenCuboidChannel3D::inRemovedFaceSlab(const util::Point &p) const {
  if (!inClosedShell(p))
    return false;
  const auto &L = d_lo;
  const auto &H = d_hi;
  const double t = d_t;
  constexpr double eps = 1.0e-12;
  const double x0 = L.d_x, y0 = L.d_y, z0 = L.d_z;
  const double x1 = H.d_x, y1 = H.d_y, z1 = H.d_z;

  switch (d_openFace) {
  case 0: { // +x: slab x in [x1-t, x1], (y,z) annulus on +x face
    if (p.d_x < x1 - t - eps || p.d_x > x1 + eps)
      return false;
    const bool oyz = p.d_y >= y0 && p.d_y <= y1 && p.d_z >= z0 && p.d_z <= z1;
    const bool iyz =
        p.d_y >= y0 + t && p.d_y <= y1 - t && p.d_z >= z0 + t && p.d_z <= z1 - t;
    return oyz && !iyz;
  }
  case 1: { // -x
    if (p.d_x < x0 - eps || p.d_x > x0 + t + eps)
      return false;
    const bool oyz = p.d_y >= y0 && p.d_y <= y1 && p.d_z >= z0 && p.d_z <= z1;
    const bool iyz =
        p.d_y >= y0 + t && p.d_y <= y1 - t && p.d_z >= z0 + t && p.d_z <= z1 - t;
    return oyz && !iyz;
  }
  case 2: { // +y
    if (p.d_y < y1 - t - eps || p.d_y > y1 + eps)
      return false;
    const bool oxz = p.d_x >= x0 && p.d_x <= x1 && p.d_z >= z0 && p.d_z <= z1;
    const bool ixz =
        p.d_x >= x0 + t && p.d_x <= x1 - t && p.d_z >= z0 + t && p.d_z <= z1 - t;
    return oxz && !ixz;
  }
  case 3: { // -y
    if (p.d_y < y0 - eps || p.d_y > y0 + t + eps)
      return false;
    const bool oxz = p.d_x >= x0 && p.d_x <= x1 && p.d_z >= z0 && p.d_z <= z1;
    const bool ixz =
        p.d_x >= x0 + t && p.d_x <= x1 - t && p.d_z >= z0 + t && p.d_z <= z1 - t;
    return oxz && !ixz;
  }
  case 4: { // +z: roof slab
    if (p.d_z < z1 - t - eps || p.d_z > z1 + eps)
      return false;
    const bool oxy = p.d_x >= x0 && p.d_x <= x1 && p.d_y >= y0 && p.d_y <= y1;
    const bool ixy =
        p.d_x >= x0 + t && p.d_x <= x1 - t && p.d_y >= y0 + t && p.d_y <= y1 - t;
    return oxy && !ixy;
  }
  case 5: { // -z
    if (p.d_z < z0 - eps || p.d_z > z0 + t + eps)
      return false;
    const bool oxy = p.d_x >= x0 && p.d_x <= x1 && p.d_y >= y0 && p.d_y <= y1;
    const bool ixy =
        p.d_x >= x0 + t && p.d_x <= x1 - t && p.d_y >= y0 + t && p.d_y <= y1 - t;
    return oxy && !ixy;
  }
  default:
    return false;
  }
}

util::Point OpenCuboidChannel3D::computeCenter() const {
  const double Lx = d_hi.d_x - d_lo.d_x;
  const double Ly = d_hi.d_y - d_lo.d_y;
  const double Lz = d_hi.d_z - d_lo.d_z;
  const double t = d_t;
  const double Vout = Lx * Ly * Lz;
  const double Vin = std::max(0., (Lx - 2. * t) * (Ly - 2. * t) * (Lz - 2. * t));
  const double Vclosed = Vout - Vin;
  const double cx = 0.5 * (d_lo.d_x + d_hi.d_x);
  const double cy = 0.5 * (d_lo.d_y + d_hi.d_y);
  const double cz = 0.5 * (d_lo.d_z + d_hi.d_z);
  util::Point c_closed(cx, cy, cz);
  if (Vclosed <= 1.0e-30)
    return c_closed;

  if (d_openFace < 0 || d_openFace > 5)
    return c_closed;

  double Aface = 0.;
  util::Point c_roof(cx, cy, cz);
  if (d_openFace == 0 || d_openFace == 1) {
    Aface = Ly * Lz - std::max(0., (Ly - 2. * t) * (Lz - 2. * t));
    c_roof.d_x = (d_openFace == 0) ? d_hi.d_x - 0.5 * t : d_lo.d_x + 0.5 * t;
    c_roof.d_y = cy;
    c_roof.d_z = cz;
  } else if (d_openFace == 2 || d_openFace == 3) {
    Aface = Lx * Lz - std::max(0., (Lx - 2. * t) * (Lz - 2. * t));
    c_roof.d_y = (d_openFace == 2) ? d_hi.d_y - 0.5 * t : d_lo.d_y + 0.5 * t;
    c_roof.d_x = cx;
    c_roof.d_z = cz;
  } else {
    Aface = Lx * Ly - std::max(0., (Lx - 2. * t) * (Ly - 2. * t));
    c_roof.d_z = (d_openFace == 4) ? d_hi.d_z - 0.5 * t : d_lo.d_z + 0.5 * t;
    c_roof.d_x = cx;
    c_roof.d_y = cy;
  }
  const double Vroof = Aface * t;
  const double Vopen = Vclosed - Vroof;
  if (Vopen <= 1.0e-30)
    return c_closed;
  return (1. / Vopen) * (Vclosed * c_closed - Vroof * c_roof);
}

void OpenCuboidChannel3D::transform(const util::Point &translation, const double &scale,
                                  const double &angle, const util::Point &axis,
                                  const util::Point *rotationPoint) {
  const util::Point c0 = d_x;
  d_t *= scale;
  util::Point c[8];
  cornersFromAabb(d_lo, d_hi, c);
  for (int i = 0; i < 8; ++i)
    c[i] = mapSimilarity(c[i], c0, translation, scale, angle, axis, rotationPoint);
  aabbFromCorners(c, d_lo, d_hi);
  validateParams(d_lo.d_x, d_lo.d_y, d_lo.d_z, d_hi.d_x, d_hi.d_y, d_hi.d_z, d_t, d_openFace);
  d_x = computeCenter();
}

double OpenCuboidChannel3D::volume() const {
  const double Lx = d_hi.d_x - d_lo.d_x;
  const double Ly = d_hi.d_y - d_lo.d_y;
  const double Lz = d_hi.d_z - d_lo.d_z;
  const double t = d_t;
  const double Vout = Lx * Ly * Lz;
  const double Vin = std::max(0., (Lx - 2. * t) * (Ly - 2. * t) * (Lz - 2. * t));
  const double Vclosed = Vout - Vin;
  double Aface = 0.;
  if (d_openFace == 0 || d_openFace == 1)
    Aface = Ly * Lz - std::max(0., (Ly - 2. * t) * (Lz - 2. * t));
  else if (d_openFace == 2 || d_openFace == 3)
    Aface = Lx * Lz - std::max(0., (Lx - 2. * t) * (Lz - 2. * t));
  else
    Aface = Lx * Ly - std::max(0., (Lx - 2. * t) * (Ly - 2. * t));
  const double Vroof = Aface * t;
  return std::max(0., Vclosed - Vroof);
}

util::Point OpenCuboidChannel3D::center() const { return d_x; }

std::pair<util::Point, util::Point> OpenCuboidChannel3D::box() const { return box(0.); }

std::pair<util::Point, util::Point> OpenCuboidChannel3D::box(const double &tol) const {
  return {d_lo - tol, d_hi + tol};
}

double OpenCuboidChannel3D::inscribedRadius() const { return 0.5 * d_t; }

double OpenCuboidChannel3D::boundingRadius() const {
  const util::Point &c = d_x;
  util::Point c8[8];
  cornersFromAabb(d_lo, d_hi, c8);
  double r = 0.;
  for (int i = 0; i < 8; ++i)
    r = std::max(r, (c8[i] - c).length());
  return r;
}

bool OpenCuboidChannel3D::isInside(const util::Point &x) const {
  return inClosedShell(x) && !inRemovedFaceSlab(x);
}

bool OpenCuboidChannel3D::isOutside(const util::Point &x) const { return !isInside(x); }

bool OpenCuboidChannel3D::isNear(const util::Point &x, const double &tol) const {
  return geom::isPointInsideBox(x, 3, box(tol));
}

bool OpenCuboidChannel3D::isNearBoundary(const util::Point &x, const double &tol,
                                       const bool &within) const {
  if (!isNear(x, within ? 0. : tol))
    return false;
  return isInside(x) && (isOutside(x + util::Point(tol, 0., 0.)) ||
                         isOutside(x - util::Point(tol, 0., 0.)) ||
                         isOutside(x + util::Point(0., tol, 0.)) ||
                         isOutside(x - util::Point(0., tol, 0.)) ||
                         isOutside(x + util::Point(0., 0., tol)) ||
                         isOutside(x - util::Point(0., 0., tol)));
}

bool OpenCuboidChannel3D::doesIntersect(const util::Point &x) const {
  return isNearBoundary(x, 1.0e-8, false);
}

bool OpenCuboidChannel3D::isInside(const std::pair<util::Point, util::Point> &bx) const {
  for (auto p : geom::getCornerPoints(3, bx))
    if (!this->isInside(p))
      return false;
  return true;
}

bool OpenCuboidChannel3D::isOutside(const std::pair<util::Point, util::Point> &bx) const {
  bool intersect = false;
  for (auto p : geom::getCornerPoints(3, bx))
    if (!intersect)
      intersect = this->isInside(p);
  return !intersect;
}

bool OpenCuboidChannel3D::isNear(const std::pair<util::Point, util::Point> &bx,
                               const double &tol) const {
  return geom::areBoxesNear(this->box(), bx, tol, 3);
}

bool OpenCuboidChannel3D::doesIntersect(const std::pair<util::Point, util::Point> &bx) const {
  for (auto p : geom::getCornerPoints(3, bx))
    if (this->isInside(p))
      return true;
  return false;
}

std::string OpenCuboidChannel3D::printStr(int nt, int lvl) const {
  auto tabS = util::io::getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "------- OpenCuboidChannel3D (open shell) --------" << std::endl;
  oss << tabS << "Outer AABB: [" << d_lo.d_x << "," << d_lo.d_y << "," << d_lo.d_z << "] — ["
      << d_hi.d_x << "," << d_hi.d_y << "," << d_hi.d_z << "], t=" << d_t
      << ", open_face=" << d_openFace << std::endl;
  oss << tabS << "Center (d_x) = " << d_x.printStr() << std::endl;
  if (lvl > 0)
    oss << tabS << "volume = " << volume() << std::endl;
  return oss.str();
}

} // namespace geom
