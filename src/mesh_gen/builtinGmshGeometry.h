/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GEN_BUILTIN_GMSH_GEOMETRY_H
#define MESH_GEN_BUILTIN_GMSH_GEOMETRY_H

#include <string>

namespace geom {
class GeomObject;
}

namespace mesh_gen {

/**
 * Build OCC/geo entities in the current Gmsh model from a concrete geom::GeomObject.
 *
 * New shapes: add a branch here or in helpers (e.g. primitiveOccMesh.cpp), and register the name in
 * geom::acceptable_geometries / createGeomObject. AnnulusGeomObject uses d_name annulus_object;
 * meshGenerator also allows that name (deck types are circle_minus_circle / rectangle_minus_rectangle / …).
 * open_rect_channel_2d: U-channel polygon (geom::OpenRectChannel2D) + optional physical groups via
 * openBoundaryWalls2D.
 * open_cuboid_channel_3d: cuboidal shell with one outer face slab removed (geom::OpenCuboidChannel3D) +
 * physical groups via openBoundaryWalls3D.
 */
void buildGmshGeometryInCurrentModel(const geom::GeomObject &geom, double h);

} // namespace mesh_gen

#endif
