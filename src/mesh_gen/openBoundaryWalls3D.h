/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef MESH_GEN_OPEN_BOUNDARY_WALLS_3D_H
#define MESH_GEN_OPEN_BOUNDARY_WALLS_3D_H

#include "util/point.h"
#include <string>

namespace mesh_gen {

/*!
 * After a 3D volume is built in the current Gmsh model, partition its boundary surfaces into
 * physical groups "wall" vs "open" for an open cuboidal shell (one outer face slab removed).
 *
 * @param volumeTag  Gmsh volume tag (dimension 3).
 * @param openFace     Same convention as geom::OpenCuboidChannel3D::d_openFace (0..5).
 * @param lo           Outer AABB low corner (world axes).
 * @param hi           Outer AABB high corner.
 * @param t            Wall thickness (used to locate the opening plane).
 * @param tol          Length tolerance for classifying nearly planar faces.
 * @param physWall     Physical name for wall surfaces.
 * @param physOpen     Physical name for opening / cavity-facing rim surfaces.
 */
void physicalGroupsWallOpenFromFace3D(int volumeTag, int openFace, const util::Point &lo,
                                      const util::Point &hi, double t, double tol,
                                      const std::string &physWall = "wall",
                                      const std::string &physOpen = "open");

} // namespace mesh_gen

#endif
