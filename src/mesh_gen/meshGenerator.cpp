/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "meshGenerator.h"
#include "builtinGmshGeometry.h"
#include "gmshMeshPipeline.h"
#include "geom/geomObjects.h"
#include "geom/geomObjectsUtil.h"
#include "inp/meshDeck.h"
#include "inp/modelDeck.h"
#include "mesh/mesh.h"
#include <gmsh.h>
#include <stdexcept>

namespace mesh_gen {

void generateBuiltinParticleMeshGmsh(const std::shared_ptr<geom::GeomObject> &geomObj, double h,
                                     const std::string &filenameStem, bool vtk_out,
                                     bool write_mesh_file, mesh::Mesh *out_mesh,
                                     const inp::MeshDeck *meshDeck, const inp::ModelDeck *modelDeck) {

  if (!geomObj)
    throw std::runtime_error("generateBuiltinParticleMeshGmsh: GeomObject pointer is null.");

  const std::string &geomName = geomObj->d_name;

  bool known = false;
  for (const auto &g : geom::getAcceptableGeometries()) {
    if (g == geomName) {
      known = true;
      break;
    }
  }
  if (!known)
    throw std::runtime_error(
        "generateBuiltinParticleMeshGmsh: geometry type is not in geom::acceptable_geometries.");

  gmsh::initialize();
  gmsh::option::setNumber("Mesh.MshFileVersion", 2.2);
  gmsh::clear();

  try {
    buildGmshGeometryInCurrentModel(*geomObj, h);
    gmsh::model::mesh::generate(gmshMeshGenerateDim(modelDeck));
  } catch (...) {
    gmsh::finalize();
    throw;
  }

  if (!write_mesh_file && out_mesh == nullptr)
    throw std::runtime_error(
        "generateBuiltinParticleMeshGmsh: when write_mesh_file is false, out_mesh is required.");

  if (out_mesh != nullptr) {
    if (meshDeck == nullptr || modelDeck == nullptr)
      throw std::runtime_error(
          "generateBuiltinParticleMeshGmsh: meshDeck and modelDeck are required when out_mesh is set.");
    fillMeshFromActiveGmshModel(out_mesh, meshDeck, modelDeck);
  }

  if (write_mesh_file) {
    if (filenameStem.empty())
      throw std::runtime_error(
          "generateBuiltinParticleMeshGmsh: non-empty filename stem is required when writing a .msh file.");
    gmsh::write(filenameStem + ".msh");
  }

  if (vtk_out) {
    if (filenameStem.empty())
      throw std::runtime_error(
          "generateBuiltinParticleMeshGmsh: non-empty filename stem is required for VTK output.");
    gmsh::write(filenameStem + ".vtk");
  }

  gmsh::finalize();
}

} // namespace mesh_gen
