/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FE_MESH_H
#define FE_MESH_H

#include "util/point.h" // definition of struct Point
#include <string>
#include <vector>

// forward declaration of geometry deck
namespace inp {
struct MeshDeck;
}

/*!
 * @brief Collection of methods and data related to finite element and mesh
 *
 * This namespace groups the data and methods related to finite element
 * methods such as quadrature points, finite elements, and also data and
 * methods related to mesh such as nodal coordinates, element-node
 * connectivity, etc.
 */
namespace fe {

/*! @brief A class for mesh data
 *
 * In this class the mesh data such as nodes, element-node connectivity,
 * node-element connectivity are stored. The class also stores fixity mask of
 * nodes which indicates if x-, y-, or z-dof of the node is fixed or free.
 *
 * We currently only support mesh with only one type of elements, i.e. mesh
 * can not have mix of two types of elements. For example, we can not have
 * mesh with triangle and quadrangle elements together.
 *
 * This class is used in both finite difference implementation and finite
 * element implementation. For finite difference, we only require nodal
 * volume. If the mesh file contains nodal volume, we skip reading
 * element-node and node-element connectivity, however if mesh file does not
 * have nodal volume data, we read connectivity data and compute the nodal
 * volume.
 */
class Mesh {

public:
  /*!
   * @brief Constructor
   *
   * @param dim Dimension of the domain
   */
  explicit Mesh(size_t dim = 0);

  /*!
   * @brief Constructor
   *
   * The constructor initializes the data using input deck, performs checks
   * on input data, and reads mesh file and populates the mesh related data.
   * The mesh file of  **.csv**, **.vtu (VTK)** and **.msh (Gmsh)** are
   * supported.
   *
   * @param deck Input deck which contains user-specified information
   */
  explicit Mesh(inp::MeshDeck *deck);

  /**
   * @name Accessor methods
   */
  /**@{*/

  /*!
   * @brief Get the dimension of the domain
   * @return N Dimension
   */
  size_t getDimension() const { return d_dim; };

  /*!
   * @brief Get the number of nodes
   * @return N number of nodes
   */
  size_t getNumNodes() const { return d_numNodes; };

  /*!
   * @brief Get the number of elements
   * @return N Number of elements
   */
  size_t getNumElements() const { return d_enc.size()/d_eNumVertex; };

  /*!
   * @brief Get the number of dofs
   * @return N Number of dofs
   */
  size_t getNumDofs() const { return d_numDofs; };

  /*!
   * @brief Get the type of element in mesh
   * @return type Element type (using VTK convention)
   */
  size_t getElementType() const { return d_eType; };

  /*!
   * @brief Get the mesh size
   * @return h Mesh size
   */
  double getMeshSize() const { return d_h; };

  /*!
   * @brief Get coordinates of node i
   * @param i Id of the node
   * @return coords Coordinates
   */
  util::Point getNode(const size_t &i) const { return d_nodes[i]; };

  /*!
   * @brief Get nodal volume of node i
   * @param i Id of the node
   * @return vol Volume
   */
  double getNodalVolume(const size_t &i) const { return d_vol[i]; };

  /*!
   * @brief Get the nodes data
   * @return nodes Nodes data
   */
  const std::vector<util::Point> &getNodes() const { return d_nodes; };

  /*! @copydoc getNodes() const */
  std::vector<util::Point> &getNodes() { return d_nodes; };

  /*!
   * @brief Get the pointer to nodes data
   * @return pointer Pointer to nodes data
   */
  const std::vector<util::Point> *getNodesP() const { return &d_nodes; };

  /*! @copydoc getNodesP() const */
  std::vector<util::Point> *getNodesP() { return &d_nodes; };

  /*!
   * @brief Get the pointer to fixity data
   * @return pointer Pointer to fixity data
   */
  const std::vector<uint8_t> *getFixityP() const { return &d_fix; };

  /*! @copydoc getFixityP() const */
  std::vector<uint8_t> *getFixityP() { return &d_fix; };

  /*!
   * @brief Get the reference to fixity data
   * @return reference Reference to fixity data
   */
  const std::vector<uint8_t> &getFixity() const { return d_fix; };

  /*! @copydoc getFixity() const */
  std::vector<uint8_t> &getFixity() { return d_fix; };

  /*!
   * @brief Get the nodal volume data
   * @return Vector Vector of nodal volume
   */
  const std::vector<double> &getNodalVolumes() const { return d_vol; };

  /*! @copydoc getNodalVolumes() const */
  std::vector<double> &getNodalVolumes() { return d_vol; };

  /*!
   * @brief Get the pointer to nodal volume data
   * @return pointer Pointer to nodal volume data
   */
  const std::vector<double> *getNodalVolumesP() const { return &d_vol; };

