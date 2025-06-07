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
#include "geom/geomIncludes.h"
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

  /*! @brief Flag which indicates if mesh size is to be computed */
  bool d_computeMeshSize;

  /*! @brief Mesh size */
  double d_h;

  /*!
   * @brief Specify if we create mesh using in-built gmsh
   * or in-built routine for uniform discretization of rectangle/cuboid
   */
  bool d_createMesh;

  /*!
   * @brief Information that will be used when creating a mesh using in-built routines
   */
  std::string d_createMeshInfo;

  /*!
   * @brief Information that will be used when creating a mesh using in-built routines
   */
  geom::GeomData d_createMeshGeomData;

  /*! @brief Use particle geometry in 'Particle' json block to create mesh? */
  bool d_useParticleGeomToCreateMesh;

  /*!
   * @brief Constructor
   */
  MeshDeck(const json &j = json({})) : d_computeMeshSize(false),
               d_h(0.), d_createMesh(false), d_useParticleGeomToCreateMesh(false) {
    readFromJson(j);
  };

  /*!
   * @brief Constructor
   */
  MeshDeck(std::string filename, double h = -1.)
    : d_createMesh(false), d_filename(filename), d_useParticleGeomToCreateMesh(false) {

    if (h <= 0)
      d_computeMeshSize = true;
    else {
      d_h = h;
      d_computeMeshSize = false;
    }
  };

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getExampleJson(std::string filename = "", double h = -1.) {

    auto j = json({});
    if (!filename.empty()) j["File"] = filename;
    if (h > 0)
      j["Mesh_Size"] = h;

    // TODO Add create mesh block

    return j;
  }

  /*!
   * @brief Reads from json object
   */
  void readFromJson(const json &j) {
    if (j.empty())
      return;

    d_filename = j.value("File", std::string());
    if (j.find("Mesh_Size") != j.end()) {
      d_computeMeshSize = false;
      d_h = j.at("Mesh_Size").get<double>();
    } else {
      d_computeMeshSize = true;
    }

    if (d_filename.empty()) {
      if (j.find("CreateMesh") != j.end()) {
        d_createMesh = j.at("CreateMesh").value("Flag", false);
        d_createMeshInfo = j.at("CreateMesh").value("Info", std::string("uniform"));

        // geometry?
        if (j.find("Geometry") != j.end()) {
          geom::readGeometry(j.at("Geometry"), d_createMeshGeomData);
          // create a geometry object based on the data
          geom::createGeomObject(d_createMeshGeomData);
        } else {
          // create mesh using particle geometry
          d_useParticleGeomToCreateMesh = true;
        }

        // mesh size is needed
        if (j.find("Mesh_Size") == j.end())
          throw std::runtime_error("Need Mesh_Size to create mesh using inbuilt function.");
      }
    }

    if (d_filename.empty()) {
      throw std::runtime_error("Mesh filename = " + d_filename + " can not be empty.");
      return;
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
    oss << tabS << "Compute mesh size = " << d_computeMeshSize << std::endl;
    oss << tabS << "Mesh size = " << d_h << std::endl;
    oss << tabS << "Create mesh = " << d_createMesh << std::endl;
    oss << tabS << "Create mesh using particle geometry in Particle block? = " << d_useParticleGeomToCreateMesh << std::endl;
    oss << tabS << "Create mesh info = " << d_createMeshInfo << std::endl;
    oss << tabS << "Create mesh geometry details: " << std::endl;
    oss << d_createMeshGeomData.printStr(nt+1, lvl);
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
