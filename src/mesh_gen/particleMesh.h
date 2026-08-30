/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GEN_PARTICLE_MESH_H
#define MESH_GEN_PARTICLE_MESH_H

#include <memory>
#include <string>

namespace geom {
struct GeomData;
}
namespace inp {
struct MeshDeck;
struct ModelDeck;
}
namespace mesh {
class Mesh;
}

namespace mesh_gen {

/*!
 * @brief Build a reference-particle mesh: file, uniform rectangle, or in-process Gmsh.
 */
std::shared_ptr<mesh::Mesh>
createParticleMesh(const inp::MeshDeck &zmeshDeck,
                            const geom::GeomData &zgeomDeck,
                            const inp::ModelDeck *modelDeck,
                            const std::string &modelName);

} // namespace mesh_gen

#endif // MESH_GEN_PARTICLE_MESH_H