  /*! @copydoc getNodalVolumesP() const */
  std::vector<double> *getNodalVolumesP() { return &d_vol; };

  /*!
   * @brief Return true if node is free
   * @param i Id of node
   * @param dof Dof to check for
   * @return bool True if dof is free else false
   */
  bool isNodeFree(const size_t &i, const unsigned int &dof) const {

    // below checks if d_fix has 1st bit (if dof=0), 2nd bit (if dof=1), 3rd
    // bit (if dof=2) is set to 1 or 0. If set to 1, then it means it is fixed,
    // and therefore it returns false
    return !(d_fix[i] >> dof & 1UL);
  };

  /*!
   * @brief Get the connectivity of element
   *
   * Since we store connectivity in a single vector, we use
   * fe::Mesh::d_eNumVertex to get the connectivity of element. Given element
   * e, the connectivity of e begins from location \f$ i_0 = e*d\_eNumVertex + 0
   * \f$ upto \f$i_{n-1} = e*d\_eNumVertex + d\_eNumVertex - 1\f$.
   *
   * So the connectivity of e is
   * d_enc[\f$i_0\f$], d_enc[\f$ i_1 \f$], ..., d_end[\f$i_{n-1}\f$]
   *
   * @param i Id of an element
   * @return vector Vector of nodal ids
   */
  std::vector<size_t> getElementConnectivity(const size_t &i) const {
    return std::vector<size_t>(d_enc.begin() + d_eNumVertex * i,
                               d_enc.begin() + d_eNumVertex * i + d_eNumVertex);
  };

  /*!
   * @brief Get the vertices of element
   *
   * @param i Id of an element
   * @return vector Vector of vertices
   */
  std::vector<util::Point> getElementConnectivityNodes(const size_t
                                                              &i) const {
    std::vector<util::Point> nds;
    for (size_t k = 0; k < d_eNumVertex; k++)
      nds.emplace_back(d_nodes[d_enc[d_eNumVertex * i + k]]);
    return nds;
  };

  /*!
   * @brief Get the reference to element-node connectivity data
   * @return reference Reference
   */
  const std::vector<size_t> &getElementConnectivities() const {
    return d_enc;
  };

  /*! @copydoc getElementConnectivities() const */
  std::vector<size_t> &getElementConnectivities() {
    return d_enc;
  };

  /*!
   * @brief Get the pointer to element-node connectivity data
   * @return pointer Pointer
   */
  const std::vector<size_t> *getElementConnectivitiesP() const {
    return &d_enc;
  };

  /*! @copydoc getElementConnectivitiesP() const */
  std::vector<size_t> *getElementConnectivitiesP() {
    return &d_enc;
  };

  /*!
   * @brief Get the bounding box of the mesh
   * @return box Bounding box
   */
  const std::pair<std::vector<double>, std::vector<double>> &getBoundingBox()
  const {
    return d_bbox;
  };

  /*! @copydoc getBoundingBox() const */
  std::pair<std::vector<double>, std::vector<double>> &getBoundingBox() {
    return d_bbox;
  };

  /** @}*/

  /**
   * @name Setter methods
   */
  /**@{*/

  /*!
   * @brief Set the fixity to free (0) or fixed (1)
   * @param i Id of node
   * @param dof Dof which is affected
   * @param flag Set fixity to fixed if true or free
   */
  void setFixity(const size_t &i, const unsigned int &dof, const bool &flag);

  /*!
   * @brief Clear element-node connectivity data
   */
  void clearElementData();

  /** @}*/

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt = 0, int lvl = 0) const;

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); };

