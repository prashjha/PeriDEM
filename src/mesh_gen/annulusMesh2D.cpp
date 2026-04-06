/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#include "annulusMesh2D.h"
#include "geom/complexGeomObjects.h"
#include "geom/geomObjects.h"
#include <cmath>
#include <gmsh.h>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mesh_gen {
namespace {

void assertRectangleAnnulusValid(const geom::Rectangle &outer, const geom::Rectangle &inner) {
  const auto &blo = outer.d_vertices[0];
  const auto &bhi = outer.d_vertices[2];
  const auto &bli = inner.d_vertices[0];
  const auto &bhi_i = inner.d_vertices[2];

  constexpr double eps = 1.0e-12;
  if (std::abs(blo.d_z - bli.d_z) > 1.0e-9 || std::abs(bhi.d_z - bhi_i.d_z) > 1.0e-9)
    throw std::runtime_error("buildAnnulus2DInCurrentModel: inner and outer rectangles must lie in the same z plane.");

  if (!(bli.d_x > blo.d_x + eps && bhi_i.d_x < bhi.d_x - eps && bli.d_y > blo.d_y + eps &&
        bhi_i.d_y < bhi.d_y - eps))
    throw std::runtime_error(
        "buildAnnulus2DInCurrentModel: inner rectangle must be strictly inside the outer rectangle.");
}

void assertCircleAnnulusValid(const geom::Circle &outer, const geom::Circle &inner) {
  constexpr double eps = 1.0e-10;
  if ((outer.d_x - inner.d_x).lengthSq() > eps)
    throw std::runtime_error(
        "buildAnnulus2DInCurrentModel: circle annulus requires coincident centers (concentric circles).");
  if (inner.d_r <= 0. || outer.d_r <= inner.d_r)
    throw std::runtime_error(
        "buildAnnulus2DInCurrentModel: require 0 < r_inner < r_outer for circle − circle.");
}

static int firstTagOfDim(const std::vector<std::pair<int, int>> &ov, int dim) {
  for (const auto &pr : ov)
    if (pr.first == dim)
      return pr.second;
  return -1;
}

/**
 * Point strictly inside the annulus for mesh embedding. Do not use AnnulusGeomObject::center():
 * for symmetric annuli the composite centroid lies in the void (hole), so Gmsh would mesh a
 * node there with zero tributary volume.
 */
util::Point embedPointInAnnulus2D(const geom::AnnulusGeomObject &a) {
  if (a.d_inObj_p->d_name == "circle" && a.d_outObj_p->d_name == "circle") {
    const auto &outer = *static_cast<const geom::Circle *>(a.d_outObj_p);
    const auto &inner = *static_cast<const geom::Circle *>(a.d_inObj_p);
    const double r_mid = 0.5 * (outer.d_r + inner.d_r);
    return {outer.d_x.d_x + r_mid, outer.d_x.d_y, outer.d_x.d_z};
  }
  if (a.d_inObj_p->d_name == "rectangle" && a.d_outObj_p->d_name == "rectangle") {
    const auto &outer = *static_cast<const geom::Rectangle *>(a.d_outObj_p);
    const auto &inner = *static_cast<const geom::Rectangle *>(a.d_inObj_p);
    const auto &blo = outer.d_vertices[0];
    const auto &bli = inner.d_vertices[0];
    return {0.5 * (blo.d_x + bli.d_x), 0.5 * (blo.d_y + bli.d_y), blo.d_z};
  }
  return a.center();
}

void finish2DCutEmbedSurface(const std::vector<std::pair<int, int>> &ov, const geom::AnnulusGeomObject &a,
                             double h) {

  int surface_tag = firstTagOfDim(ov, 2);
  if (surface_tag < 0) {
    std::vector<std::pair<int, int>> ents;
    gmsh::model::getEntities(ents, 2);
    if (!ents.empty())
      surface_tag = ents.back().second;
  }
  if (surface_tag < 0) {
    std::ostringstream oss;
    oss << "buildAnnulus2DInCurrentModel: boolean cut did not produce a surface (ov size = "
        << ov.size() << ").";
    throw std::runtime_error(oss.str());
  }

  const util::Point c = embedPointInAnnulus2D(a);
  const int p = gmsh::model::occ::addPoint(c.d_x, c.d_y, c.d_z, h);
  gmsh::model::occ::synchronize();
  gmsh::model::mesh::embed(0, {p}, 2, surface_tag);
  gmsh::model::occ::synchronize();
}

void buildRectangleAnnulus2D(const geom::AnnulusGeomObject &a, double h) {
  const auto &outer = *static_cast<const geom::Rectangle *>(a.d_outObj_p);
  const auto &inner = *static_cast<const geom::Rectangle *>(a.d_inObj_p);
  assertRectangleAnnulusValid(outer, inner);

  const auto &lo = outer.d_vertices[0];
  const int out_surf = gmsh::model::occ::addRectangle(lo.d_x, lo.d_y, lo.d_z, outer.d_Lx, outer.d_Ly);
  gmsh::model::occ::synchronize();

  const auto &li = inner.d_vertices[0];
  const int in_surf =
      gmsh::model::occ::addRectangle(li.d_x, li.d_y, li.d_z, inner.d_Lx, inner.d_Ly);
  gmsh::model::occ::synchronize();

  std::vector<std::pair<int, int>> ov;
  std::vector<std::vector<std::pair<int, int>>> ovv;
  gmsh::model::occ::cut({{2, out_surf}}, {{2, in_surf}}, ov, ovv, -1, true, true);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::removeAllDuplicates();
  gmsh::model::occ::synchronize();

  finish2DCutEmbedSurface(ov, a, h);
}

void buildCircleAnnulus2D(const geom::AnnulusGeomObject &a, double h) {
  const auto &outer = *static_cast<const geom::Circle *>(a.d_outObj_p);
  const auto &inner = *static_cast<const geom::Circle *>(a.d_inObj_p);
  assertCircleAnnulusValid(outer, inner);

  const std::vector<double> zAxis = {0., 0., 1.};
  const std::vector<double> xAxis = {1., 0., 0.};

  const int out_surf = gmsh::model::occ::addDisk(outer.d_x.d_x, outer.d_x.d_y, outer.d_x.d_z,
                                                   outer.d_r, outer.d_r, -1, zAxis, xAxis);
  gmsh::model::occ::synchronize();
  const int in_surf = gmsh::model::occ::addDisk(inner.d_x.d_x, inner.d_x.d_y, inner.d_x.d_z,
                                                 inner.d_r, inner.d_r, -1, zAxis, xAxis);
  gmsh::model::occ::synchronize();

  std::vector<std::pair<int, int>> ov;
  std::vector<std::vector<std::pair<int, int>>> ovv;
  gmsh::model::occ::cut({{2, out_surf}}, {{2, in_surf}}, ov, ovv, -1, true, true);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::removeAllDuplicates();
  gmsh::model::occ::synchronize();

  finish2DCutEmbedSurface(ov, a, h);
}

} // namespace

void buildAnnulus2DInCurrentModel(const geom::AnnulusGeomObject &a, double h) {

  if (a.d_dim != 2)
    throw std::runtime_error("buildAnnulus2DInCurrentModel: expected d_dim == 2.");
  if (!a.d_inObj_p || !a.d_outObj_p)
    throw std::runtime_error("buildAnnulus2DInCurrentModel: null inner or outer geometry.");

  if (a.d_inObj_p->d_name == "rectangle" && a.d_outObj_p->d_name == "rectangle") {
    buildRectangleAnnulus2D(a, h);
    return;
  }
  if (a.d_inObj_p->d_name == "circle" && a.d_outObj_p->d_name == "circle") {
    buildCircleAnnulus2D(a, h);
    return;
  }

  throw std::runtime_error(
      "buildAnnulus2DInCurrentModel: unsupported 2D pair (supported: rectangle−rectangle, circle−circle).");
}

} // namespace mesh_gen
