/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GEN_GMSH_MESH_PIPELINE_H
#define MESH_GEN_GMSH_MESH_PIPELINE_H

#include <cstddef>
#include <vector>

namespace inp {
struct MeshDeck;
struct ModelDeck;
}
namespace mesh {
class Mesh;
}

namespace mesh_gen {

/** Meshing dimension for Gmsh::generate (1–3), aligned with ModelDeck::d_dim. */
int gmshMeshGenerateDim(const inp::ModelDeck *modelDeck);

/** Fill PeriDEM Mesh from the active Gmsh model (after mesh::generate). */
void fillMeshFromActiveGmshModel(mesh::Mesh *mesh_p, const inp::MeshDeck *meshDeck,
                                 const inp::ModelDeck *modelDeck);

} // namespace mesh_gen

#endif
