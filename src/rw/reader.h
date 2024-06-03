/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef RW_READER_H
#define RW_READER_H

#include "util/point.h"           // definition of Point
#include "util/matrix.h"           // definition of matrices
#include <vector>

/*!
 * @brief Collection of methods and database related to reading and writing
 *
 * This namespace provides methods and data members specific to reading and
 * writing of the mesh data and simulation data.
 */
namespace rw {

/*!
 * @brief Collection of methods and database related to reading
 *
 * This namespace provides methods and data members specific to reading of
 * the mesh data. Currently, .csv, .vtu and .msh files are supported.
 */
namespace reader {

/**
  * @name CSV specific functions
  */
/**@{*/

/*!
 * @brief Reads mesh data into node file and element file
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param nodes Vector of nodes data
 * @param volumes Vector holding volume of the nodes
 */
void readCsvFile(const std::string &filename, size_t dim,
                 std::vector<util::Point> *nodes,
                 std::vector<double> *volumes);

/*!
 * @brief Reads particles center location, radius, and zone id
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param nodes Vector of center locations
 * @param rads Vector of radius
 * @param zones Vector zone ids
 */
void readParticleCsvFile(const std::string &filename, size_t dim,
                 std::vector<util::Point> *nodes,
                 std::vector<double> *rads,
                 std::vector<size_t> *zones);

/*!
 * @brief Reads particles center location, radius, and zone id
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param nodes Vector of center locations
 * @param rads Vector of radius
 * @param zone Only reads particle with this zone
 */
void readParticleCsvFile(const std::string &filename, size_t dim,
                         std::vector<util::Point> *nodes,
                         std::vector<double> *rads,
                         const size_t &zone);

/*!
 * @brief Reads particles center location, radius, and zone id. In this case, file also provides initial orientation of particles.
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param nodes Vector of center locations
 * @param rads Vector of radius
 * @param orients Vector of orientation of particles
 * @param zone Only reads particle with this zone
 */
void readParticleWithOrientCsvFile(const std::string &filename, size_t dim,
                         std::vector<util::Point> *nodes,
                         std::vector<double> *rads,
                         std::vector<double> *orients,
                         const size_t &zone);

/** @}*/

/**
 * @name VTU specific functions
 */
/**@{*/

/*!
 * @brief Reads mesh data into node file and element file
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param nodes Vector of nodes data
 * @param element_type Type of element
 * @param num_elem Number of elements
 * @param enc Vector holding element-node connectivity
 * @param nec Vector holding node-element connectivity
 * @param volumes Vector holding volume of the nodes
 * @param is_fd Flag indicating if this mesh is for finite_difference
 * simulation
 */
void readVtuFile(const std::string &filename, size_t dim,
                 std::vector<util::Point> *nodes, size_t &element_type,
                 size_t &num_elem, std::vector<size_t> *enc,
                 std::vector<std::vector<size_t>> *nec,
                 std::vector<double> *volumes, bool is_fd = false);

/*!
 * @brief Reads nodal coordinates
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param nodes Vector of nodes data
 * @param ref_config Flag which specifies if we need to subtract the
 * displacement from nodes obtained from vtu file to get reference position
 * of nodes
 */
void readVtuFileNodes(const std::string &filename, size_t dim,
                 std::vector<util::Point> *nodes, bool
                     ref_config = false);

/*!
 * @brief Reads cell data, i.e. element-node connectivity and node-element connectivity
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param element_type Type of element
 * @param num_elem Number of elements
 * @param enc Element-node connectivity
 * @param nec Node-element connectivity
 */
void readVtuFileCells(const std::string &filename, size_t dim,
                      size_t &element_type, size_t &num_elem,
                      std::vector<size_t> *enc,
                      std::vector<std::vector<size_t>> *nec);

/*!
 * @brief Reads mesh data into node file and element file
 * @param filename Name of mesh file
 * @param u Pointer to vector of nodal displacement
 * @param v Pointer to vector of nodal velocity
 * @param X Pointer to vector of nodal reference position (Optional)
 */
void readVtuFileRestart(const std::string &filename,
                        std::vector<util::Point> *u,
                        std::vector<util::Point> *v,
                        const std::vector<util::Point> *X = nullptr);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<uint8_t> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<size_t> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<int> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<float> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<double> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                        std::vector<util::Point> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<util::SymMatrix3> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<util::Matrix3> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFileCellData(const std::string &filename,
                          const std::string &tag,
                          std::vector<float> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFileCellData(const std::string &filename,
                         const std::string &tag,
                         std::vector<double> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFileCellData(const std::string &filename,
                         const std::string &tag,
                         std::vector<util::Point> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFileCellData(const std::string &filename,
                         const std::string &tag,
                         std::vector<util::SymMatrix3> *data);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readVtuFileCellData(const std::string &filename,
                         const std::string &tag,
                         std::vector<util::Matrix3> *data);

/** @}*/

/**
 * @name MSH specific functions
 */
/**@{*/

/*!
 * @brief Reads mesh data into node file and element file
 * @param filename Name of mesh file
 * @param dim Dimension
 * @param nodes Vector of nodes data
 * @param element_type Type of element
 * @param num_elem Number of elements
 * @param enc Vector holding element-node connectivity
 * @param nec Vector holding node-element connectivity
 * @param volumes Vector holding volume of the nodes
 * @param is_fd Flag indicating if this mesh is for finite_difference
 * simulation
 */
void readMshFile(const std::string &filename, size_t dim,
                 std::vector<util::Point> *nodes, size_t &element_type,
                 size_t &num_elem, std::vector<size_t> *enc,
                 std::vector<std::vector<size_t>> *nec,
                 std::vector<double> *volumes, bool is_fd = false);

/*!
 * @brief Reads mesh data into node file and element file
 * @param filename Name of mesh file
 * @param u Pointer to vector of nodal displacement
 * @param v Pointer to vector of nodal velocity
 * @param X Pointer to vector of nodal reference position (Optional)
 */
void readMshFileRestart(const std::string &filename,
                        std::vector<util::Point> *u,
                        std::vector<util::Point> *v,
                        const std::vector<util::Point> *X = nullptr);

/*!
 * @brief Reads data of specified tag from the vtu file
 * @param filename Name of mesh file
 * @param tag Name of point data to be read from .vtu file
 * @param data Pointer to vector of point data
 * @return bool True if found the data in file
 */
bool readMshFilePointData(const std::string &filename,
                          const std::string &tag,
                          std::vector<double> *data);

/** @}*/

} // namespace reader

} // namespace rw

#endif // RW_READER_H
