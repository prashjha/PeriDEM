/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#pragma once

#include "util/point.h"
#include "util/matrix.h"
#include "inp/materialDeck.h"
#include <string>
#include <vector>

namespace mesh {

// forward declare Mesh
class Mesh;

/*! @brief Creates uniform mesh for rectangle/cuboid domain
 *
 * @param mesh_p Pointer to already created possibly empty mesh object
 * @param dim Dimension of the domain
 * @param box Specifies domain (e.g., rectangle/cuboid)
 * @param nGrid Grid sizes in dim directions
 */
void createUniformMesh(mesh::Mesh *mesh_p, size_t dim, std::pair<std::vector<double>, std::vector<double>> box, std::vector<size_t> nGrid);

/*!
 * @brief Get current location of quadrature points of elements in the mesh.
 * This function expects mesh has element-node connectivity data.
 *
 * In case of multiple particles and meshes, xRef and u data will hold data
 * for all meshes. If this is the case, iNodeStart integer can be used to
 * specify from what index the data for a given mesh should be read. E.g., if
 * we have two particles with their own mesh, and suppose particle 1 and 2
 * have n1 and n2 number nodes than
 * 1. xRef and u will be a vector of n1+n2 size
 * 2. For particle 1, node data in xRef and u starts from iNodeStart = 0
 * 3. For particle 2, node data in xRef and u starts from iNodeStart = n1
 *
 * For the above example, suppose first particle has total nq1 number of quadrature
 * points from all the elements in the mesh of particle 1 and second particle has total nq2
 * number of quadrature points. Then,
 * 1. xQuadCur will be of size nq1 + nq2
 * 2. For particle 1, quad data in xQuadCur starts from iQuadStart = 0
 * 3. For particle 2, quad data in xQuadCur starts from iQuadStart = nq2
 *
 * @param mesh_p Pointer to already created possibly empty mesh object
 * @param xRef Vector of reference coordinates of nodes
 * @param u Vector of displacement of nodes
 * @param xQuadCur Vector of current positions of quadrature points (this
 * argument is modified)
 * @param iNodeStart Assume that nodal data in xRef and u starts from iNodeStart
 * @param iQuadStart Assume that quadrature data in xQuadCur starts from
 * iNodeStart
 * @param quadOrder Order of quadrature approximation (default is 1)
 */
void getCurrentQuadPoints(const mesh::Mesh *mesh_p,
                          const std::vector<util::Point> &xRef,
                          const std::vector<util::Point> &u,
                          std::vector<util::Point> &xQuadCur,
                          size_t iNodeStart = 0,
                          size_t iQuadStart = 0,
                          size_t quadOrder = 1);

/*!
 * @brief Strain and stress at quadrature points in the mesh
 *
 * In case of multiple particles and meshes, xRef and u data will hold data
 * for all meshes. If this is the case, iNodeStart integer can be used to
 * specify from what index the data for a given mesh should be read. Similarly,
 * iStrainStart can be used to specify from what index the data for strain and stress
 * should be substituted in strain/stress vectors. See documentation of @getCurrentQuadPoints().
 *
 * @param mesh_p Pointer to already created possibly empty mesh object
 * @param xRef Vector of reference coordinates of nodes
 * @param u Vector of displacement of nodes
 * @param isPlaneStrain Bool that indicates whether to use plane stress/strain assumption (only in 2-d)
 * @param strain Vector of symmetric matrix to store strain (this
 * argument is modified)
 * @param stress Vector of symmetric matrix to store stress (this
 * argument is modified)
 * @param iNodeStart Assume that nodal data in xRef and u starts from iNodeStart
 * @param iStrainStart Assume that quadrature data in strain/stress starts from
 * iNodeStart
 * @param nu Poisson ratio (default is zero)
 * @param lambda Lame's first parameter (default is zero and for this value, stress will not be computed)
 * @param mu Lame's second parameter, i.e., shear modulus (default is zero and for this value, stress will not be computed)
 * @param computeStress False will not compute stress
 * @param quadOrder Order of quadrature approximation (default is 1)
 */
void getStrainStress(const mesh::Mesh *mesh_p,
                     const std::vector<util::Point> & xRef,
                     const std::vector<util::Point> &u,
                     bool isPlaneStrain,
                     std::vector<util::SymMatrix3> &strain,
                     std::vector<util::SymMatrix3> &stress,
                     size_t iNodeStart = 0,
                     size_t iStrainStart = 0,
                     double nu = 0.,
                     double lambda = 0.,
                     double mu = 0.,
                     bool computeStress = false,
                     size_t quadOrder = 1);

/*!
 * @brief Get location where maximum of specified component of stress
 * occurs in this particle
 *
 * @param mesh_p Pointer to already created possibly empty mesh object
 * @param xRef Vector of reference coordinates of nodes
 * @param u Vector of displacement of nodes
 * @param stress Vector of symmetric stress tensor
 * @param maxShearStress Value of maximum shear stress
 * @param maxShearStressLocRef Location where this occurs (in reference configuration)
 * @param maxShearStressLocCur Location where this occurs (in current configuration)
 * @param iNodeStart Assume that nodal data in xRef and u starts from iNodeStar
 * @param iStrainStart Assume that quadrature data in strain/stress starts from
 * iNodeStart
 * @param quadOrder Order of quadrature approximation (default is 1)
 */
void getMaxShearStressAndLoc(const mesh::Mesh *mesh_p,
                             const std::vector<util::Point> & xRef,
                             const std::vector<util::Point> &u,
                             const std::vector<util::SymMatrix3> &stress,
                             double &maxShearStress,
                             util::Point &maxShearStressLocRef,
                             util::Point &maxShearStressLocCur,
                             size_t iNodeStart = 0,
                             size_t iStrainStart = 0,
                             size_t quadOrder = 1);

} // namespace mesh