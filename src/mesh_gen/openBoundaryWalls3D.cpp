/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#include "openBoundaryWalls3D.h"
#include <algorithm>
#include <cmath>
#include <gmsh.h>
#include <vector>

namespace mesh_gen {

namespace {

/** Smallest edge length of the surface bounding box (planar-ish if small vs max edge). */
double minEdge2D(double dx, double dy, double dz) {
  const double a = std::min({dx, dy, dz});
  return a;
}

double maxEdge2D(double dx, double dy, double dz) {
  return std::max({dx, dy, dz});
}

} // namespace

void physicalGroupsWallOpenFromFace3D(int volumeTag, int openFace, const util::Point &lo,
                                      const util::Point &hi, double t, double tol,
                                      const std::string &physWall, const std::string &physOpen) {

  const double x0 = lo.d_x, y0 = lo.d_y, z0 = lo.d_z;
  const double x1 = hi.d_x, y1 = hi.d_y, z1 = hi.d_z;
  const double Lx = x1 - x0;
  const double Ly = y1 - y0;
  const double Lz = z1 - z0;
  const double horizTol = std::max(tol, 1.0e-9 * std::max({Lx, Ly, Lz, 1.0}));

  std::vector<std::pair<int, int>> bnd;
  // Request unoriented boundary entities so tags are always valid entity IDs.
  gmsh::model::getBoundary({{3, volumeTag}}, bnd, false, false);

  std::vector<int> wallTags;
  std::vector<int> openTags;
  wallTags.reserve(bnd.size());
  openTags.reserve(bnd.size());

  for (const auto &pr : bnd) {
    if (pr.first != 2)
      continue;
    const int surfTag = std::abs(pr.second);
    double xmin = 0., ymin = 0., zmin = 0., xmax = 0., ymax = 0., zmax = 0.;
    gmsh::model::getBoundingBox(pr.first, surfTag, xmin, ymin, zmin, xmax, ymax, zmax);
    const double dx = xmax - xmin;
    const double dy = ymax - ymin;
    const double dz = zmax - zmin;
    const double cx = 0.5 * (xmin + xmax);
    const double cy = 0.5 * (ymin + ymax);
    const double cz = 0.5 * (zmin + zmax);
    const double emin = minEdge2D(dx, dy, dz);
    const double emax = maxEdge2D(dx, dy, dz);
    const bool thin = emin < horizTol * std::max(1.0, 0.1 * emax);

    bool isOpen = false;
    if (thin) {
      switch (openFace) {
      case 0: // +x
        isOpen = cx > x1 - t - 2. * horizTol && std::abs(dx - t) < 0.25 * t + horizTol;
        break;
      case 1: // -x
        isOpen = cx < x0 + t + 2. * horizTol && std::abs(dx - t) < 0.25 * t + horizTol;
        break;
      case 2: // +y
        isOpen = cy > y1 - t - 2. * horizTol && std::abs(dy - t) < 0.25 * t + horizTol;
        break;
      case 3: // -y
        isOpen = cy < y0 + t + 2. * horizTol && std::abs(dy - t) < 0.25 * t + horizTol;
        break;
      case 4: // +z — opening at top: rim / horizontal faces near z1 or cavity top
        isOpen = cz > z1 - t - 2. * horizTol && dz < horizTol * 10.;
        break;
      case 5: // -z
        isOpen = cz < z0 + t + 2. * horizTol && dz < horizTol * 10.;
        break;
      default:
        break;
      }
    }

    if (isOpen)
      openTags.push_back(surfTag);
    else
      wallTags.push_back(surfTag);
  }

  if (!wallTags.empty()) {
    const int g = gmsh::model::addPhysicalGroup(2, wallTags, -1);
    gmsh::model::setPhysicalName(2, g, physWall);
  }
  if (!openTags.empty()) {
    const int g = gmsh::model::addPhysicalGroup(2, openTags, -1);
    gmsh::model::setPhysicalName(2, g, physOpen);
  }
}

} // namespace mesh_gen
