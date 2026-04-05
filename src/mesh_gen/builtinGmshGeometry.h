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
 * New shapes: add a branch in builtinGmshGeometry.cpp (and register the name in
 * geom::acceptable_geometries / createGeomObject).
 */
void buildGmshGeometryInCurrentModel(const geom::GeomObject &geom, double h);

} // namespace mesh_gen

#endif
