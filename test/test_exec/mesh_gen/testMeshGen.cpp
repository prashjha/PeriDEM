/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "mesh_gen/meshGenerator.h"
#include "geom/complexGeomObjects.h"
#include "geom/geomObjects.h"
#include "geom/geomObjectsUtil.h"
#include "inp/meshDeck.h"
#include "inp/modelDeck.h"
#include "mesh/mesh.h"
#include "util/io.h"
#include "util/point.h"
#include <cmath>
#include <filesystem>
#include <format>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

namespace {

/**
 * @brief Model dimension for mesh deck (annulus deck names map to annulus_object with d_dim set).
 */
size_t meshDimForBuiltin(const std::string &deckName, const std::shared_ptr<geom::GeomObject> &obj) {
  if (obj->d_name == "annulus_object")
    return static_cast<const geom::AnnulusGeomObject &>(*obj).d_dim;
  return geom::getGeomTypeToDim(deckName);
}

/**
 * @brief Circle: write .msh and .vtk via the built-in Gmsh path (file I/O smoke).
 */
bool testCircleMeshWritesFiles() {
  util::io::log("Testing circle mesh file output (gmsh_builtin_mesh)...\n");

  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double radius = 0.001;
  const double meshSize = radius / 5.0;

  const fs::path outputDir = "test_output/mesh_gen";
  fs::create_directories(outputDir);

  try {
    auto circle =
        std::make_shared<geom::Circle>(radius, util::Point(center[0], center[1], center[2]));
    mesh_gen::generateBuiltinParticleMeshGmsh(circle, meshSize, (outputDir / "circle").string(),
                                              true, true, nullptr, nullptr, nullptr);

    const bool mshOk = fs::exists(outputDir / "circle.msh");
    const bool vtkOk = fs::exists(outputDir / "circle.vtk");
    if (!mshOk || !vtkOk) {
      util::io::log("Error: Expected circle.msh and circle.vtk under test_output/mesh_gen.\n");
      if (!mshOk)
        util::io::log("Missing: circle.msh\n");
      if (!vtkOk)
        util::io::log("Missing: circle.vtk\n");
      return false;
    }

    util::io::log("Circle mesh file output test passed.\n");
    return true;
  } catch (const std::exception &e) {
    util::io::log(std::format("Error in mesh generation: {}\n", e.what()));
    return false;
  }
}

/**
 * @brief For one acceptable geometry name: exampleGeomParams → mesh in memory → non-empty nodes,
 *        every node inside geom::box() (loose tolerance).
 */
bool builtinMeshSmokeTestForGeometry(const std::string &geomName, double s) {
  try {
    auto geom = geom::makeExampleGeomObject(geomName, util::Point(0., 0., 0.), s);
    const double meshSize = s / 5.0;

    inp::MeshDeck meshDeck;
    meshDeck.d_hMeshing = meshSize;
    meshDeck.d_filename = std::string("smoke_") + geomName + ".msh";

    const size_t dim = meshDimForBuiltin(geomName, geom);
    const auto modelJson =
        inp::ModelDeck::getExampleJson(dim, 0.001, 10, "finite_difference", "central_difference", true,
                                       2, "Multi_Particle", 0);
    inp::ModelDeck modelDeck(modelJson);

    mesh::Mesh mesh;
    mesh_gen::generateBuiltinParticleMeshGmsh(geom, meshSize, "", false, false, &mesh, &meshDeck,
                                              &modelDeck);

    if (mesh.getNodes().empty()) {
      util::io::log(std::format("Error: mesh smoke: {} produced zero nodes.\n", geomName));
      return false;
    }

    const auto box = geom->box();
    const double tol = 4.0 * meshSize;
    for (const auto &p : mesh.getNodes()) {
      if (p.d_x < box.first.d_x - tol || p.d_y < box.first.d_y - tol || p.d_z < box.first.d_z - tol ||
          p.d_x > box.second.d_x + tol || p.d_y > box.second.d_y + tol || p.d_z > box.second.d_z + tol) {
        util::io::log(std::format(
            "Error: mesh smoke {}: node ({},{},{}) outside geometry box (tol {}).\n", geomName, p.d_x,
            p.d_y, p.d_z, tol));
        return false;
      }
    }

    util::io::log(std::format("  mesh smoke [ {} ] passed.\n", geomName));
    return true;
  } catch (const std::exception &e) {
    util::io::log(std::format("Error: mesh smoke {}: {}\n", geomName, e.what()));
    return false;
  }
}

} // namespace

int main() {
  util::io::log("Starting mesh generator tests (loop over geom::getAcceptableGeometries)...\n");

  bool allTestsPassed = true;

  if (!testCircleMeshWritesFiles()) {
    util::io::log("Circle mesh file output test failed.\n");
    allTestsPassed = false;
  }

  const double s = 0.001;
  util::io::log("Builtin Gmsh mesh smoke tests (exampleGeomParams + bounding box)...\n");
  for (const std::string &name : geom::getAcceptableGeometries()) {
    if (!builtinMeshSmokeTestForGeometry(name, s)) {
      util::io::log(std::format("Mesh smoke failed for geometry: {}\n", name));
      allTestsPassed = false;
    }
  }

  if (allTestsPassed) {
    util::io::log("All mesh generator tests passed.\n");
    return 0;
  }
  util::io::log("Some mesh generator tests failed.\n");
  return 1;
}
