/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_MESHDECK_H
#define INP_MESHDECK_H

#include "deckField.h"
#include "util/io.h"
#include <stdexcept>
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
   * @brief Axis-aligned boxes emptied from an in-built uniform grid
   * (`CreateMesh.Void_Regions`: `[xlo,ylo,zlo,xhi,yhi,zhi]`).
   */
  std::vector<std::vector<double>> d_voidRegions;

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
  static json getExampleJson(std::string filename = "", double h_meshing = -1.,
                             bool createMesh = false,
                             std::string createMeshInfo = "gmsh_builtin_mesh",
                             bool writeMeshFile = true,
                             std::vector<std::vector<double>> voidRegions =
                                 std::vector<std::vector<double>>()) {

    auto j = json({});
    if (!filename.empty())
      j["File"] = filename;

    // A mesh size with no filename requests generation. This call shape is
    // kept for the existing callers. The createMesh argument lets a set both
    // generate a mesh and name the file it is written to or read back from.
    const bool create = createMesh || (filename.empty() && h_meshing > 0.);
    if (create) {
      if (!(h_meshing > 0.))
        throw std::runtime_error(
            "MeshDeck::getExampleJson: mesh creation needs a positive "
            "Mesh_Size.");
      auto cm = applyGiven(json{{"Flag", true},
                                {"Info", createMeshInfo},
                                {"Mesh_Size", h_meshing},
                                {"Write_Mesh_File", writeMeshFile}},
                           createMeshFields(), {"Void_Regions"});
      for (const auto &r : voidRegions) {
        if (r.size() != 6)
          throw std::runtime_error(
              "MeshDeck::getExampleJson: each Void_Regions entry needs 6 "
              "values [xlo, ylo, zlo, xhi, yhi, zhi].");
      }
      if (!voidRegions.empty())
        cm["Void_Regions"] = voidRegions;
      j["CreateMesh"] = cm;
    } else if (filename.empty()) {
      throw std::runtime_error(
          "MeshDeck::getExampleJson: a mesh set needs a File or a Mesh_Size.");
    }
    return j;
  }

  /*!
   * @brief Reads from json object
   */
  /*!
   * @brief The fields of the block itself
   * @return fields The field table
   */
  static const std::vector<Field<MeshDeck>> &fields() {
    static const std::vector<Field<MeshDeck>> f = {
        field(&MeshDeck::d_filename, "File", std::string(),
              "Mesh file read, or written when one is generated"),
    };
    return f;
  }

  /*!
   * @brief The fields of the CreateMesh sub-block
   *
   * The sub-block has a table of its own. Its members are members of this
   * deck, so the two tables write into the same object.
   *
   * @return fields The field table
   */
  static const std::vector<Field<MeshDeck>> &createMeshFields() {
    static const std::vector<Field<MeshDeck>> f = {
        field(&MeshDeck::d_createMesh, "Flag", false,
              "Generate the mesh rather than read it"),
        field(&MeshDeck::d_createMeshInfo, "Info", std::string("uniform"),
              "Mesh generator used",
              {{"uniform", "gmsh_builtin_mesh"}, {}, {}}),
        field(&MeshDeck::d_hMeshing, "Mesh_Size", 0.,
              "Element size the generator is asked for"),
        field(&MeshDeck::d_writeMeshFile, "Write_Mesh_File", true,
              "Write the generated mesh to File"),
    };
    return f;
  }

  void readFromJson(const json &j) {
    if (j.empty())
      return;

    readFields(*this, j, fields());
    // Defaults of the sub-block apply whether or not it is present.
    readFields(*this, json::object(), createMeshFields());

    if (j.find("CreateMesh") != j.end()) {
      const auto &cm = j.at("CreateMesh");
      readFields(*this, cm, createMeshFields());

      // A mesh size written beside the block rather than inside it.
      if (cm.find("Mesh_Size") == cm.end() && d_createMesh &&
          j.find("Mesh_Size") != j.end())
        d_hMeshing = j.at("Mesh_Size").get<double>();

      d_voidRegions.clear();
      if (cm.find("Void_Regions") != cm.end()) {
        for (const auto &r : cm.at("Void_Regions")) {
          auto box = r.get<std::vector<double>>();
          if (box.size() != 6)
            throw std::runtime_error(
                "CreateMesh.Void_Regions entries need 6 values [xlo,ylo,zlo,xhi,yhi,zhi].");
          d_voidRegions.push_back(std::move(box));
        }
      }
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
    printFields(*this, oss, fields(), tabS);
    printFields(*this, oss, createMeshFields(), tabS + "  ");
    oss << tabS << "Void_Regions count = " << d_voidRegions.size() << std::endl;
    oss << tabS << "The geometry of a generated mesh is Particle.Set_i at the "
        << "same index." << std::endl;
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
