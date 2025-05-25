/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GEN_MESH_GENERATOR_H
#define MESH_GEN_MESH_GENERATOR_H

#include "util/point.h"
#include <gmsh.h>
#include <string>
#include <vector>

namespace mesh_gen {

/**
 * @brief Base class for mesh generation
 */
class MeshGenerator {
public:
  /**
   * @brief Constructor
   * @param debugLevel Debug level (0: no debug, 1: basic debug, 2: verbose debug)
   */
  explicit MeshGenerator(int debugLevel = 0);

  /**
   * @brief Destructor
   */
  virtual ~MeshGenerator();

  /**
   * @brief Initialize Gmsh
   */
  void initialize();

  /**
   * @brief Finalize Gmsh
   */
  void finalize();

  /**
   * @brief Set mesh size
   * @param meshSize Mesh size
   */
  void setMeshSize(double meshSize) { d_meshSize = meshSize; }

  /**
   * @brief Set debug level
   * @param level Debug level (0: no debug, 1: basic debug, 2: verbose debug)
   */
  void setDebugLevel(int level) { d_debugLevel = level; }

  /**
   * @brief Get debug level
   * @return Debug level
   */
  int getDebugLevel() const { return d_debugLevel; }

  /**
   * @brief Get mesh size
   * @return Target mesh size
   */
  double getMeshSize() const { return d_meshSize; }

protected:
  /**
   * @brief Target mesh size
   */
  double d_meshSize;

  /**
   * @brief Debug level (0: no debug, 1: basic debug, 2: verbose debug)
   */
  int d_debugLevel;

  /**
   * @brief Flag indicating if Gmsh is initialized
   */
  bool d_isInitialized;
};

/**
 * @brief Class for generating circular particle mesh
 */
class CircularParticleMeshGenerator : public MeshGenerator {
public:
  /**
   * @brief Constructor
   * 
   * @param center Center coordinates [x, y, z]
   * @param radius Radius of the particle
   * @param meshSize Target mesh size
   * @param tag Particle tag
   * @param debugLevel Debug level (0: no debug, 1: basic debug, 2: verbose debug)
   */
  CircularParticleMeshGenerator(const std::vector<double>& center, 
                               double radius,
                               double meshSize,
                               int tag,
                               int debugLevel = 0);

  /**
   * @brief Generate mesh and save to file
   * 
   * @param filename Output filename without extension
   */
  void generate(const std::string& filename);

private:
  /**
   * @brief Center coordinates
   */
  std::vector<double> d_center;

  /**
   * @brief Radius
   */
  double d_radius;

  /**
   * @brief Particle tag
   */
  int d_tag;
};

} // namespace mesh_gen

#endif // MESH_GEN_MESH_GENERATOR_H 