/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "gmshMeshPipeline.h"
#include "inp/meshDeck.h"
#include "inp/modelDeck.h"
#include "mesh/mesh.h"
#include "util/feElementDefs.h"
#include <gmsh.h>
#include <stdexcept>
#include <unordered_map>

namespace mesh_gen {

int gmshMeshGenerateDim(const inp::ModelDeck *modelDeck) {
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

  if (modelDeck != nullptr && modelDeck->d_dim == 2) {
    for (size_t i = 0; i < nodeTags.size(); ++i)
      coord[3 * i + 2] = 0.;
  }

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

} // namespace mesh_gen
