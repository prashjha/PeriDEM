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
#include "util/geomObjectsUtil.h"
#include <string>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store mesh related input data */
struct MeshDeck {

  /*! @brief Dimension */
  size_t d_dim;

  /*!
   * @brief Tag for spatial discretization
   *
   * @note Value of this variable is equal to @ModelDeck::d_spatialDiscretization
   *
   * List of allowed values are:
   * - \a finite_difference
   * - \a weak_finite_element
   * - \a nodal_finite_element
   * - \a truss_finite_element
   */
  std::string d_spatialDiscretization;

  /*!
   * @brief Flag to indicate if we should populate element-node connectivity data in meshes
   * @note Value of this variable is equal to @ModelDeck::d_populateElementNodeConnectivity
   */
  bool d_populateElementNodeConnectivity;

  /*!
   * @brief Order of quadrature approximation for strain and stress computation (default is 1)
   * @note Value of this variable is equal to @ModelDeck::d_quadOrder
   */
  size_t d_quadOrder;

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
  util::geometry::GeomData d_createMeshGeomData;

  /*!
   * @brief Constructor
   */
  MeshDeck() : d_dim(0), d_computeMeshSize(false),
               d_h(0.), d_createMesh(false),
               d_createMeshGeomData() {};

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
    oss << tabS << "Dimension = " << d_dim << std::endl;
    oss << tabS << "Populate element-node connectivity data = " << d_populateElementNodeConnectivity << std::endl;
    oss << tabS << "Order of quad approximation = " << d_quadOrder << std::endl;
    oss << tabS << "Spatial discretization type = " << d_spatialDiscretization
        << std::endl;
    oss << tabS << "Filename = " << d_filename << std::endl;
    oss << tabS << "Compute mesh size = " << d_computeMeshSize << std::endl;
    oss << tabS << "Mesh size = " << d_h << std::endl;
    oss << tabS << "Create mesh = " << d_createMesh << std::endl;
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
