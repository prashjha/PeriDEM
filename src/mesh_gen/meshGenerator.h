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
#include <gmsh.h>
#include <stdexcept>

namespace inp {
struct MeshDeck;
struct ModelDeck;
}
namespace mesh {
class Mesh;
}

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
 * @param write_mesh_file If true, write filename.msh (requires non-empty filename)
 * @param out_mesh If non-null, fill this mesh from the active Gmsh model before finalize (required when write_mesh_file is false)
 * @param meshDeck_for_out Required when out_mesh is set
 * @param modelDeck_for_out Required when out_mesh is set
 */
void circleMeshSymmetric(const std::vector<double>& xc, double r, double h, const std::string& filename,
                         bool vtk_out = false, bool symmetric_mesh = true, bool write_mesh_file = true,
                         mesh::Mesh* out_mesh = nullptr,
                         const inp::MeshDeck* meshDeck_for_out = nullptr,
                         const inp::ModelDeck* modelDeck_for_out = nullptr);

/**
 * @brief Gmsh symmetric/in-process mesh for any built-in particle geometry in geom::acceptable_geometries.
 *
 * Uses the same mesh size h and file/VTK behavior as circleMeshSymmetric. Circle delegates to
 * circleMeshSymmetric; 2D shapes use a planar geo mesh; sphere/cube/cuboid use OCC volume meshing.
 *
 * CreateMesh.Info should be "gmsh_symmetric_mesh" (or legacy "gmsh_circle_symmetric" for circles).
 */
void generateSymmetricParticleMeshGmsh(const std::string& geomName,
                                       const std::vector<double>& params, double h,
                                       const std::string& filenameStem, bool vtk_out, bool write_mesh_file,
                                       mesh::Mesh* out_mesh, const inp::MeshDeck* meshDeck,
                                       const inp::ModelDeck* modelDeck);

} // namespace mesh_gen

#endif // MESH_GENERATOR_H 