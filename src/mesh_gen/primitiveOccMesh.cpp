/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "primitiveOccMesh.h"
#include "geom/geomObjects.h"
#include "geom/geomUtilFunctions.h"
#include <algorithm>
#include <cmath>
#include <gmsh.h>
#include <stdexcept>
#include <vector>

namespace mesh_gen {

void buildCylinderOcc(const geom::Cylinder &c) {

  if (c.d_r <= 0.)
    throw std::runtime_error("buildCylinderOcc: radius must be positive.");
  if (c.d_l <= 0.)
    throw std::runtime_error("buildCylinderOcc: length must be positive.");

  const double dx = c.d_l * c.d_xa.d_x;
  const double dy = c.d_l * c.d_xa.d_y;
  const double dz = c.d_l * c.d_xa.d_z;

  gmsh::model::occ::addCylinder(c.d_xBegin.d_x, c.d_xBegin.d_y, c.d_xBegin.d_z, dx, dy, dz, c.d_r);
  gmsh::model::occ::synchronize();
}

void buildEllipseOcc(const geom::Ellipse &e, double h) {

  if (e.d_a <= 0. || e.d_b <= 0.)
    throw std::runtime_error("buildEllipseOcc: semi-axes must be positive.");

  const double rx = std::max(e.d_a, e.d_b);
  const double ry = std::min(e.d_a, e.d_b);
  double theta = e.d_theta;
  if (e.d_a < e.d_b)
    theta += M_PI / 2.;

  const std::vector<double> zAxis = {0., 0., 1.};
  const std::vector<double> xAxis = {std::cos(theta), std::sin(theta), 0.};

  const int s = gmsh::model::occ::addDisk(e.d_x.d_x, e.d_x.d_y, e.d_x.d_z, rx, ry, -1, zAxis, xAxis);
  gmsh::model::occ::synchronize();
  const int p = gmsh::model::occ::addPoint(e.d_x.d_x, e.d_x.d_y, e.d_x.d_z, h);
  gmsh::model::occ::synchronize();
  gmsh::model::mesh::embed(0, {p}, 2, s);
}

void buildEllipsoidOcc(const geom::Ellipsoid &e) {

  if (e.d_a <= 0. || e.d_b <= 0. || e.d_c <= 0.)
    throw std::runtime_error("buildEllipsoidOcc: semi-axes must be positive.");

  // Unit sphere at origin, then x' = R diag(r1,r2,r3) x + c (row-major 4×4 for gmsh).
  double R[9];
  geom::ellipsoidRotationMatrix(e, R);

  std::vector<double> mat(16);
  for (int i = 0; i < 3; ++i) {
    mat[static_cast<size_t>(i * 4 + 0)] = R[i * 3 + 0] * e.d_a;
    mat[static_cast<size_t>(i * 4 + 1)] = R[i * 3 + 1] * e.d_b;
    mat[static_cast<size_t>(i * 4 + 2)] = R[i * 3 + 2] * e.d_c;
    mat[static_cast<size_t>(i * 4 + 3)] = (i == 0) ? e.d_x.d_x : (i == 1) ? e.d_x.d_y : e.d_x.d_z;
  }
  mat[12] = mat[13] = mat[14] = 0.;
  mat[15] = 1.;

  const int v = gmsh::model::occ::addSphere(0., 0., 0., 1.);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::affineTransform({{3, v}}, mat);
  gmsh::model::occ::synchronize();
  gmsh::model::occ::removeAllDuplicates();
  gmsh::model::occ::synchronize();
}

} // namespace mesh_gen
