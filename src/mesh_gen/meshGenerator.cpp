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

namespace mesh_gen {

std::vector<std::pair<int, int>> getGmshEntities() {
  std::vector<std::pair<int, int>> entities;
  for (int dim = 0; dim < 4; dim++) {
    std::vector<std::pair<int, int>> dimEntities;
    gmsh::model::getEntities(dimEntities, dim);
    entities.insert(entities.end(), dimEntities.begin(), dimEntities.end());
  }
  return entities;
}

void gmshTransform(const std::vector<std::pair<int, int>>& m, int offset_entity, int offset_node, int offset_element, double tx, double ty, double tz) {
  for (const auto& e : m) {
    // Add new discrete entity
    std::vector<std::pair<int, int>> boundaryTags;
    gmsh::model::getBoundary({e}, boundaryTags, false);
    std::vector<int> outDimTags;
    for (const auto& tag : boundaryTags) {
      outDimTags.push_back(std::abs(tag.second) + offset_entity);
      if (tag.second < 0) outDimTags.back() *= -1;
    }
    gmsh::model::addDiscreteEntity(e.first, e.second + offset_entity, outDimTags);

    // Get and transform node coordinates
    std::vector<std::size_t> nodeTags;
    std::vector<double> coord;
    std::vector<double> paramCoord;
    gmsh::model::mesh::getNodes(nodeTags, coord, paramCoord, e.first, e.second);

    std::vector<double> newCoord;
    for (std::size_t i = 0; i < coord.size(); i += 3) {
      newCoord.push_back(coord[i] * tx);
      newCoord.push_back(coord[i + 1] * ty);
      newCoord.push_back(coord[i + 2] * tz);
    }

    // Add transformed nodes
    std::vector<std::size_t> newNodeTags;
    for (auto tag : nodeTags) newNodeTags.push_back(tag + offset_node);
    gmsh::model::mesh::addNodes(e.first, e.second + offset_entity, newNodeTags, newCoord);

    // Get and transform elements
    std::vector<int> elementTypes;
    std::vector<std::vector<std::size_t>> elementTags, elementNodeTags;
    gmsh::model::mesh::getElements(elementTypes, elementTags, elementNodeTags, e.first, e.second);

    // Add transformed elements
    for (std::size_t i = 0; i < elementTypes.size(); i++) {
      std::vector<std::size_t> newElementTags;
      for (auto tag : elementTags[i]) newElementTags.push_back(tag + offset_element);

      std::vector<std::size_t> newElementNodeTags;
      for (auto tag : elementNodeTags[i]) newElementNodeTags.push_back(tag + offset_node);

      gmsh::model::mesh::addElements(e.first, e.second + offset_entity, {elementTypes[i]}, {newElementTags}, {newElementNodeTags});
    }

    // Reverse orientation if needed
    if ((tx * ty * tz) < 0) {
      gmsh::model::mesh::reverse({{e.first, e.second + offset_entity}});
    }
  }
}

void gmshTranslate(const std::vector<double>& xc) {
  // Get all nodes
  std::vector<std::size_t> nodeTags;
  std::vector<double> coord;
  std::vector<double> paramCoord;
  gmsh::model::mesh::getNodes(nodeTags, coord, paramCoord);

  // Update each node's coordinates
  for (std::size_t i = 0; i < nodeTags.size(); i++) {
    std::vector<double> newCoord = {
      coord[3*i] + xc[0],
      coord[3*i + 1] + xc[1],
      coord[3*i + 2] + xc[2]
    };
    gmsh::model::mesh::setNode(nodeTags[i], newCoord, paramCoord.empty() ? std::vector<double>() : std::vector<double>(paramCoord.begin() + 3*i, paramCoord.begin() + 3*i + 3));
  }
}

void circleMeshSymmetric(const std::vector<double>& xc, double r, double h, const std::string& filename, bool vtk_out, bool symmetric_mesh) {
  gmsh::initialize();
  gmsh::option::setNumber("Mesh.MshFileVersion", 2.2);

  if (symmetric_mesh) {
    // Create 1/4 circle at origin first, then mirror and translate
    std::vector<double> xc_mesh = {0., 0., 0.};

    // Create points for 1/4 circle
    int p1 = gmsh::model::geo::addPoint(xc_mesh[0], xc_mesh[1], xc_mesh[2], h);  // Center
    int p2 = gmsh::model::geo::addPoint(xc_mesh[0] + r, xc_mesh[1], xc_mesh[2], h);  // Right
    int p3 = gmsh::model::geo::addPoint(xc_mesh[0], xc_mesh[1] + r, xc_mesh[2], h);  // Top

    // Create circle arc and lines
    int l1 = gmsh::model::geo::addCircleArc(p2, p1, p3);  // Quarter circle arc
    int l2 = gmsh::model::geo::addLine(p1, p2);  // Center to right
    int l3 = gmsh::model::geo::addLine(p3, p1);  // Top to center

    // Create curve loop and surface
    int c1 = gmsh::model::geo::addCurveLoop({l2, l1, l3});
    int s1 = gmsh::model::geo::addPlaneSurface({c1});

    gmsh::model::geo::synchronize();
    gmsh::model::mesh::generate(3);

    // Get mesh data for mirroring
    auto m = getGmshEntities();

    // Mirror the mesh in all quadrants
    gmshTransform(m, 1000, 1000000, 1000000, -1, 1, 1);  // Mirror across y-axis
    gmshTransform(m, 2000, 2000000, 2000000, 1, -1, 1);  // Mirror across x-axis
    gmshTransform(m, 3000, 3000000, 3000000, -1, -1, 1);  // Mirror across origin

    // Remove duplicate nodes
    gmsh::model::mesh::removeDuplicateNodes();

    // Translate to specified center coordinates
    gmshTranslate(xc);
  } else {
    // Create full circle directly
    int c = gmsh::model::occ::addCircle(xc[0], xc[1], xc[2], r);
    int cl = gmsh::model::occ::addCurveLoop({c});
    int s = gmsh::model::occ::addPlaneSurface({cl});
    int p = gmsh::model::occ::addPoint(xc[0], xc[1], xc[2], h);

    gmsh::model::occ::synchronize();
    gmsh::model::mesh::embed(0, {p}, 2, s);
    gmsh::model::mesh::generate(3);
  }

  // Write output files
  gmsh::write(filename + ".msh");
  if (vtk_out) {
    gmsh::write(filename + ".vtk");
  }

  gmsh::finalize();
}

} // namespace mesh_gen 