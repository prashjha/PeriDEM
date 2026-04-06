/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GEN_PRIMITIVE_OCC_MESH_H
#define MESH_GEN_PRIMITIVE_OCC_MESH_H

namespace geom {
class Cylinder;
class Ellipse;
class Ellipsoid;
}

namespace mesh_gen {

/** OCC cylinder matching `geom::Cylinder` (base center, axis × length, radius). */
void buildCylinderOcc(const geom::Cylinder &c);

/** OCC disk / ellipse in the plane z = center.d_z (see `geom::Ellipse`). */
void buildEllipseOcc(const geom::Ellipse &e, double h);

/** OCC ellipsoid: unit sphere + affine map R diag(r1,r2,r3) and translation. */
void buildEllipsoidOcc(const geom::Ellipsoid &e);

} // namespace mesh_gen

#endif
