/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GENERATOR_H
#define MESH_GENERATOR_H

#include <vector>
#include <string>
#include <cmath>
#include <gmsh.h>
#include <stdexcept>

namespace inp {
struct MeshDeck;
struct ModelDeck;
}
namespace mesh {
class Mesh;
}

namespace mesh_gen {

/**
 * @brief In-process Gmsh mesh from a built-in geometry name (geom::acceptable_geometries).
 *
 * Meshes the full domain (not a symmetry-reduced patch). 2D shapes use planar OCC/geo;
 * sphere/cube/cuboid use volumes. CreateMesh.Info must be "gmsh_builtin_mesh".
 */
void generateBuiltinParticleMeshGmsh(const std::string &geomName,
                                       const std::vector<double> &params, double h,
                                       const std::string &filenameStem, bool vtk_out, bool write_mesh_file,
                                       mesh::Mesh *out_mesh, const inp::MeshDeck *meshDeck,
                                       const inp::ModelDeck *modelDeck);

} // namespace mesh_gen

#endif // MESH_GENERATOR_H
