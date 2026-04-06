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
#include <gmsh.h>
#include <stdexcept>

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

} // namespace mesh_gen
