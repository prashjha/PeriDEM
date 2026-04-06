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
#include "annulusMesh2D.h"
#include "annulusMesh3D.h"
#include "openBoundaryWalls2D.h"
#include "primitiveOccMesh.h"
#include "geom/complexGeomObjects.h"
#include "geom/geomObjects.h"
#include "geom/openRectChannel2D.h"
#include "geom/openCuboidChannel3D.h"
#include "openBoundaryWalls3D.h"
#include "util/point.h"
#include <cmath>
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

void buildOpenRectChannel2DGeo(const geom::OpenRectChannel2D &g, double h) {
  meshPolygon2DGeoFromVertices(g.d_vertices, h);
  std::vector<std::pair<int, int>> ents;
  gmsh::model::getEntities(ents, 2);
  if (!ents.empty()) {
    const double tol = std::max(1.0e-9, 1.0e-6 * std::max(g.d_y1 - g.d_y0, g.d_x1 - g.d_x0));
    physicalGroupsWallOpenFromY2D(ents.back().second, g.d_y1, tol);
  }
}

static int firstVolumeTagFromCut(const std::vector<std::pair<int, int>> &ov) {
  for (const auto &pr : ov)
    if (pr.first == 3)
      return pr.second;
  return -1;
}

/**
 * Point inside solid wall for mesh::embed (like embedPointInAnnulus3D). The AABB center lies in the
 * cavity, so g.center() must not be used.
 */
static util::Point embedPointOpenCuboidChannel3DForMesh(const geom::OpenCuboidChannel3D &g) {
  const double x0 = g.d_lo.d_x, y0 = g.d_lo.d_y, z0 = g.d_lo.d_z;
  const double x1 = g.d_hi.d_x, y1 = g.d_hi.d_y, z1 = g.d_hi.d_z;
  const double t = g.d_t;
  const double hx = 0.5 * t, hy = 0.5 * t, hz = 0.5 * t;
  switch (g.d_openFace) {
  case 0:
    return {x0 + hx, y0 + hy, z0 + hz};
  case 1:
    return {x1 - hx, y0 + hy, z0 + hz};
  case 2:
    return {x0 + hx, y0 + hy, z0 + hz};
  case 3:
    return {x0 + hx, y1 - hy, z0 + hz};
  case 4:
    return {x0 + hx, y0 + hy, z0 + hz};
  case 5:
    return {x0 + hx, y0 + hy, z1 - hz};
  default:
    throw std::runtime_error("embedPointOpenCuboidChannel3DForMesh: open_face must be 0..5.");
  }
}

