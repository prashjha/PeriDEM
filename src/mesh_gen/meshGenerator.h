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

#include <memory>
#include <string>

namespace geom {
class GeomObject;
}
namespace inp {
struct MeshDeck;
struct ModelDeck;
}
namespace mesh {
class Mesh;
}

namespace mesh_gen {

/**
 * @brief In-process Gmsh mesh from geom::GeomObject (after createGeomObject on deck / Particle data).
 *
 * Single entry point: geometry is only the shared object; mesh size, VTK, file stem, and write
 * flags are explicit. Simulation code unpacks MeshDeck + GeomData.d_geom_p at the call site.
 */
void generateBuiltinParticleMeshGmsh(const std::shared_ptr<geom::GeomObject> &geomObj, double h,
                                     const std::string &filenameStem, bool vtk_out,
                                     bool write_mesh_file, mesh::Mesh *out_mesh,
                                     const inp::MeshDeck *meshDeck, const inp::ModelDeck *modelDeck);

} // namespace mesh_gen

#endif // MESH_GENERATOR_H
