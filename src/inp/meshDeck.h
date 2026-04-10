/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_MESHDECK_H
#define INP_MESHDECK_H

#include "util/io.h"
#include "util/json.h"
#include <string>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store mesh related input data */
struct MeshDeck {

  /*! @brief Filename to read mesh data */
  std::string d_filename;

  /*!
   * @brief Target spacing for in-built meshers only (Gmsh uniform / uniform rectangle grid).
   *
   * Read from @c CreateMesh.Mesh_Size in JSON. It drives mesh generation only; the
   * characteristic length stored on @c mesh::Mesh is always computed from the generated
   * nodes (same as for meshes read from file).
   */
  double d_hMeshing;

  /*!
   * @brief Specify if we create mesh using in-built gmsh
   * or in-built routine for uniform discretization of rectangle/cuboid
   */
  bool d_createMesh;

  /*!
   * @brief Selector for in-built mesh creation (e.g. uniform rectangle, gmsh_builtin_mesh
   *        for built-in geometries meshed in-process with Gmsh).
   */
  std::string d_createMeshInfo;

  /*! @brief If true (default), Gmsh-based create-mesh paths write a .msh file; set false for in-memory only. */
  bool d_writeMeshFile;

  /*!
   * @brief Constructor
   */
  MeshDeck(const json &j = json({})) : d_hMeshing(0.), d_createMesh(false), d_writeMeshFile(true) {
    readFromJson(j);
  };

  /*!
   * @brief Constructor for programmatic use (optional meshing size for tests).
   */
  MeshDeck(std::string filename, double h_meshing = -1.)
      : d_filename(std::move(filename)), d_hMeshing(h_meshing > 0. ? h_meshing : 0.),
        d_createMesh(false), d_writeMeshFile(true) {}

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getExampleJson(std::string filename = "", double h_meshing = -1.) {

    auto j = json({});
    if (!filename.empty())
      j["File"] = filename;
    if (filename.empty() && h_meshing > 0.) {
      j["CreateMesh"] = json{{"Flag", true},
                             {"Info", "gmsh_builtin_mesh"},
                             {"Mesh_Size", h_meshing},
                             {"Write_Mesh_File", true}};
    }
    return j;
  }

  /*!
   * @brief Reads from json object
   */
  void readFromJson(const json &j) {
    if (j.empty())
      return;

    d_filename = j.value("File", std::string());
    d_hMeshing = 0.;
    d_createMesh = false;
    d_createMeshInfo = "uniform";
    d_writeMeshFile = true;

    if (j.find("CreateMesh") != j.end()) {
      const auto &cm = j.at("CreateMesh");
      d_createMesh = cm.value("Flag", false);
      d_createMeshInfo = cm.value("Info", std::string("uniform"));
      d_writeMeshFile = cm.value("Write_Mesh_File", true);
      if (cm.find("Mesh_Size") != cm.end())
        d_hMeshing = cm.at("Mesh_Size").get<double>();
      else if (d_createMesh && j.find("Mesh_Size") != j.end())
        d_hMeshing = j.at("Mesh_Size").get<double>();
    }

    if (d_createMesh && d_hMeshing <= 0.)
      throw std::runtime_error(
          "In-built mesh creation requires CreateMesh.Mesh_Size (or legacy top-level Mesh_Size).");

    if (d_filename.empty() && !d_createMesh) {
      throw std::runtime_error("Mesh filename can not be empty unless CreateMesh is enabled.");
    }
  }

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt = 0, int lvl = 0) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- MeshDeck --------" << std::endl << std::endl;
    oss << tabS << "Filename = " << d_filename << std::endl;
    oss << tabS << "Meshing size (in-built only) = " << d_hMeshing << std::endl;
    oss << tabS << "Create mesh = " << d_createMesh << std::endl;
    oss << tabS << "Create mesh info = " << d_createMeshInfo << std::endl;
    oss << tabS << "Write mesh file (Gmsh) = " << d_writeMeshFile << std::endl;
    oss << tabS << "(Built-in mesh geometry comes from Particle.Set_i for the same index.)" << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
};

/** @}*/

} // namespace inp

#endif // INP_MESHDECK_H
