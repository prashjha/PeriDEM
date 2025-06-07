/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GENERATOR_H
#define MESH_GENERATOR_H

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <gmsh.h>

namespace mesh_gen {

/**
 * @brief Get all GMSH entities in the current model
 * @return Vector of pairs (dimension, tag) for each entity
 */
std::vector<std::pair<int, int>> getGmshEntities();

/**
 * @brief Transform mesh entities by applying a scaling transformation
 * @param m Vector of entity pairs (dimension, tag) to transform
 * @param offset_entity Offset for new entity tags
 * @param offset_node Offset for new node tags
 * @param offset_element Offset for new element tags
 * @param tx X-axis scale factor
 * @param ty Y-axis scale factor
 * @param tz Z-axis scale factor
 */
void gmshTransform(const std::vector<std::pair<int, int>>& m, int offset_entity, int offset_node, int offset_element, double tx, double ty, double tz);

/**
 * @brief Translate mesh by a vector
 * @param xc Translation vector [x, y, z]
 */
void gmshTranslate(const std::vector<double>& xc);

/**
 * @brief Generate a circular mesh with optional symmetry
 * @param xc Center coordinates [x, y, z]
 * @param r Radius of the circle
 * @param h Mesh size
 * @param filename Output filename (without extension)
 * @param vtk_out Whether to output VTK file
 * @param symmetric_mesh If true, creates 1/4 mesh and mirrors it. If false, creates full circle
 */
void circleMeshSymmetric(const std::vector<double>& xc, double r, double h, const std::string& filename, bool vtk_out = false, bool symmetric_mesh = true);

} // namespace mesh_gen

#endif // MESH_GENERATOR_H 