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
#include "geom/geomObjects.h"
#include "geom/geomObjectsUtil.h"
#include "inp/meshDeck.h"
#include "inp/modelDeck.h"
#include "mesh/mesh.h"
#include "util/feElementDefs.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace mesh_gen {

namespace {

/*!
 * Gmsh mesh::generate(dim) requests meshing up to dimension dim (1, 2, or 3).
 * Must match ModelDeck::d_dim so 2D simulations mesh surfaces only and 3D
 * simulations mesh volumes; do not hardcode 3 for all cases.
 */
inline int gmshMeshGenerateDim(const inp::ModelDeck *modelDeck) {
  if (modelDeck == nullptr)
    return 3;
  const int d = static_cast<int>(modelDeck->d_dim);
  if (d < 1 || d > 3)
    return 3;
  return d;
}

void fillMeshFromActiveGmshModel(mesh::Mesh *mesh_p, const inp::MeshDeck *meshDeck,
                                 const inp::ModelDeck *modelDeck) {

  std::vector<std::size_t> nodeTags;
  std::vector<double> coord;
  std::vector<double> paramCoord;
  gmsh::model::mesh::getNodes(nodeTags, coord, paramCoord);

  std::unordered_map<std::size_t, std::size_t> tagToIdx;
  tagToIdx.reserve(nodeTags.size());
  for (size_t i = 0; i < nodeTags.size(); ++i)
    tagToIdx[nodeTags[i]] = i;

  std::vector<util::Point> nodes(nodeTags.size());
  for (size_t i = 0; i < nodeTags.size(); ++i)
    nodes[i] = util::Point(coord[3 * i], coord[3 * i + 1], coord[3 * i + 2]);

  std::vector<int> elementTypes;
  std::vector<std::vector<std::size_t>> elementTags, elementNodeTags;
  gmsh::model::mesh::getElements(elementTypes, elementTags, elementNodeTags, -1, -1);

  bool hasTetra = false;
  for (size_t t = 0; t < elementTypes.size(); ++t) {
    if (elementTypes[t] == util::msh_type_tetrahedron) {
      hasTetra = true;
      break;
    }
  }

  if (hasTetra) {
    std::vector<size_t> enc;
    for (size_t t = 0; t < elementTypes.size(); ++t) {
      if (elementTypes[t] != util::msh_type_tetrahedron)
        continue;
      const auto &nt = elementNodeTags[t];
      for (size_t j = 0; j < nt.size(); j += 4) {
        enc.push_back(tagToIdx.at(nt[j]));
        enc.push_back(tagToIdx.at(nt[j + 1]));
        enc.push_back(tagToIdx.at(nt[j + 2]));
        enc.push_back(tagToIdx.at(nt[j + 3]));
      }
    }
    if (enc.empty())
      throw std::runtime_error("fillMeshFromActiveGmshModel: no Gmsh tetrahedron elements found.");
    mesh_p->loadFromTetraElements3D(std::move(nodes), std::move(enc), meshDeck, modelDeck);
    return;
  }

  std::vector<size_t> enc;
  for (size_t t = 0; t < elementTypes.size(); ++t) {
    if (elementTypes[t] != util::msh_type_triangle)
      continue;
    const auto &nt = elementNodeTags[t];
    for (size_t j = 0; j < nt.size(); j += 3) {
      enc.push_back(tagToIdx.at(nt[j]));
      enc.push_back(tagToIdx.at(nt[j + 1]));
      enc.push_back(tagToIdx.at(nt[j + 2]));
    }
  }

  if (enc.empty())
    throw std::runtime_error("fillMeshFromActiveGmshModel: no Gmsh triangle elements found.");

  mesh_p->loadFromTriangleElements2D(std::move(nodes), std::move(enc), meshDeck, modelDeck);
}

void meshPolygon2DGeoFromVertices(const std::vector<util::Point> &verts, double h) {
  if (verts.size() < 3)
    throw std::runtime_error("meshPolygon2DGeoFromVertices: need at least 3 vertices.");

  std::vector<int> pids;
  pids.reserve(verts.size());
  for (const auto &v : verts)
    pids.push_back(gmsh::model::geo::addPoint(v.d_x, v.d_y, v.d_z, h));

  std::vector<int> lines;
  const size_t n = verts.size();
  lines.reserve(n);
  for (size_t i = 0; i < n; ++i)
    lines.push_back(gmsh::model::geo::addLine(pids[i], pids[(i + 1) % n]));

  int cl = gmsh::model::geo::addCurveLoop(lines);
  gmsh::model::geo::addPlaneSurface({cl});
  gmsh::model::geo::synchronize();
}

std::vector<util::Point> boundaryVerticesForGeometry(const std::string &geomName,
                                                     const std::vector<double> &params) {

  if (geomName == "square") {
    if (params.size() == 1)
      return geom::Square(params[0]).d_vertices;
    if (params.size() == 4)
      return geom::Square(params[0], util::Point(params[1], params[2], params[3])).d_vertices;
    if (params.size() == 6)
      return geom::Square(util::Point(params[0], params[1], params[2]),
                          util::Point(params[3], params[4], params[5]))
          .d_vertices;
  } else if (geomName == "rectangle") {
    if (params.size() == 2)
      return geom::Rectangle(params[0], params[1]).d_vertices;
    if (params.size() == 5)
      return geom::Rectangle(params[0], params[1],
                             util::Point(params[2], params[3], params[4]))
          .d_vertices;
    if (params.size() == 6)
      return geom::Rectangle(util::Point(params[0], params[1], params[2]),
                             util::Point(params[3], params[4], params[5]))
          .d_vertices;
  } else if (geomName == "triangle") {
    if (params.size() == 1)
      return geom::Triangle(params[0]).d_vertices;
    if (params.size() == 4)
      return geom::Triangle(params[0], util::Point(params[1], params[2], params[3])).d_vertices;
    if (params.size() == 7)
      return geom::Triangle(params[0], util::Point(params[1], params[2], params[3]),
                            util::Point(params[4], params[5], params[6]))
          .d_vertices;
  } else if (geomName == "hexagon") {
    if (params.size() == 4)
      return geom::Hexagon(params[0], util::Point(params[1], params[2], params[3])).d_vertices;
    if (params.size() == 7)
      return geom::Hexagon(params[0], util::Point(params[1], params[2], params[3]),
                           util::Point(params[4], params[5], params[6]))
          .d_vertices;
  } else if (geomName == "drum2d") {
    geom::Drum2D drum;
    if (params.size() >= 8)
      drum = geom::Drum2D(params[0], params[1],
                          util::Point(params[2], params[3], params[4]),
                          util::Point(params[5], params[6], params[7]));
    else if (params.size() >= 5)
      drum = geom::Drum2D(params[0], params[1],
                          util::Point(params[2], params[3], params[4]));
    else
      throw std::runtime_error("boundaryVerticesForGeometry: drum2d needs at least 5 parameters.");

    static const int kDrumBoundaryOrder[] = {3, 2, 1, 0, 5, 4};
    std::vector<util::Point> poly;
    poly.reserve(6);
    for (int k : kDrumBoundaryOrder)
      poly.push_back(drum.d_vertices[k]);
    return poly;
  }

  throw std::runtime_error("boundaryVerticesForGeometry: unsupported 2D geometry or parameter count.");
}

void buildGmshGeometryInCurrentModel(const std::string &geomName,
                                     const std::vector<double> &params, double h) {

  if (geomName == "sphere") {
    double r = params[0];
    util::Point c(0., 0., 0.);
    if (params.size() >= 4)
      c = util::Point(params[1], params[2], params[3]);
    gmsh::model::occ::addSphere(c.d_x, c.d_y, c.d_z, r);
    gmsh::model::occ::synchronize();
    return;
  }

  if (geomName == "cube") {
    if (params.size() == 1) {
      const double L = params[0];
      gmsh::model::occ::addBox(-0.5 * L, -0.5 * L, -0.5 * L, L, L, L);
    } else if (params.size() == 4) {
      const double L = params[0];
      util::Point x(params[1], params[2], params[3]);
      gmsh::model::occ::addBox(x.d_x - 0.5 * L, x.d_y - 0.5 * L, x.d_z - 0.5 * L, L, L, L);
    } else if (params.size() == 6) {
      util::Point x1(params[0], params[1], params[2]);
      util::Point x2(params[3], params[4], params[5]);
      const double lo_x = std::min(x1.d_x, x2.d_x);
      const double lo_y = std::min(x1.d_y, x2.d_y);
      const double lo_z = std::min(x1.d_z, x2.d_z);
      const double hi_x = std::max(x1.d_x, x2.d_x);
      const double hi_y = std::max(x1.d_y, x2.d_y);
      const double hi_z = std::max(x1.d_z, x2.d_z);
      gmsh::model::occ::addBox(lo_x, lo_y, lo_z, hi_x - lo_x, hi_y - lo_y, hi_z - lo_z);
    } else
      throw std::runtime_error("buildGmshGeometryInCurrentModel: cube needs 1, 4, or 6 parameters.");
    gmsh::model::occ::synchronize();
    return;
  }

  if (geomName == "cuboid") {
    if (params.size() != 6)
      throw std::runtime_error("buildGmshGeometryInCurrentModel: cuboid needs 6 parameters (dx,dy,dz,cx,cy,cz).");
    const double dx = params[0];
    const double dy = params[1];
    const double dz = params[2];
    util::Point c(params[3], params[4], params[5]);
    gmsh::model::occ::addBox(c.d_x - 0.5 * dx, c.d_y - 0.5 * dy, c.d_z - 0.5 * dz, dx, dy, dz);
    gmsh::model::occ::synchronize();
    return;
  }

  if (geomName == "circle") {
    if (params.size() < 4)
      throw std::runtime_error(
          "buildGmshGeometryInCurrentModel: circle needs radius and center (4 parameters).");
    const double r = params[0];
    const double cx = params[1], cy = params[2], cz = params[3];
    const int c = gmsh::model::occ::addCircle(cx, cy, cz, r);
    const int cl = gmsh::model::occ::addCurveLoop({c});
    const int s = gmsh::model::occ::addPlaneSurface({cl});
    const int p = gmsh::model::occ::addPoint(cx, cy, cz, h);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::embed(0, {p}, 2, s);
    return;
  }

  auto verts = boundaryVerticesForGeometry(geomName, params);
  meshPolygon2DGeoFromVertices(verts, h);
}

} // namespace

void generateBuiltinParticleMeshGmsh(const std::string &geomName,
                                       const std::vector<double> &params, double h,
                                       const std::string &filenameStem, bool vtk_out,
                                       bool write_mesh_file, mesh::Mesh *out_mesh,
                                       const inp::MeshDeck *meshDeck,
                                       const inp::ModelDeck *modelDeck) {

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

  if (!geom::isNumberOfParamForGeometryValid(params.size(), geomName))
    throw std::runtime_error(
        "generateBuiltinParticleMeshGmsh: parameter count does not match geometry " + geomName + ".");

  gmsh::initialize();
  gmsh::option::setNumber("Mesh.MshFileVersion", 2.2);
  gmsh::clear();

  try {
    buildGmshGeometryInCurrentModel(geomName, params, h);
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