public:
  /**
   * @name Utility methods
   */
  /**@{*/

  /*!
   * @brief Reads mesh data from the file and populates other data
   *
   * This function calls reader methods in namespace rw::reader to read the
   * mesh file. For finite difference implementation, we support **.csv** mesh
   * file which has nodal coordinates and nodal volumes data.
   *
   * However, for finite element implementation, we require either **.vtu** or
   * **.msh** file with element-node connectivity data.
   *
   * @param filename Name of the mesh file
   * @param ref_config Flag which specifies if we need to subtract the
   * displacement from nodes obtained from vtu file to get reference position
   * of nodes
   * */
  void createData(const std::string &filename, bool
  ref_config = false);
  /*!
   * @brief Compute the nodal volume
   *
   * This method requires element-node connectivity data to compute the nodal
   * volumes. Formula for volume of a node \f$ i\f$ is given by
   * \f[ V_i = \sum_{e \in \mathbf{N}_i} \int_{T_e} N_i(x) dx, \f]
   * where \f$\mathbf{N}_i\f$ is a list of elements which have node \f$ i\f$
   * as its vertex, \f$ T_e\f$ is the element domain, \f$ N_i\f$ is the shape
   * function of the node \f$ i\f$ in element e.
   */
  void computeVol();

  /*! @brief Compute the bounding box  */
  void computeBBox();

  /*!
   * @brief Compute the mesh size
   *
   * This method searches for minimum distance between any two mesh nodes and
   * stores it as a mesh size.
   */
  void computeMeshSize();

  /** @}*/

  /**
   * @name Mesh data
   */
  /**@{*/

  /*! @brief Number of nodes */
  size_t d_numNodes;

  /*! @brief Number of elements */
  size_t d_numElems;

  /*! @brief Element type
   *
   * We follow VTK convention to identify the elements:
   * - Line element = 3,
   * - Triangle element = 5,
   * - Pixel element = 8,
   * - Quadrilateral element = 9,
   * - Tetrahedral element = 10
   */
  size_t d_eType;

  /*! @brief Number of vertex per element
   *
   * This information is useful in getting the connectivity for a given
   * element. We assume that the mesh has only one type of elements and based
   * on that assumption we store the element-node connectivity in a single
   * vector.
   *
   * The value for different elements are
   * - Line element: 2,
   * - Triangle element: 3,
   * - Quadrilateral element: 4,
   * - Tetrahedral element: 4
   */
  size_t d_eNumVertex;

  /*! @brief Vector of initial (reference) coordinates of nodes */
  std::vector<util::Point> d_nodes;

  /*! @brief Element-node connectivity data
   *
   * Structure: First d_eNumVertex data gives the connectivity of first
   * element, and next d_eNumVertex data gives the connectivity of second
   * element and so on and so fourth.
   */
  std::vector<size_t> d_enc;

  /*! @brief Node-element connectivity data
   *
   * At present, this data is never populated.
   */
  std::vector<std::vector<size_t>> d_nec;

  /*! @brief Vector of fixity mask of each node
   *
   * First bit represents x-dof, second represents y-dof, and third
   * represents z-dof. 0 represents free dof and 1 represents fixed dof.
   *
   * We store data in uint8_t type which can hold 8 bit. At present we only
   * use first 3 bits.
   */
  std::vector<uint8_t> d_fix;

  /*! @brief Vector of volume of each node
   *
   * For uniform square mesh, the volume is simply \f$ h^2 \f$ in 2-d and \f$
   * h^3\f$ in 3-d, where \f$ h\f$ is the mesh size. For general mesh, the
   * volume is computed using the element-node connectivity of the mesh.
   */
  std::vector<double> d_vol;

  /** @}*/

  /**
   * @name Mesh data specific to parallel implementation
   */
  /**@{*/

  /*! @brief Number of partitions */
  size_t d_nPart;

  /*! @brief Partitioning method.
   * It could be either empty string or "metis_recursive" or "metis_kway".
   */
  std::string d_partitionMethod;

  /*! @brief Node partition information.
   * For each node i, d_nodePartition[i] specifies the partition number, i.e., the processor that owns the node in MPI application.
   *
   * For uniform square mesh, the volume is simply \f$ h^2 \f$ in 2-d and \f$
   * h^3\f$ in 3-d, where \f$ h\f$ is the mesh size. For general mesh, the
   * volume is computed using the element-node connectivity of the mesh.
   */
  std::vector<size_t> d_nodePartition;

  /** @}*/

  /*! @brief Dimension of the mesh */
  size_t d_dim;

  /*! @brief Tag for spatial discretization type
   *
   * List of valid values are:
   * - **finite_difference**
   * - **weak_finite_element**
   * - **nodal_finite_element**
   * - **truss_finite_element**
   */
  std::string d_spatialDiscretization;

  /*! @brief Filename to read mesh data */
  std::string d_filename;

  /*! @brief Number of dofs = (dimension) times (number of nodes) */
  size_t d_numDofs;

  /*! @brief Map from global reduced id to default global id
   *
   * We assign number to each free dof where number ranges from 0
   * to total number of free dofs. This is referred to set of global reduced
   * id. This new set of ids are subset of set of global ids of all dofs, and
   * therefore, "reduced" word is used.
   *
   * d_gMap provides a map from global reduced id to global id.
   *
   * @note Needed only when the discretization is "weak_finite_element" for the
   *  assembly of the mass matrix.
   */
  std::vector<size_t> d_gMap;

  /*! @brief Map from global id to reduced global id.
   *
   * This is a inverse of d_gMap
   */
  std::vector<int> d_gInvMap;

  /*! @brief Bounding box */
  std::pair<std::vector<double>, std::vector<double>> d_bbox;

  /*! @brief Mesh size */
  double d_h;
};

} // namespace fe

#endif // FE_MESH_H
