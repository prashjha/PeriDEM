/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef RW_VTK_PARTICLE_WRITER_H
#define RW_VTK_PARTICLE_WRITER_H

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>

// forward declaration
namespace model {
class ModelData;
}

namespace rw {

namespace writer {

/*! @brief A vtk writer for simple point data and complex fem mesh data */
class VtkParticleWriter {

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
  explicit VtkParticleWriter(const std::string &filename, const std::string
  &compress_type = "");

  /**
   * @name Mesh data
   */
  /**@{*/

  /*!
   * @brief Writes the nodes to the file
   * @param particles Particle data for each particle
   * @param walls Wall data for each wall
   */
  void appendNodes(const model::ModelData *model,
                   const std::vector<std::string> &tags);

  /*!
   * @brief Writes the nodes to the file
   * @param particles Particle data for each particle
   * @param walls Wall data for each wall
   */
  void
  appendMesh(const model::ModelData *model,
             const std::vector<std::string> &tags);

  /*!
   * @brief Prepares contact data that is set of nodes in contact and
   * line-element connecting two contacting nodes
   */
  void appendContactData(const model::ModelData *model,
                         const std::vector<size_t> *processed_nodes,
                         const std::vector <
                             std::pair<size_t, size_t>> *processed_elems);

  /*!
   * @brief Writes the nodes to the file
   * @param particles Particle data for each particle
   * @param walls Wall data for each wall
   */
  void appendStrainStress(const model::ModelData *model) {};

  /** @}*/

  /*!
   * @brief Writes the time step to the file
   * @param timestep Current time step of the simulation
   */
  void addTimeStep(const double &timestep);

  /*!
   * @brief Closes the file and store it to the hard disk
   */
  void close();

private:
  /*! @brief XML unstructured grid writer */
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> d_writer_p;

  /*! @brief Unstructured grid */
  vtkSmartPointer<vtkUnstructuredGrid> d_grid_p;

  /*! @brief compression_type Specify the compressor (if any) */
  std::string d_compressType;
};

} // namespace writer

} // namespace rw

#endif // RW_VTK_PARTICLE_WRITER_H
