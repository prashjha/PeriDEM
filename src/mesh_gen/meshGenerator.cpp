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
#include "util/feElementDefs.h"
#include <fstream>
#include <gmsh.h>
#include <iomanip>
#include <stdexcept>
#include <string>

namespace mesh_gen {

namespace {

/** One-shot Gmsh 2.2 ASCII from filled 2D triangle mesh (nodes z=0; matches fillMeshFromActiveGmshModel). */
void writeGmshMsh22From2DTriangleMesh(const mesh::Mesh &m, const std::string &path) {
  if (m.getElementType() != util::vtk_type_triangle)
    throw std::runtime_error("writeGmshMsh22From2DTriangleMesh: only triangle meshes are supported.");
  if (m.getDimension() != 2)
    throw std::runtime_error("writeGmshMsh22From2DTriangleMesh: dimension must be 2.");

  const auto &nodes = m.getNodes();
  const auto &enc = m.getElementConnectivities();
  const size_t n = nodes.size();
  const size_t ne = enc.size() / 3;

  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("writeGmshMsh22From2DTriangleMesh: cannot open " + path);

  out << std::setprecision(17);
  out << "$MeshFormat\n2.2 0 8\n$EndMeshFormat\n";
  out << "$Nodes\n" << n << "\n";
  for (size_t i = 0; i < n; ++i) {
    const auto &p = nodes[i];
    out << (i + 1) << " " << p.d_x << " " << p.d_y << " 0\n";
  }
  out << "$EndNodes\n";
  out << "$Elements\n" << ne << "\n";
  for (size_t e = 0; e < ne; ++e) {
    const size_t a = enc[3 * e] + 1;
    const size_t b = enc[3 * e + 1] + 1;
    const size_t c = enc[3 * e + 2] + 1;
    out << (e + 1) << " 2 2 0 1 " << a << " " << b << " " << c << "\n";
  }
  out << "$EndElements\n";
}

} // namespace

void generateBuiltinParticleMeshGmsh(const std::shared_ptr<geom::GeomObject> &geomObj, double h,
                                     const std::string &filenameStem, bool vtk_out,
                                     bool write_mesh_file, mesh::Mesh *out_mesh,
                                     const inp::MeshDeck *meshDeck, const inp::ModelDeck *modelDeck) {
  // Geometry is only `geomObj`; decks supply mesh/model metadata for `out_mesh` fill — no parallel param arrays.

  if (!geomObj)
    throw std::runtime_error("generateBuiltinParticleMeshGmsh: GeomObject pointer is null.");

  const std::string &geomName = geomObj->d_name;

  // AnnulusGeomObject always uses d_name == "annulus_object" (deck may say
  // circle_minus_circle / rectangle_minus_rectangle / etc.).
  bool known = (geomName == "annulus_object");
  if (!known) {
    for (const auto &g : geom::getAcceptableGeometries()) {
      if (g == geomName) {
        known = true;
        break;
      }
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
    // OCC-built 3D shapes (e.g. sphere, ellipsoid) do not attach `h` to geometry; without this,
    // Gmsh uses a default size and can produce far too few tets so mesh spacing > PD horizon.
    if (h > 0.) {
      gmsh::option::setNumber("Mesh.MeshSizeMin", h);
      gmsh::option::setNumber("Mesh.MeshSizeMax", h);
    }
    const int genDim = gmshMeshGenerateDim(modelDeck);
    if (genDim == 2)
      gmsh::option::setNumber("Mesh.Algorithm", 5); // 2D Delaunay (default 6 = Frontal-Delaunay breaks some geo polygons)
    gmsh::model::mesh::generate(genDim);
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
    const std::string mshPath = filenameStem + ".msh";
    if (modelDeck != nullptr && modelDeck->d_dim == 2 && out_mesh != nullptr)
      writeGmshMsh22From2DTriangleMesh(*out_mesh, mshPath);
    else
      gmsh::write(mshPath);
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
