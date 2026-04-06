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
#include "geom/geomObjects.h"
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
    auto circle =
        std::make_shared<geom::Circle>(radius, util::Point(center[0], center[1], center[2]));
    mesh_gen::generateBuiltinParticleMeshGmsh(circle, meshSize, (outputDir / "circle").string(),
                                              true, true, nullptr, nullptr, nullptr);

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
 * @brief Cylinder mesh: tetra path + every node inside the axis-aligned bounding box from
 *        geom::Cylinder::box() (with tolerance).
 */
bool testCylinderMesh() {
  util::io::log("Testing cylinder mesh generation (OCC cylinder / 3D tets)...\n");

  const double r = 0.001;
  const double L = 0.002;
  const double meshSize = r / 5.0;

  try {
    auto cyl = std::make_shared<geom::Cylinder>(
        r, util::Point(0.0, 0.0, 0.0), util::Point(0.0, 0.0, L));

    inp::MeshDeck meshDeck;
    meshDeck.d_h = meshSize;
    meshDeck.d_filename = "cylinder_test.msh";
    meshDeck.d_computeMeshSize = false;

    const auto modelJson =
        inp::ModelDeck::getExampleJson(3, 0.001, 10, "finite_difference", "central_difference", true, 2,
                                       "Multi_Particle", 0);
    inp::ModelDeck modelDeck(modelJson);

    mesh::Mesh mesh;
    mesh_gen::generateBuiltinParticleMeshGmsh(cyl, meshSize, "", false, false, &mesh, &meshDeck,
                                                &modelDeck);

    const auto box = cyl->box();
    const double tol = 4.0 * meshSize;
    for (const auto &p : mesh.getNodes()) {
      if (p.d_x < box.first.d_x - tol || p.d_y < box.first.d_y - tol || p.d_z < box.first.d_z - tol ||
          p.d_x > box.second.d_x + tol || p.d_y > box.second.d_y + tol || p.d_z > box.second.d_z + tol) {
        util::io::log(std::format(
            "Error: mesh node ({},{},{}) outside Cylinder::box() axis-aligned bounds (tol {}).\n",
            p.d_x, p.d_y, p.d_z, tol));
        return false;
      }
    }

    util::io::log("Cylinder mesh generation and bounding-box check passed.\n");
    return true;
  } catch (const std::exception &e) {
    util::io::log(std::format("Error in cylinder mesh test: {}\n", e.what()));
    return false;
  }
}

/**
 * @brief Ellipse: 2D triangles + nodes inside geom::Ellipse::box().
 */
bool testEllipseMesh() {
  util::io::log("Testing ellipse mesh generation (OCC disk / 2D triangles)...\n");

  const double a = 0.0012;
  const double b = 0.00085;
  const double theta = 0.35;
  const double meshSize = std::min(a, b) / 5.0;

  try {
    auto el = std::make_shared<geom::Ellipse>(a, b, theta, util::Point(0.0, 0.0, 0.0));

    inp::MeshDeck meshDeck;
    meshDeck.d_h = meshSize;
    meshDeck.d_filename = "ellipse_test.msh";
    meshDeck.d_computeMeshSize = false;

    const auto modelJson =
        inp::ModelDeck::getExampleJson(2, 0.001, 10, "finite_difference", "central_difference", true, 2,
                                       "Multi_Particle", 0);
    inp::ModelDeck modelDeck(modelJson);

    mesh::Mesh mesh;
    mesh_gen::generateBuiltinParticleMeshGmsh(el, meshSize, "", false, false, &mesh, &meshDeck,
                                              &modelDeck);

    const auto box = el->box();
    const double tol = 4.0 * meshSize;
    for (const auto &p : mesh.getNodes()) {
      if (p.d_x < box.first.d_x - tol || p.d_y < box.first.d_y - tol || p.d_z < box.first.d_z - tol ||
          p.d_x > box.second.d_x + tol || p.d_y > box.second.d_y + tol || p.d_z > box.second.d_z + tol) {
        util::io::log(std::format(
            "Error: ellipse mesh node ({},{},{}) outside Ellipse::box() bounds (tol {}).\n", p.d_x,
            p.d_y, p.d_z, tol));
        return false;
      }
    }

    util::io::log("Ellipse mesh generation and bounding-box check passed.\n");
    return true;
  } catch (const std::exception &e) {
    util::io::log(std::format("Error in ellipse mesh test: {}\n", e.what()));
    return false;
  }
}

/**
 * @brief Ellipsoid: 3D tets + nodes inside geom::Ellipsoid::box().
 */
bool testEllipsoidMesh() {
  util::io::log("Testing ellipsoid mesh generation (OCC unit sphere + affine / 3D tets)...\n");

  const double a = 0.001;
  const double b = 0.0009;
  const double c = 0.00085;
  const double meshSize = std::min(a, std::min(b, c)) / 5.0;

  try {
    auto e =
        std::make_shared<geom::Ellipsoid>(a, b, c, util::Point(0.0, 0.0, 0.0));

    inp::MeshDeck meshDeck;
    meshDeck.d_h = meshSize;
    meshDeck.d_filename = "ellipsoid_test.msh";
    meshDeck.d_computeMeshSize = false;

    const auto modelJson =
        inp::ModelDeck::getExampleJson(3, 0.001, 10, "finite_difference", "central_difference", true, 2,
                                       "Multi_Particle", 0);
    inp::ModelDeck modelDeck(modelJson);

    mesh::Mesh mesh;
    mesh_gen::generateBuiltinParticleMeshGmsh(e, meshSize, "", false, false, &mesh, &meshDeck,
                                              &modelDeck);

    const auto box = e->box();
    const double tol = 4.0 * meshSize;
    for (const auto &p : mesh.getNodes()) {
      if (p.d_x < box.first.d_x - tol || p.d_y < box.first.d_y - tol || p.d_z < box.first.d_z - tol ||
          p.d_x > box.second.d_x + tol || p.d_y > box.second.d_y + tol || p.d_z > box.second.d_z + tol) {
        util::io::log(std::format(
            "Error: ellipsoid mesh node ({},{},{}) outside Ellipsoid::box() bounds (tol {}).\n",
            p.d_x, p.d_y, p.d_z, tol));
        return false;
      }
    }

    util::io::log("Ellipsoid mesh generation and bounding-box check passed.\n");
    return true;
  } catch (const std::exception &e) {
    util::io::log(std::format("Error in ellipsoid mesh test: {}\n", e.what()));
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

  if (!testEllipseMesh()) {
    util::io::log("Ellipse mesh tests failed.\n");
    allTestsPassed = false;
  }

  if (!testCylinderMesh()) {
    util::io::log("Cylinder mesh tests failed.\n");
    allTestsPassed = false;
  }

  if (!testEllipsoidMesh()) {
    util::io::log("Ellipsoid mesh tests failed.\n");
    allTestsPassed = false;
  }

  if (allTestsPassed) {
    util::io::log("All mesh generator tests passed.\n");
    return 0;
  } else {
    util::io::log("Some mesh generator tests failed.\n");
    return 1;
  }
} 