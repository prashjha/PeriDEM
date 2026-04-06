/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef MESH_GEN_OPEN_BOUNDARY_WALLS_2D_H
#define MESH_GEN_OPEN_BOUNDARY_WALLS_2D_H

#include <string>

namespace mesh_gen {

/*!
 * After a 2D plane surface is built in the current Gmsh model, partition its
 * boundary curves into physical groups: "wall" vs "open" for a top opening at +y.
 *
 * Curves classified as "open" are nearly horizontal segments whose elevation is
 * near yOpen (outer top of a closed rectangular annulus). U-channels have no such
 * edge (the opening is a gap), so all curves are typically "wall".
 *
 * @param surfaceTag Gmsh surface tag (dimension 2).
 * @param yOpen      Reference y-coordinate of the intended open (top) side.
 * @param tol        Length tolerance for "horizontal" vs slanted edges.
 */
void physicalGroupsWallOpenFromY2D(int surfaceTag, double yOpen, double tol,
                                   const std::string &physWall = "wall",
                                   const std::string &physOpen = "open");

} // namespace mesh_gen

#endif
