/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef RW_VTK_PARTICLE_READER_H
#define RW_VTK_PARTICLE_READER_H

#include <string>

// if clion does not load vtk header files, invalidate cache in File menu following https://stackoverflow.com/a/78215443
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>

// forward declaration
namespace model {
class ModelData;
}

namespace rw {

namespace reader {

/*! @brief A vtk writer for simple point data and complex fem mesh data */
class VtkParticleReader {

public:
  /*!
   * @brief Constructor
   * @param filename Name of the vtk file (with/without .vtu extension)
   */
  explicit VtkParticleReader(const std::string &filename);

  /**
   * @name Mesh data
   */
  /**@{*/

  /*!
   * @brief Writes the nodes to the file
   * @param model ModelData class object
   */
  void readNodes(model::ModelData *model);

  /*! @brief Closes the file and store it to the hard disk */
  void close();

private:
  /*! @brief XML unstructured grid writer */
  vtkSmartPointer<vtkXMLUnstructuredGridReader> d_reader_p;

  /*! @brief Unstructured grid */
  vtkSmartPointer<vtkUnstructuredGrid> d_grid_p;
};

} // namespace reader

} // namespace rw

#endif // RW_VTK_PARTICLE_READER_H
