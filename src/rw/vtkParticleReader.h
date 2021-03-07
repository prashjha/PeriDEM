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
   */
  explicit VtkParticleReader(const std::string &filename);

  /**
   * @name Mesh data
   */
  /**@{*/

  /*!
   * @brief Writes the nodes to the file
   * @param particles Particle data for each particle
   */
  void readNodes(model::ModelData *model);

  /*!
   * @brief Closes the file and store it to the hard disk
   */
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
