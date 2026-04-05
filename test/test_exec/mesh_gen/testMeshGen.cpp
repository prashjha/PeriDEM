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
#include <vector>

namespace fs = std::filesystem;

/**
 * @brief Test circle mesh via the same path as other 2D geometries (OCC disk).
 */
bool testCircleMesh() {
  util::io::log("Testing circle mesh generation (gmsh_builtin_mesh / built-in Gmsh path)...\n");

  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double radius = 0.001;
  const double meshSize = radius / 5.0;

  const fs::path outputDir = "test_output/mesh_gen";
  fs::create_directories(outputDir);

  try {
    mesh_gen::generateBuiltinParticleMeshGmsh(
        "circle",
        std::vector<double>{radius, center[0], center[1], center[2]},
        meshSize,
        (outputDir / "circle").string(),
        true,  // vtk
        true,  // write .msh
        nullptr,
        nullptr,
        nullptr);

    const bool mshOk = fs::exists(outputDir / "circle.msh");
    const bool vtkOk = fs::exists(outputDir / "circle.vtk");
    if (!mshOk || !vtkOk) {
      util::io::log("Error: Expected circle.msh and circle.vtk under test_output/mesh_gen.\n");
      if (!mshOk) util::io::log("Missing: circle.msh\n");
      if (!vtkOk) util::io::log("Missing: circle.vtk\n");
      return false;
    }

    util::io::log("Circle mesh generation test passed.\n");
    return true;
  } catch (const std::exception &e) {
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

  // Test circle mesh generation (unified path)
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