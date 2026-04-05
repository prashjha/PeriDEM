/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "builtinGmshGeometry.h"
#include "geom/geomObjects.h"
#include "util/point.h"
#include <gmsh.h>
#include <stdexcept>
#include <vector>

namespace mesh_gen {
namespace {

void meshPolygon2DGeoFromVertices(const std::vector<util::Point> &verts, double h) {
  if (verts.size() < 3)
    throw std::runtime_error("meshPolygon2DGeoFromVertices: need at least 3 vertices.");

  std::vector<int> pids;
  pids.reserve(verts.size());
  for (const auto &v : verts)
    pids.push_back(gmsh::model::geo::addPoint(v.d_x, v.d_y, v.d_z, h));

  std::vector<int> lines;
  const size_t n = verts.size();
  lines.reserve(n);
  for (size_t i = 0; i < n; ++i)
    lines.push_back(gmsh::model::geo::addLine(pids[i], pids[(i + 1) % n]));

  int cl = gmsh::model::geo::addCurveLoop(lines);
  gmsh::model::geo::addPlaneSurface({cl});
  gmsh::model::geo::synchronize();
}

/** Closed boundary vertex order for Drum2D mesh (matches prior param-based path). */
std::vector<util::Point> drum2dPolygonBoundary(const geom::Drum2D &d) {
  static const int kOrder[] = {3, 2, 1, 0, 5, 4};
  std::vector<util::Point> poly;
  poly.reserve(6);
  for (int k : kOrder)
    poly.push_back(d.d_vertices[static_cast<size_t>(k)]);
  return poly;
}

void buildCircleOcc(const geom::Circle &c, double h) {
  const int circ =
      gmsh::model::occ::addCircle(c.d_x.d_x, c.d_x.d_y, c.d_x.d_z, c.d_r);
  const int cl = gmsh::model::occ::addCurveLoop({circ});
  const int s = gmsh::model::occ::addPlaneSurface({cl});
  const int p = gmsh::model::occ::addPoint(c.d_x.d_x, c.d_x.d_y, c.d_x.d_z, h);
  gmsh::model::occ::synchronize();
  gmsh::model::mesh::embed(0, {p}, 2, s);
}

void buildSphereOcc(const geom::Sphere &s) {
  gmsh::model::occ::addSphere(s.d_x.d_x, s.d_x.d_y, s.d_x.d_z, s.d_r);
  gmsh::model::occ::synchronize();
}

void buildOccBoxFromAabb(const geom::GeomObject &g) {
  auto bx = g.box();
  const auto &lo = bx.first;
  const auto &hi = bx.second;
  gmsh::model::occ::addBox(lo.d_x, lo.d_y, lo.d_z, hi.d_x - lo.d_x, hi.d_y - lo.d_y,
                           hi.d_z - lo.d_z);
  gmsh::model::occ::synchronize();
}

} // namespace

void buildGmshGeometryInCurrentModel(const geom::GeomObject &g, double h) {
  const std::string &n = g.d_name;

  if (n == "circle")
    return buildCircleOcc(static_cast<const geom::Circle &>(g), h);
  if (n == "sphere")
    return buildSphereOcc(static_cast<const geom::Sphere &>(g));
  if (n == "cube")
    return buildOccBoxFromAabb(g);
  if (n == "cuboid")
    return buildOccBoxFromAabb(g);

  if (n == "square")
    return meshPolygon2DGeoFromVertices(static_cast<const geom::Square &>(g).d_vertices, h);
  if (n == "rectangle")
    return meshPolygon2DGeoFromVertices(static_cast<const geom::Rectangle &>(g).d_vertices, h);
  if (n == "triangle")
    return meshPolygon2DGeoFromVertices(static_cast<const geom::Triangle &>(g).d_vertices, h);
  if (n == "hexagon")
    return meshPolygon2DGeoFromVertices(static_cast<const geom::Hexagon &>(g).d_vertices, h);
  if (n == "drum2d")
    return meshPolygon2DGeoFromVertices(drum2dPolygonBoundary(static_cast<const geom::Drum2D &>(g)),
                                        h);

  throw std::runtime_error("buildGmshGeometryInCurrentModel: no Gmsh recipe for geometry \"" + n +
                           "\".");
}

} // namespace mesh_gen
