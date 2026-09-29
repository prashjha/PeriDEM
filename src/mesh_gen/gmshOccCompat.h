/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#pragma once

#include <cmath>
#include <gmsh.h>
#include <vector>

namespace mesh_gen {

/*!
 * @brief OCC disk compatible with older Gmsh (no zAxis/xAxis) and newer APIs.
 *
 * Gmsh before ~4.11 only accepts addDisk(xc,yc,zc,rx,ry,tag). Later versions
 * add optional axis vectors for in-plane rotation. On older Gmsh, apply a
 * z-rotation about the disk center when @p theta is nonzero.
 */
inline int addDiskOcc(double xc, double yc, double zc, double rx, double ry,
                      double theta = 0.) {
#if (GMSH_API_VERSION_MAJOR > 4) ||                                        \
    (GMSH_API_VERSION_MAJOR == 4 && GMSH_API_VERSION_MINOR >= 11)
  const std::vector<double> zAxis = {0., 0., 1.};
  const std::vector<double> xAxis = {std::cos(theta), std::sin(theta), 0.};
  return gmsh::model::occ::addDisk(xc, yc, zc, rx, ry, -1, zAxis, xAxis);
#else
  const int tag = gmsh::model::occ::addDisk(xc, yc, zc, rx, ry, -1);
  if (std::abs(theta) > 1.0e-15)
    gmsh::model::occ::rotate({{2, tag}}, xc, yc, zc, 0., 0., 1., theta);
  return tag;
#endif
}

} // namespace mesh_gen
