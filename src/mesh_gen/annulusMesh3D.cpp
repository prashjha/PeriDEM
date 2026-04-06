/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#include "annulusMesh3D.h"
#include "geom/complexGeomObjects.h"
#include "geom/geomObjects.h"
#include <cmath>
#include <gmsh.h>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mesh_gen {
namespace {

void assertCuboidAnnulusValid(const geom::Cuboid &outer, const geom::Cuboid &inner) {
  const auto &blo = outer.d_vertices[0];
  const auto &bhi = outer.d_vertices[6];
  const auto &bli = inner.d_vertices[0];
  const auto &bhi_i = inner.d_vertices[6];

  constexpr double eps = 1.0e-12;
  if (!(bli.d_x > blo.d_x + eps && bhi_i.d_x < bhi.d_x - eps && bli.d_y > blo.d_y + eps &&
        bhi_i.d_y < bhi.d_y - eps && bli.d_z > blo.d_z + eps && bhi_i.d_z < bhi.d_z - eps))
    throw std::runtime_error(
        "buildAnnulus3DInCurrentModel: inner cuboid must be strictly inside the outer cuboid.");
}

void assertSphereAnnulusValid(const geom::Sphere &outer, const geom::Sphere &inner) {
  constexpr double eps = 1.0e-10;
  if ((outer.d_x - inner.d_x).lengthSq() > eps)
    throw std::runtime_error(
        "buildAnnulus3DInCurrentModel: sphere annulus requires coincident centers (concentric spheres).");
  if (inner.d_r <= 0. || outer.d_r <= inner.d_r)
    throw std::runtime_error(
        "buildAnnulus3DInCurrentModel: require 0 < r_inner < r_outer for sphere − sphere.");
}

static int firstTagOfDim(const std::vector<std::pair<int, int>> &ov, int dim) {
  for (const auto &pr : ov)
    if (pr.first == dim)
      return pr.second;
  return -1;
}

/** See embedPointInAnnulus2D: centroid of shell can lie in the void; embed in solid material. */
util::Point embedPointInAnnulus3D(const geom::AnnulusGeomObject &a) {
  if (a.d_inObj_p->d_name == "sphere" && a.d_outObj_p->d_name == "sphere") {
    const auto &outer = *static_cast<const geom::Sphere *>(a.d_outObj_p);
    const auto &inner = *static_cast<const geom::Sphere *>(a.d_inObj_p);
    const double r_mid = 0.5 * (outer.d_r + inner.d_r);
    return {outer.d_x.d_x + r_mid, outer.d_x.d_y, outer.d_x.d_z};
  }
  if (a.d_inObj_p->d_name == "cuboid" && a.d_outObj_p->d_name == "cuboid") {
    const auto &outer = *static_cast<const geom::Cuboid *>(a.d_outObj_p);
    const auto &inner = *static_cast<const geom::Cuboid *>(a.d_inObj_p);
    const auto &blo = outer.d_vertices[0];
    const auto &bli = inner.d_vertices[0];
    return {0.5 * (blo.d_x + bli.d_x), 0.5 * (blo.d_y + bli.d_y), 0.5 * (blo.d_z + bli.d_z)};
  }
  return a.center();
}

void finish3DCutEmbedVolume(const std::vector<std::pair<int, int>> &ov, const geom::AnnulusGeomObject &a,
                            double h) {

  int vol_tag = firstTagOfDim(ov, 3);
  if (vol_tag < 0) {
    std::vector<std::pair<int, int>> ents;
    gmsh::model::getEntities(ents, 3);
    if (!ents.empty())
      vol_tag = ents.back().second;
  }
  if (vol_tag < 0) {
    std::ostringstream oss;
    oss << "buildAnnulus3DInCurrentModel: boolean cut did not produce a volume (ov size = "
        << ov.size() << ").";
    throw std::runtime_error(oss.str());
  }

  const util::Point c = embedPointInAnnulus3D(a);
  const int p = gmsh::model::occ::addPoint(c.d_x, c.d_y, c.d_z, h);
  gmsh::model::occ::synchronize();
  gmsh::model::mesh::embed(0, {p}, 3, vol_tag);
  gmsh::model::occ::synchronize();
}

void buildCuboidAnnulus3D(const geom::AnnulusGeomObject &a, double h) {
  const auto &outer = *static_cast<const geom::Cuboid *>(a.d_outObj_p);
  const auto &inner = *static_cast<const geom::Cuboid *>(a.d_inObj_p);
  assertCuboidAnnulusValid(outer, inner);

  const auto &lo = outer.d_vertices[0];
  const int out_vol =
      gmsh::model::occ::addBox(lo.d_x, lo.d_y, lo.d_z, outer.d_Lx, outer.d_Ly, outer.d_Lz);
  gmsh::model::occ::synchronize();

  const auto &li = inner.d_vertices[0];
  const int in_vol =
      gmsh::model::occ::addBox(li.d_x, li.d_y, li.d_z, inner.d_Lx, inner.d_Ly, inner.d_Lz);
  gmsh::model::occ::synchronize();

  std::vector<std::pair<int, int>> ov;
  std::vector<std::vector<std::pair<int, int>>> ovv;
  gmsh::model::occ::cut({{3, out_vol}}, {{3, in_vol}}, ov, ovv, -1, true, true);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::removeAllDuplicates();
  gmsh::model::occ::synchronize();

  finish3DCutEmbedVolume(ov, a, h);
}

void buildSphereAnnulus3D(const geom::AnnulusGeomObject &a, double h) {
  const auto &outer = *static_cast<const geom::Sphere *>(a.d_outObj_p);
  const auto &inner = *static_cast<const geom::Sphere *>(a.d_inObj_p);
  assertSphereAnnulusValid(outer, inner);

  const int out_vol =
      gmsh::model::occ::addSphere(outer.d_x.d_x, outer.d_x.d_y, outer.d_x.d_z, outer.d_r);
  gmsh::model::occ::synchronize();
  const int in_vol =
      gmsh::model::occ::addSphere(inner.d_x.d_x, inner.d_x.d_y, inner.d_x.d_z, inner.d_r);
  gmsh::model::occ::synchronize();

  std::vector<std::pair<int, int>> ov;
  std::vector<std::vector<std::pair<int, int>>> ovv;
  gmsh::model::occ::cut({{3, out_vol}}, {{3, in_vol}}, ov, ovv, -1, true, true);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::removeAllDuplicates();
  gmsh::model::occ::synchronize();

  finish3DCutEmbedVolume(ov, a, h);
}

} // namespace

void buildAnnulus3DInCurrentModel(const geom::AnnulusGeomObject &a, double h) {

  if (a.d_dim != 3)
    throw std::runtime_error("buildAnnulus3DInCurrentModel: expected d_dim == 3.");
  if (!a.d_inObj_p || !a.d_outObj_p)
    throw std::runtime_error("buildAnnulus3DInCurrentModel: null inner or outer geometry.");

  if (a.d_inObj_p->d_name == "cuboid" && a.d_outObj_p->d_name == "cuboid") {
    buildCuboidAnnulus3D(a, h);
    return;
  }
  if (a.d_inObj_p->d_name == "sphere" && a.d_outObj_p->d_name == "sphere") {
    buildSphereAnnulus3D(a, h);
    return;
  }

  throw std::runtime_error(
      "buildAnnulus3DInCurrentModel: unsupported 3D pair (supported: cuboid−cuboid, sphere−sphere).");
}

} // namespace mesh_gen
