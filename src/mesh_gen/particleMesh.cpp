/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "particleMesh.h"

#include "geom/geomObjectsUtil.h"
#include "inp/meshDeck.h"
#include "inp/modelDeck.h"
#include "mesh/mesh.h"
#include "mesh/meshUtil.h"
#include "meshGenerator.h"
#include "util/io.h"

#include <stdexcept>

std::shared_ptr<mesh::Mesh>
mesh_gen::createParticleMesh(const inp::MeshDeck &zmeshDeck,
                                      const geom::GeomData &zgeomDeck,
                                      const inp::ModelDeck *modelDeck,
                                      const std::string &modelName) {
  if (!zmeshDeck.d_createMesh) {
    return std::make_shared<mesh::Mesh>(&zmeshDeck, modelDeck);
  }

  if (zmeshDeck.d_createMeshInfo == "uniform" &&
      zgeomDeck.d_geomName == "rectangle") {

    if (!zgeomDeck.d_geom_p)
      throw std::runtime_error(
          modelName + ": uniform mesh on rectangle requires particle geometry object "
                      "(geom::createGeomObject on Particle.Set_i).");

    const size_t dim = modelDeck->d_dim;
    const auto bb = zgeomDeck.d_geom_p->box();
    std::pair<std::vector<double>, std::vector<double>> box;
    box.first.reserve(dim);
    box.second.reserve(dim);
    for (size_t i = 0; i < dim; ++i) {
      const double lo_i =
          (i == 0) ? bb.first.d_x : (i == 1) ? bb.first.d_y : bb.first.d_z;
      const double hi_i =
          (i == 0) ? bb.second.d_x : (i == 1) ? bb.second.d_y : bb.second.d_z;
      box.first.push_back(lo_i);
      box.second.push_back(hi_i);
    }

    std::vector<size_t> nGrid(dim);
    for (size_t i = 0; i < dim; ++i) {
      const double span = box.second[i] - box.first[i];
      if (span <= 0.)
        throw std::runtime_error(
            modelName + ": uniform mesh: non-positive axis extent from geom box "
                        "(axis " +
            std::to_string(i) + ").");
      nGrid[i] = static_cast<size_t>(span / zmeshDeck.d_hMeshing);
    }

    mesh::Mesh temp_mesh;
    mesh::createUniformMesh(&temp_mesh, dim, box, nGrid);
    return std::make_shared<mesh::Mesh>(temp_mesh);
  }

  if (zmeshDeck.d_createMeshInfo == "gmsh_builtin_mesh") {
    if (!zgeomDeck.d_geom_p)
      throw std::runtime_error(
          modelName + ": gmsh_builtin_mesh requires particle geometry object "
                      "(geom::createGeomObject on Particle.Set_i).");
    mesh::Mesh temp_mesh;
    const std::string mesh_stem =
        zmeshDeck.d_filename.empty()
            ? std::string()
            : util::io::removeExtensionFromFile(zmeshDeck.d_filename);
    mesh_gen::generateBuiltinParticleMeshGmsh(
        zgeomDeck.d_geom_p, zmeshDeck.d_hMeshing, mesh_stem, false,
        zmeshDeck.d_writeMeshFile, &temp_mesh, &zmeshDeck, modelDeck);
    return std::make_shared<mesh::Mesh>(temp_mesh);
  }

  throw std::runtime_error(
      "Error: Unsupported in-built mesh: CreateMesh.Info = " +
      zmeshDeck.d_createMeshInfo + " with geometry = " + zgeomDeck.d_geomName);
}
