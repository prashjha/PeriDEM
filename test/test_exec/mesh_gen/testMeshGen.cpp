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
#include "util/io.h"
#include <filesystem>
#include <format>

namespace fs = std::filesystem;

/**
 * @brief Test circular particle mesh generation
 */
bool testCircularParticleMesh() {
  util::io::log("Testing circular particle mesh generation...\n");

  // Test parameters
  std::vector<double> center = {0.0, 0.0};
  double radius = 0.001;  // 1mm radius
  double meshSize = radius / 5.0;
  int tag = 1;
  int debugLevel = 2;  // Set to maximum debug level

  // Create output directory if it doesn't exist
  fs::path outputDir = "test_output/mesh_gen";
  fs::create_directories(outputDir);

  // Create mesh generator with debug level 2 (verbose)
  mesh_gen::CircularParticleMeshGenerator generator(center, radius, meshSize, tag, debugLevel);

  try {
    // Generate mesh
    generator.generate((outputDir / "circle").string());

    // Check if files were created
    bool mshExists = fs::exists(outputDir / "circle.msh");
    bool vtkExists = fs::exists(outputDir / "circle.vtk");

    if (!mshExists || !vtkExists) {
      util::io::log("Error: Mesh files were not created.\n");
      return false;
    }

    util::io::log("Circular particle mesh generation test passed.\n");
    return true;
  } catch (const std::exception& e) {
    util::io::log(std::format("Error in mesh generation: {}\n", e.what()));
    return false;
  }
}

/**
 * @brief Main test function
 */
int main() {
  util::io::log("Starting mesh generator tests...\n");

  bool allTestsPassed = true;

  // Test circular particle mesh generation
  if (!testCircularParticleMesh()) {
    util::io::log("Circular particle mesh test failed.\n");
    allTestsPassed = false;
  }

  // Add more tests here as we add more geometry types

  if (allTestsPassed) {
    util::io::log("All mesh generator tests passed.\n");
    return 0;
  } else {
    util::io::log("Some mesh generator tests failed.\n");
    return 1;
  }
} 