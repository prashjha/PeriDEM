/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef RW_LEGACY_VTKWRITER_H
#define RW_LEGACY_VTKWRITER_H

#include "util/matrix.h" // definition of SymMatrix3
#include "util/point.h"  // definition of Point
#include <fstream>
#include <vector>

namespace rw {

namespace writer {

/*! @brief A vtk writer for simple point data and complex fem mesh data */
class LegacyVtkWriter {

public:
  /*!
   * @brief Constructor
   *
   * Creates and opens .vtu file of name given by filename. The file remains
   * open till the close() function is invoked.
   *
   * @param filename Name of file which will be created
   * @param compress_type Compression method (optional)
   */
  explicit LegacyVtkWriter(const std::string &filename, const std::string &compress_type = "");

  /**
   * @name Mesh data
   */
  /**@{*/

  /*!
   * @brief Writes the nodes to the file
   * @param nodes Current positions of the nodes
   */
  void appendNodes(const std::vector<util::Point> *nodes);

  /*!
   * @brief Writes the nodes to the file
   * @param nodes Reference positions of the nodes
   * @param u Nodal displacements
   */
  void appendNodes(const std::vector<util::Point> *nodes,
                   const std::vector<util::Point> *u);

  /*!
   * @brief Writes the mesh data to file
   *
   * @param nodes Vector of nodal coordinates
   * @param element_type Type of element
   * @param en_con Vector of element-node connectivity
   * @param u Vector of nodal displacement
   */
  void appendMesh(const std::vector<util::Point> *nodes,
                  const size_t &element_type,
                  const std::vector<size_t> *en_con,
                  const std::vector<util::Point> *u = nullptr);

  /** @}*/

  /**
   * @name Point data
   */
  /**@{*/

  /*!
   * @brief Writes the scalar point data to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendPointData(const std::string &name,
                       const std::vector<uint8_t> *data);

  /*!
   * @brief Writes the scalar point data to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendPointData(const std::string &name,
                       const std::vector<size_t> *data);

  /*!
   * @brief Writes the scalar point data to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendPointData(const std::string &name, const std::vector<int> *data);

  /*!
   * @brief Writes the scalar point data to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendPointData(const std::string &name, const std::vector<float> *data);

  /*!
   * @brief Writes the scalar point data to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendPointData(const std::string &name,
                       const std::vector<double> *data);

  /*!
   * @brief Writes the vector point data to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendPointData(const std::string &name,
                       const std::vector<util::Point> *data);

  /*!
   * @brief Writes the symmetric matrix data associated to nodes to the
   * file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendPointData(const std::string &name,
                       const std::vector<util::SymMatrix3> *data);

  /** @}*/

  /**
   * @name Cell data
   */
  /**@{*/

  /*!
   * @brief Writes the float data associated to cells to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendCellData(const std::string &name, const std::vector<float> *data);

  /*!
   * @brief Writes the symmetric matrix data associated to cells to the file
   * @param name Name of the data
   * @param data Vector containing the data
   */
  void appendCellData(const std::string &name,
                       const std::vector<util::SymMatrix3> *data);

  /** @}*/

  /**
   * @name Field data
   */
  /**@{*/

  /*!
   * @brief Writes the scalar field data to the file
   * @param name Name of the data
   * @param data Value
   */
  void appendFieldData(const std::string &name, const double &data);

  /*!
   * @brief Writes the scalar field data to the file
   * @param name Name of the data
   * @param data Value
   */
  void appendFieldData(const std::string &name, const float &data);

  /*!
   * @brief Writes the time step to the file
   * @param timestep Current time step of the simulation
   */
  void addTimeStep(const double &timestep);

  /** @}*/

  /*!
   * @brief Closes the file and store it to the hard disk
   */
  void close();

private:
  /*! @brief filename */
  std::string d_filename;

  /*! @brief compression_type Specify the compressor (if any) */
  std::string d_compressType;

  /*! @brief vtk/vtu file */
  std::ofstream d_file;
};

} // namespace writer

} // namespace rw

#endif // RW_LEGACY_VTKWRITER_H