/** Cuboidal shell with one outer face slab removed; OCC cut outer−inner, then cut shell−slab. */
void buildOpenCuboidChannel3DGeo(const geom::OpenCuboidChannel3D &g, double h) {
  const double x0 = g.d_lo.d_x, y0 = g.d_lo.d_y, z0 = g.d_lo.d_z;
  const double x1 = g.d_hi.d_x, y1 = g.d_hi.d_y, z1 = g.d_hi.d_z;
  const double Lx = x1 - x0, Ly = y1 - y0, Lz = z1 - z0;
  const double t = g.d_t;
  const int face = g.d_openFace;

  const int out_vol = gmsh::model::occ::addBox(x0, y0, z0, Lx, Ly, Lz);
  gmsh::model::occ::synchronize();
  const int in_vol = gmsh::model::occ::addBox(x0 + t, y0 + t, z0 + t, Lx - 2. * t, Ly - 2. * t,
                                               Lz - 2. * t);
  gmsh::model::occ::synchronize();
  std::vector<std::pair<int, int>> ov;
  std::vector<std::vector<std::pair<int, int>>> ovv;
  gmsh::model::occ::cut({{3, out_vol}}, {{3, in_vol}}, ov, ovv, -1, true, true);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::removeAllDuplicates();
  gmsh::model::occ::synchronize();

  int shell = firstVolumeTagFromCut(ov);
  if (shell < 0) {
    std::vector<std::pair<int, int>> ents;
    gmsh::model::getEntities(ents, 3);
    if (!ents.empty())
      shell = ents.back().second;
  }
  if (shell < 0)
    throw std::runtime_error("buildOpenCuboidChannel3DGeo: no volume after outer−inner cut.");

  int slab = -1;
  switch (face) {
  case 0:
    slab = gmsh::model::occ::addBox(x1 - t, y0, z0, t, Ly, Lz);
    break;
  case 1:
    slab = gmsh::model::occ::addBox(x0, y0, z0, t, Ly, Lz);
    break;
  case 2:
    slab = gmsh::model::occ::addBox(x0, y1 - t, z0, Lx, t, Lz);
    break;
  case 3:
    slab = gmsh::model::occ::addBox(x0, y0, z0, Lx, t, Lz);
    break;
  case 4:
    slab = gmsh::model::occ::addBox(x0, y0, z1 - t, Lx, Ly, t);
    break;
  case 5:
    slab = gmsh::model::occ::addBox(x0, y0, z0, Lx, Ly, t);
    break;
  default:
    throw std::runtime_error("buildOpenCuboidChannel3DGeo: open_face must be 0..5.");
  }
  gmsh::model::occ::synchronize();

  std::vector<std::pair<int, int>> ov2;
  gmsh::model::occ::cut({{3, shell}}, {{3, slab}}, ov2, ovv, -1, true, true);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::removeAllDuplicates();
  gmsh::model::occ::synchronize();

  int vol = firstVolumeTagFromCut(ov2);
  if (vol < 0) {
    std::vector<std::pair<int, int>> ents;
    gmsh::model::getEntities(ents, 3);
    if (!ents.empty())
      vol = ents.back().second;
  }
  if (vol < 0)
    throw std::runtime_error("buildOpenCuboidChannel3DGeo: no volume after removing opening slab.");

  const util::Point c = embedPointOpenCuboidChannel3DForMesh(g);
  const int p = gmsh::model::occ::addPoint(c.d_x, c.d_y, c.d_z, h);
  gmsh::model::occ::synchronize();
  gmsh::model::mesh::embed(0, {p}, 3, vol);
  gmsh::model::occ::synchronize();

  const double tol = std::max(1.0e-9, 1.0e-6 * std::max({Lx, Ly, Lz}));
  physicalGroupsWallOpenFromFace3D(vol, face, g.d_lo, g.d_hi, g.d_t, tol);
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

  if (n == "annulus_object") {
    const auto &ag = static_cast<const geom::AnnulusGeomObject &>(g);
    if (ag.d_dim == 2)
      return buildAnnulus2DInCurrentModel(ag, h);
    if (ag.d_dim == 3)
      return buildAnnulus3DInCurrentModel(ag, h);
    throw std::runtime_error("buildGmshGeometryInCurrentModel: annulus_object has invalid d_dim.");
  }

  if (n == "circle")
    return buildCircleOcc(static_cast<const geom::Circle &>(g), h);
  if (n == "ellipse")
    return buildEllipseOcc(static_cast<const geom::Ellipse &>(g), h);
  if (n == "sphere")
    return buildSphereOcc(static_cast<const geom::Sphere &>(g));
  if (n == "ellipsoid")
    return buildEllipsoidOcc(static_cast<const geom::Ellipsoid &>(g));
  if (n == "cube")
    return buildOccBoxFromAabb(g);
  if (n == "cuboid")
    return buildOccBoxFromAabb(g);
  if (n == "cylinder")
    return buildCylinderOcc(static_cast<const geom::Cylinder &>(g));

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
  if (n == "open_rect_channel_2d")
    return buildOpenRectChannel2DGeo(static_cast<const geom::OpenRectChannel2D &>(g), h);
  if (n == "open_cuboid_channel_3d")
    return buildOpenCuboidChannel3DGeo(static_cast<const geom::OpenCuboidChannel3D &>(g), h);

  throw std::runtime_error("buildGmshGeometryInCurrentModel: no Gmsh recipe for geometry \"" + n +
                           "\".");
}

} // namespace mesh_gen
