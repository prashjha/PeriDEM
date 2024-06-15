/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef RW_VTKREADER_H
#define RW_VTKREADER_H

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>

#include "util/point.h"           // definition of Point
#include "util/matrix.h"           // definition of matrices
#include <cstdint> // uint8_t type
#include <cstring> // string and size_t type

namespace rw {

namespace reader {

/*!
 * @brief A class to read VTK (.vtu) mesh files
 *
 * @note Depends on VTK library.
 */
class VtkReader {

public:
  /*!
   * @brief Constructor
   * @param filename Name of mesh file (with/without .vtu extension)
   */
  explicit VtkReader(const std::string &filename);

  /*!
   * @brief Reads mesh data into node file and element file
   * @param dim Dimension
   * @param nodes Vector of nodes data
   * @param element_type Type of element
   * @param num_elems Number of elements
   * @param enc Vector holding element-node connectivity
   * @param nec Vector holding node-element connectivity
   * @param volumes Vector holding volume of the nodes
   * @param is_fd Flag indicating if this mesh is for finite_difference
   * simulation
   */
  void readMesh(size_t dim, std::vector<util::Point> *nodes,
                size_t &element_type, size_t &num_elems,
                std::vector<size_t> *enc, std::vector<std::vector<size_t>> *nec,
                std::vector<double> *volumes, bool is_fd = false);

  /*!
   * @brief Reads nodal position
   *
   * @param nodes Vector of nodal coordinates
   */
  void readNodes(std::vector<util::Point> *nodes);

  /*!
   * @brief Reads cell data, i.e. element-node connectivity and node-element connectivity
   * @param dim Dimension
   * @param element_type Type of element
   * @param num_elems Number of elements
   * @param enc Element-node connectivity
   * @param nec Node-element connectivity
   */
  void readCells(size_t dim, size_t &element_type,
                 size_t &num_elems, std::vector<size_t> *enc,
                 std::vector<std::vector<size_t>> *nec);

  /*!
   * @brief reads point data from .vtu file
   * @param name Name of data
   * @param data Pointer to the vector of data
   * @return status True if data is found otherwise false
   */
  bool readPointData(const std::string &name, std::vector<uint8_t> *data);

  /*! @copydoc readPointData(const std::string &name, std::vector<uint8_t> *data) */
  bool readPointData(const std::string &name, std::vector<size_t> *data);

  /*! @copydoc readPointData(const std::string &name, std::vector<uint8_t> *data) */
  bool readPointData(const std::string &name, std::vector<int> *data);

  /*! @copydoc readPointData(const std::string &name, std::vector<uint8_t> *data) */
  bool readPointData(const std::string &name, std::vector<float> *data);

  /*! @copydoc readPointData(const std::string &name, std::vector<uint8_t> *data) */
  bool readPointData(const std::string &name, std::vector<double> *data);

  /*! @copydoc readPointData(const std::string &name, std::vector<uint8_t> *data) */
  bool readPointData(const std::string &name, std::vector<util::Point> *data);

  /*! @copydoc readPointData(const std::string &name, std::vector<uint8_t> *data) */
  bool readPointData(const std::string &name, std::vector<util::SymMatrix3> *data);

  /*! @copydoc readPointData(const std::string &name, std::vector<uint8_t> *data) */
  bool readPointData(const std::string &name,
                     std::vector<util::Matrix3> *data);

  /*!
   * @brief reads cell data from .vtu file
   * @param name Name of data
   * @param data Pointer to the vector of data
   * @return status True if data is found otherwise false
   */
  bool readCellData(const std::string &name, std::vector<float> *data);

  /*! @copydoc readCellData(const std::string &name, std::vector<float> *data) */
  bool readCellData(const std::string &name, std::vector<double> *data);

  /*! @copydoc readCellData(const std::string &name, std::vector<float> *data) */
  bool readCellData(const std::string &name, std::vector<util::Point> *data);

  /*! @copydoc readCellData(const std::string &name, std::vector<float> *data) */
  bool readCellData(const std::string &name, std::vector<util::SymMatrix3> *data);

  /*! @copydoc readCellData(const std::string &name, std::vector<float> *data) */
  bool readCellData(const std::string &name, std::vector<util::Matrix3> *data);

  /*! @brief Close the file */
  void close();

private:

  /*! @brief Counter */
  static size_t d_count;

  /*! @brief XML unstructured grid writer */
  vtkSmartPointer<vtkXMLUnstructuredGridReader> d_reader_p;

  /*! @brief Unstructured grid */
  vtkSmartPointer<vtkUnstructuredGrid> d_grid_p;
};

} // namespace reader

} // namespace rw

#endif // RW_VTKREADER_H
