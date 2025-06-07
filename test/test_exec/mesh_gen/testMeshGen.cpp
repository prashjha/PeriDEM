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
 * @brief Test circle mesh generation with symmetric and non-symmetric options
 */
bool testCircleMesh() {
  util::io::log("Testing circle mesh generation...\n");

  // Test parameters
  std::vector<double> center = {0.0, 0.0, 0.0};  // 3D coordinates
  double radius = 0.001;  // 1mm radius
  double meshSize = radius / 5.0;

  // Create output directory if it doesn't exist
  fs::path outputDir = "test_output/mesh_gen";
  fs::create_directories(outputDir);

  try {
    // Test symmetric mesh generation
    util::io::log("Testing symmetric circle mesh...\n");
    mesh_gen::circleMeshSymmetric(
        center,
        radius,
        meshSize,
        (outputDir / "circle_symmetric").string(),
        true,   // output vtk
        true    // symmetric mesh
    );

    // Test non-symmetric mesh generation
    util::io::log("Testing non-symmetric circle mesh...\n");
    mesh_gen::circleMeshSymmetric(
        center,
        radius,
        meshSize,
        (outputDir / "circle_full").string(),
        true,    // output vtk
        false    // non-symmetric mesh
    );

    // Check if files were created
    bool symMshExists = fs::exists(outputDir / "circle_symmetric.msh");
    bool symVtkExists = fs::exists(outputDir / "circle_symmetric.vtk");
    bool fullMshExists = fs::exists(outputDir / "circle_full.msh");
    bool fullVtkExists = fs::exists(outputDir / "circle_full.vtk");

    if (!symMshExists || !symVtkExists || !fullMshExists || !fullVtkExists) {
      util::io::log("Error: Some mesh files were not created.\n");
      if (!symMshExists) util::io::log("Missing: circle_symmetric.msh\n");
      if (!symVtkExists) util::io::log("Missing: circle_symmetric.vtk\n");
      if (!fullMshExists) util::io::log("Missing: circle_full.msh\n");
      if (!fullVtkExists) util::io::log("Missing: circle_full.vtk\n");
      return false;
    }

    util::io::log("Circle mesh generation tests passed.\n");
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

  // Test circle mesh generation (both symmetric and non-symmetric)
  if (!testCircleMesh()) {
    util::io::log("Circle mesh tests failed.\n");
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