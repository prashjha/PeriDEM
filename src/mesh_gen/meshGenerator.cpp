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
#include <format>
#include <stdexcept>

namespace mesh_gen {

MeshGenerator::MeshGenerator(int debugLevel) 
    : d_meshSize(0.0), d_debugLevel(debugLevel), d_isInitialized(false) {}

MeshGenerator::~MeshGenerator() {
  if (d_isInitialized)
    finalize();
}

void MeshGenerator::initialize() {
  if (!d_isInitialized) {
    gmsh::initialize();
    
    // Set debug options based on debug level
    if (d_debugLevel > 0) {
      gmsh::option::setNumber("General.Terminal", 1);
      if (d_debugLevel > 1) {
        gmsh::option::setNumber("General.Verbosity", 99);
      } else {
        gmsh::option::setNumber("General.Verbosity", 5);
      }
    }
    
    // Set mesh file version to 2.2
    gmsh::option::setNumber("Mesh.MshFileVersion", 2.2);
    
    d_isInitialized = true;
  }
}

void MeshGenerator::finalize() {
  // Only finalize if we initialized
  if (d_isInitialized) {
    try {
      gmsh::finalize();
      d_isInitialized = false;
    } catch (const std::runtime_error& e) {
      // If Gmsh is already finalized, just update our flag
      d_isInitialized = false;
    }
  }
}

CircularParticleMeshGenerator::CircularParticleMeshGenerator(
    const std::vector<double>& center, double radius, double meshSize, int tag, int debugLevel)
    : MeshGenerator(debugLevel), d_center(center), d_radius(radius), d_tag(tag) {
  setMeshSize(meshSize);
}

void CircularParticleMeshGenerator::generate(const std::string& filename) {
  // Initialize if needed
  if (!d_isInitialized)
    initialize();

  // Create a new model
  gmsh::model::add(std::format("particle_{}", d_tag));

  // Create circle
  double cx = d_center[0];
  double cy = d_center[1];
  
  // Center point
  int centerPoint = gmsh::model::geo::addPoint(cx, cy, 0, d_meshSize);
  
  // Points on circle
  int p1 = gmsh::model::geo::addPoint(cx + d_radius, cy, 0, d_meshSize);
  int p2 = gmsh::model::geo::addPoint(cx, cy + d_radius, 0, d_meshSize);
  int p3 = gmsh::model::geo::addPoint(cx - d_radius, cy, 0, d_meshSize);
  int p4 = gmsh::model::geo::addPoint(cx, cy - d_radius, 0, d_meshSize);

  // Create circle arcs
  int c1 = gmsh::model::geo::addCircleArc(p1, centerPoint, p2);
  int c2 = gmsh::model::geo::addCircleArc(p2, centerPoint, p3);
  int c3 = gmsh::model::geo::addCircleArc(p3, centerPoint, p4);
  int c4 = gmsh::model::geo::addCircleArc(p4, centerPoint, p1);

  // Create curve loop
  gmsh::model::geo::addCurveLoop({c1, c2, c3, c4}, 1);

  // Create surface
  gmsh::model::geo::addPlaneSurface({1}, 1);

  // First synchronize to create the surface
  gmsh::model::geo::synchronize();

  if (d_debugLevel > 0) {
    // Print debug info about entities
    std::vector<std::pair<int, int>> surfaces;
    gmsh::model::getEntities(surfaces, 2);
    std::cout << "Number of surfaces: " << surfaces.size() << std::endl;
    for (const auto& s : surfaces) {
      std::cout << "Surface: (" << s.first << ", " << s.second << ")" << std::endl;
    }
  }

  // Now embed the center point in the surface
  std::vector<int> pointTags = {centerPoint};
  gmsh::model::mesh::embed(0, pointTags, 2, 1);

  // Generate 2D mesh
  gmsh::model::mesh::generate(2);

  // Save to file
  gmsh::write(filename + ".msh");
  gmsh::write(filename + ".vtk");

  // Clear the current model but don't finalize Gmsh
  gmsh::model::remove();
}

} // namespace mesh_gen 