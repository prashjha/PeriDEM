/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#include "openBoundaryWalls2D.h"
#include <algorithm>
#include <cmath>
#include <gmsh.h>
#include <vector>

namespace mesh_gen {

void physicalGroupsWallOpenFromY2D(int surfaceTag, double yOpen, double tol,
                                   const std::string &physWall,
                                   const std::string &physOpen) {

  std::vector<std::pair<int, int>> bnd;
  gmsh::model::getBoundary({{2, surfaceTag}}, bnd, false);

  std::vector<int> wallTags;
  std::vector<int> openTags;
  wallTags.reserve(bnd.size());
  openTags.reserve(bnd.size());

  const double horizEps = std::max(tol * 1.0e-3, 1.0e-12 * (1.0 + std::abs(yOpen)));

  for (const auto &pr : bnd) {
    if (pr.first != 1)
      continue;
    double xmin = 0., ymin = 0., zmin = 0., xmax = 0., ymax = 0., zmax = 0.;
    gmsh::model::getBoundingBox(pr.first, pr.second, xmin, ymin, zmin, xmax, ymax, zmax);
    const double dy = ymax - ymin;
    const double yc = 0.5 * (ymin + ymax);
    const bool nearlyHorizontal = dy < horizEps;
    const bool nearTopOpen = yc >= yOpen - tol && nearlyHorizontal;
    if (nearTopOpen)
      openTags.push_back(pr.second);
    else
      wallTags.push_back(pr.second);
  }

  if (!wallTags.empty()) {
    const int g = gmsh::model::addPhysicalGroup(1, wallTags, -1);
    gmsh::model::setPhysicalName(1, g, physWall);
  }
  if (!openTags.empty()) {
    const int g = gmsh::model::addPhysicalGroup(1, openTags, -1);
    gmsh::model::setPhysicalName(1, g, physOpen);
  }
}

} // namespace mesh_gen
