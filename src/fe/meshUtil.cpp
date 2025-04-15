/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "meshUtil.h"
#include "mesh.h"
#include "elemIncludes.h"
#include "util/feElementDefs.h"
#include "util/parallelUtil.h"
#include "util/function.h"

#include <fmt/format.h>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

void fe::createUniformMesh(fe::Mesh *mesh_p, size_t dim, std::pair<std::vector<double>, std::vector<double>> box, std::vector<size_t> nGrid) {

  mesh_p->d_dim = dim;
  if (nGrid.size() < dim or box.first.size() < dim or box.second.size() < dim) {
    std::cerr << "createUniformMesh(): check nGrid or box arguments.\n";
    exit(1);
  }

  if (dim == 1) {
    mesh_p->d_bbox.first = std::vector<double>{box.first[0], 0., 0.};
    mesh_p->d_bbox.second = std::vector<double>{box.second[0], 0., 0.};
    mesh_p->d_numNodes = (nGrid[0] + 1);
    mesh_p->d_numElems = nGrid[0];
    mesh_p->d_eType = util::vtk_type_line;
  } else if (dim == 2) {
    mesh_p->d_bbox.first = std::vector<double>{box.first[0], box.first[1], 0.};
    mesh_p->d_bbox.second = std::vector<double>{box.second[0], box.second[1], 0.};
    mesh_p->d_numNodes = (nGrid[0] + 1) * (nGrid[1] + 1);
    mesh_p->d_numElems = nGrid[0] * nGrid[1];
    mesh_p->d_eType = util::vtk_type_quad;
  } else if (dim == 3) {
    mesh_p->d_bbox.first = std::vector<double>{box.first[0], box.first[1], box.first[2]};
    mesh_p->d_bbox.second = std::vector<double>{box.second[0], box.second[1], box.second[2]};
    mesh_p->d_numNodes = (nGrid[0] + 1) * (nGrid[1] + 1) * (nGrid[2] + 1);
    mesh_p->d_numElems = nGrid[0] * nGrid[1] * nGrid[2];
    mesh_p->d_eType = util::vtk_type_hexahedron;
  } else {
    std::cerr << "createUniformMesh(): invalid dim = " << dim << " argument.\n";
    exit(1);
  }

  mesh_p->d_eNumVertex = util::vtk_map_element_to_num_nodes[mesh_p->d_eType];
  mesh_p->d_numDofs = mesh_p->d_numNodes * mesh_p->d_dim;

  // local nodal data
  mesh_p->d_nodes.resize(mesh_p->d_numNodes);
  mesh_p->d_enc.resize(mesh_p->d_numElems * mesh_p->d_eNumVertex);
  mesh_p->d_fix = std::vector<uint8_t>(mesh_p->d_nodes.size(), uint8_t(0));
  mesh_p->d_vol.resize(mesh_p->d_numNodes);

  // create mesh data
  std::vector<double> h;
  double h_small = 0.;
  for (size_t i=0; i<dim; i++) {
    h.push_back((box.second[i] - box.first[i])/nGrid[i]);
    if (i == 0)
      h_small = h[0];
    else {
      if (std::abs(h[i] - h_small) > 1.0e-14 and h_small > h[1] + 1.0e-14)
        h_small = h[i];
    }
  }

  // set smallest h as mesh size
  mesh_p->d_h = h_small;

  if (dim == 1) {
    // compute node positions
    for (size_t i = 0; i <= nGrid[0]; i++) {
      mesh_p->d_nodes[i] = util::Point(box.first[0] + double(i) * h[0], 0., 0.);
      mesh_p->d_vol[i] = h[0];
      if (i == 0 || i == nGrid[0]) mesh_p->d_vol[i] *= 0.5;
    } // loop over i

    // compute element-node connectivity
    for (size_t i = 0; i < nGrid[0]; i++) {
      // element node connectivity
      // TODO Check if ordering is conforming to the standard
      mesh_p->d_enc[2 * i + 0] = i;
      mesh_p->d_enc[2 * i + 1] = i + 1;
    } // loop over i
  } else if (dim == 2) {
    // compute node positions
    for (size_t j = 0; j <= nGrid[1]; j++) {
      for (size_t i = 0; i <= nGrid[0]; i++) {
        // node number
        size_t n = j * (nGrid[0] + 1) + i;
        mesh_p->d_nodes[n] = util::Point(box.first[0] + double(i) * h[0],
                                         box.first[1] + double(j) * h[1], 0.);

        mesh_p->d_vol[n] = h[0] * h[1];
        if (i == 0 || i == nGrid[0]) mesh_p->d_vol[n] *= 0.5;
        if (j == 0 || j == nGrid[1]) mesh_p->d_vol[n] *= 0.5;
      }  // loop over i
    } // loop over j

    // compute element-node connectivity
    for (size_t j = 0; j < nGrid[1]; j++) {
      for (size_t i = 0; i < nGrid[0]; i++) {

        // element number
        auto n = j * nGrid[0] + i;

        // element node connectivity (put it in anti clockwise order)
        mesh_p->d_enc[4 * n + 0] = j * (nGrid[0] + 1) + i;
        mesh_p->d_enc[4 * n + 1] = j * (nGrid[0] + 1) + i + 1;
        mesh_p->d_enc[4 * n + 2] = (j + 1) * (nGrid[0] + 1) + i + 1;
        mesh_p->d_enc[4 * n + 3] = (j + 1) * (nGrid[0] + 1) + i;
      } // loop over i
    } // loop over j
  } else if (dim == 3) {
    // compute node positions
    for (size_t k = 0; k <= nGrid[2]; k++) {
      for (size_t j = 0; j <= nGrid[1]; j++) {
        for (size_t i = 0; i <= nGrid[0]; i++) {
          // node number
          size_t n = k * (nGrid[1] + 1) * (nGrid[0] + 1) +  j * (nGrid[0] + 1) + i;
          mesh_p->d_nodes[n] = util::Point(box.first[0] + double(i) * h[0],
                                           box.first[1] + double(j) * h[1],
                                           box.first[2] + double(k) * h[2]);

          mesh_p->d_vol[n] = h[0] * h[1] * h[2];
          if (i == 0 || i == nGrid[0]) mesh_p->d_vol[n] *= 0.5;
          if (j == 0 || j == nGrid[1]) mesh_p->d_vol[n] *= 0.5;
          if (k == 0 || k == nGrid[2]) mesh_p->d_vol[n] *= 0.5;
        }  // loop over i
      } // loop over j
    } // loop over k

    // compute element-node connectivity
    for (size_t k = 0; k <= nGrid[2]; k++) {
      for (size_t j = 0; j < nGrid[1]; j++) {
        for (size_t i = 0; i < nGrid[0]; i++) {

          // element number
          auto n = k * nGrid[1] * nGrid[0] + j * nGrid[0] + i;

          // element node connectivity
          // TODO Check if ordering is conforming to the standard
          mesh_p->d_enc[8 * n + 0] =   k * (nGrid[1] + 1) * (nGrid[0] + 1)
                                     + j * (nGrid[0] + 1) + i;
          mesh_p->d_enc[8 * n + 1] =   k * (nGrid[1] + 1) * (nGrid[0] + 1)
                                     + j * (nGrid[0] + 1) + i + 1;
          mesh_p->d_enc[8 * n + 2] =   k * (nGrid[1] + 1) * (nGrid[0] + 1)
                                     + (j + 1) * (nGrid[0] + 1) + i + 1;
          mesh_p->d_enc[8 * n + 3] =   k * (nGrid[1] + 1) * (nGrid[0] + 1)
                                     + (j + 1) * (nGrid[0] + 1) + i;

          mesh_p->d_enc[8 * n + 4] =   (k + 1) * (nGrid[1] + 1) * (nGrid[0] + 1)
                                       + j * (nGrid[0] + 1) + i;
          mesh_p->d_enc[8 * n + 5] =   (k + 1) * (nGrid[1] + 1) * (nGrid[0] + 1)
                                       + j * (nGrid[0] + 1) + i + 1;
          mesh_p->d_enc[8 * n + 6] =   (k + 1) * (nGrid[1] + 1) * (nGrid[0] + 1)
                                       + (j + 1) * (nGrid[0] + 1) + i + 1;
          mesh_p->d_enc[8 * n + 7] =   (k + 1) * (nGrid[1] + 1) * (nGrid[0] + 1)
                                       + (j + 1) * (nGrid[0] + 1) + i;
        } // loop over i
      } // loop over j
    } // loop over k
  }
}

void
fe::getCurrentQuadPoints(const fe::Mesh *mesh_p,
                         const std::vector<util::Point> &xRef,
                         const std::vector<util::Point> &u,
                         std::vector<util::Point> &xQuadCur,
                         size_t iNodeStart,
                         size_t iQuadStart,
                         size_t quadOrder) {

  size_t num_elems = mesh_p->getNumElements();

  // check data
  assert((num_elems != 0) && "Number of elements in the mesh is zero "
                             "possibly due to missing element-node "
                             "connectivity data. Can not proceed with "
                             "computation.\n");

  assert(( (xRef.size() >= mesh_p->getNumNodes() + iNodeStart) ||
           (u.size() >= mesh_p->getNumNodes() + iNodeStart)
          ) &&
           "Number of elements i nnodal data can not be smaller than number of "
                                                   "nodes.\n");

  // get Quadrature
  fe::BaseElem *elem;
  if (mesh_p->getElementType() == util::vtk_type_line)
    elem = new fe::LineElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_triangle)
    elem = new fe::TriElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_quad)
    elem = new fe::QuadElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_tetra)
    elem = new fe::TetElem(quadOrder);
  else {
    std::cerr << fmt::format("Error: Can not compute strain/stress as the element "
                 "type = {} is not yet supported in this routine.\n", mesh_p->getElementType());
    exit(EXIT_FAILURE);
  }

  // get total number of quadrature points by getting the number of quad
  // points in one element times the number of elements
  size_t numQuadPointsTotal = mesh_p->getNumElements() *
          elem->getNumQuadPoints();

  assert((xQuadCur.size() >= numQuadPointsTotal + iQuadStart)
        && "Number of elements in xQuad data can not be less than "
           "total number of quadrature points.\n");

  //  std::cout << fmt::format("num_elems = {}, "
  //                           "iNodeStart = {}, "
  //                           "numQuadPointsTotal = {}, "
  //                           "iQuadStart = {}, "
  //                           "xQuadCur.size() = {}",
  //                           num_elems,
  //                           iNodeStart,
  //                           numQuadPointsTotal,
  //                           iQuadStart,
  //                           xQuadCur.size())
  //           << std::endl;


  // compute current position of quad points
  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;
  taskflow.for_each_index(
        (std::size_t) 0, num_elems, (std::size_t) 1,
        [elem, mesh_p, xRef, u, iNodeStart, iQuadStart, &xQuadCur]
        (std::size_t e) {

          // get ids of nodes of element and reference coordinate of nodes
          auto id_nds = mesh_p->getElementConnectivity(e);
          auto e_nds_start = iNodeStart + mesh_p->d_eNumVertex * e;
          auto e_nds_end = e_nds_start + mesh_p->d_eNumVertex;

          //assert( (e_nds_end <= xRef.size()) && "e_nds_end bigger than size of xRef\n" );

          //std::vector<util::Point> nds(xRef.begin() + e_nds_start,
          //                             xRef.begin() + e_nds_end);
          std::vector<util::Point> nds;
          for (const auto &i : id_nds)
            nds.push_back(xRef[i + iNodeStart]);

          auto qds = elem->getQuadDatas(nds);

          auto qd_point_current = util::Point();

          for (size_t q=0; q<qds.size(); q++) {
            qd_point_current = qds[q].d_p;
            for (size_t i = 0; i < id_nds.size(); i++) {
              auto i_global_id = iNodeStart + id_nds[i];
              qd_point_current += u[i_global_id] * qds[q].d_shapes[i];
            }

            // location of this quad points current position in the vector
            // is e * getNumQuadPoints() + q, where e is the index of
            // current element we are processing
            auto q_global_id = iQuadStart + e * elem->getNumQuadPoints() + q;
            xQuadCur[q_global_id] = qd_point_current;
          }
        } // loop over elements
  ); // for_each

  executor.run(taskflow).get();
}

void fe::getStrainStress(const fe::Mesh *mesh_p,
                         const std::vector<util::Point> & xRef,
                         const std::vector<util::Point> &u,
                         bool isPlaneStrain,
                         std::vector<util::SymMatrix3> &strain,
                         std::vector<util::SymMatrix3> &stress,
                         size_t iNodeStart,
                         size_t iStrainStart,
                         double nu,
                         double lambda,
                         double mu,
                         bool computeStress,
                         size_t quadOrder) {

  assert((mesh_p->getDimension() > 1) && "In getStrainStress(), dimension = 2,3 is supported.\n");

  size_t num_elems = mesh_p->getNumElements();

  // check data
  assert((num_elems != 0) && "Number of elements in the mesh is zero "
                             "possibly due to missing element-node "
                             "connectivity data. Can not proceed with "
                             "computation.\n");

  assert(( (xRef.size() >= mesh_p->getNumNodes() + iNodeStart) ||
           (u.size() >= mesh_p->getNumNodes() + iNodeStart)
         ) &&
         "Number of elements i nodal data can not be smaller than number of "
         "nodes.\n");

  // get Quadrature
  fe::BaseElem *elem;
  if (mesh_p->getElementType() == util::vtk_type_line)
    elem = new fe::LineElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_triangle)
    elem = new fe::TriElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_quad)
    elem = new fe::QuadElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_tetra)
    elem = new fe::TetElem(quadOrder);
  else {
    std::cerr << "Error: Can not compute strain/stress as the element "
                 "type is not yet supported in this routine.\n";
    exit(EXIT_FAILURE);
  }

  // get total number of quadrature points by getting the number of quad
  // points in one element times the number of elements
  size_t numQuadPointsTotal = mesh_p->getNumElements() *
                              elem->getNumQuadPoints();

  assert((strain.size() >= numQuadPointsTotal + iStrainStart)
         && "Number of elements in strain data can not be less than "
            "total number of quadrature points.\n");

  // check if we can compute stress from given material data
  computeStress = computeStress || util::isLess(mu, 1.e-16) || util::isLess(lambda, 1.e-16);

  if (computeStress)
    assert((stress.size() >= numQuadPointsTotal + iStrainStart)
         && "Number of elements in stress data can not be less than "
            "total number of quadrature points.\n");

  // compute current position of quad points
  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;
  taskflow.for_each_index(
          (std::size_t) 0, num_elems, (std::size_t) 1,
          [elem, mesh_p, xRef, u, iNodeStart, iStrainStart,
           isPlaneStrain, nu, lambda, mu, computeStress,
           &strain, &stress]
                  (std::size_t e) {

              auto ssn = util::SymMatrix3();
              auto sss = util::SymMatrix3();

              // get ids of nodes of element and reference coordinate of nodes
              auto id_nds = mesh_p->getElementConnectivity(e);
              auto e_nds_start = iNodeStart + mesh_p->d_eNumVertex * e;
              auto e_nds_end = e_nds_start + mesh_p->d_eNumVertex;
              //std::vector<util::Point> nds(xRef.begin() + e_nds_start,
              //                             xRef.begin() + e_nds_end);
              std::vector<util::Point> nds;
              for (const auto &i : id_nds)
                nds.push_back(xRef[i + iNodeStart]);

              auto qds = elem->getQuadDatas(nds);

              for (size_t q=0; q<qds.size(); q++) {
                ssn = util::SymMatrix3();
                sss = util::SymMatrix3();

                // compute strain
                for (size_t i = 0; i < id_nds.size(); i++) {
                  auto i_global_id = iNodeStart + id_nds[i];
                  auto ui = u[i_global_id];

                  ssn(0, 0) += ui[0] + qds[q].d_derShapes[i][0];
                  if (mesh_p->getDimension() > 1) {
                    ssn(1, 1) += ui[1] + qds[q].d_derShapes[i][1];
                    // xy
                    ssn(0, 1) += 0.5 * ui[0] * qds[q].d_derShapes[i][1] +
                                 0.5 * ui[1] * qds[q].d_derShapes[i][0];
                  }
                  if (mesh_p->getDimension() > 2) {
                    ssn(2, 2) += ui[2] + qds[q].d_derShapes[i][2];

                    // yz
                    ssn(1, 2) += 0.5 * ui[1] * qds[q].d_derShapes[i][2] +
                                 0.5 * ui[2] * qds[q].d_derShapes[i][1];
                    // xz
                    ssn(0, 2) += 0.5 * ui[0] * qds[q].d_derShapes[i][2] +
                                 0.5 * ui[2] * qds[q].d_derShapes[i][0];
                  }
                }

                if (mesh_p->getDimension() == 2 && isPlaneStrain)
                  ssn(2, 2) = -nu * (ssn(0, 0) + ssn(1, 1)) / (1. - nu);

                // compute stress
                if (computeStress) {
                  auto trace_ssn = ssn(0, 0) + ssn(1, 1) + ssn(2, 2);
                  sss(0, 0) = lambda * trace_ssn + 2 * mu * ssn(0, 0);
                  sss(0, 1) = 2 * mu * ssn(0, 1);
                  sss(0, 2) = 2 * mu * ssn(0, 2);

                  sss(1, 1) = lambda * trace_ssn + 2 * mu * ssn(1, 1);
                  sss(1, 2) = 2 * mu * ssn(1, 2);

                  sss(2, 2) = lambda * trace_ssn + 2 * mu * ssn(2, 2);

                  if (mesh_p->getDimension() == 2 && !isPlaneStrain)
                    sss(2, 2) = nu * (sss(0, 0) + sss(1, 1));
                }

                // location of this quad points in the vector
                // is e * getNumQuadPoints() + q, where e is the index of
                // current element we are processing
                auto q_global_id = iStrainStart + e * elem->getNumQuadPoints() + q;
                strain[q_global_id] = ssn;
                if (computeStress)
                  stress[q_global_id] = sss;
              }
          } // loop over elements
  ); // for_each

  executor.run(taskflow).get();
}

void fe::getMaxShearStressAndLoc(const fe::Mesh *mesh_p,
                                 const std::vector<util::Point> & xRef,
                                 const std::vector<util::Point> &u,
                                 const std::vector<util::SymMatrix3> &stress,
                                 double &maxShearStress,
                                 util::Point &maxShearStressLocRef,
                                 util::Point &maxShearStressLocCur,
                                 size_t iNodeStart,
                                 size_t iStrainStart,
                                 size_t quadOrder) {

  assert((mesh_p->getDimension() == 2) && "In getMaxShearStressAndLoc(), only dimension = 2 is supported.\n");

  size_t num_elems = mesh_p->getNumElements();

  // check data
  assert((num_elems != 0) && "Number of elements in the mesh is zero "
                             "possibly due to missing element-node "
                             "connectivity data. Can not proceed with "
                             "computation.\n");

  assert(( (xRef.size() >= mesh_p->getNumNodes() + iNodeStart) ||
           (u.size() >= mesh_p->getNumNodes() + iNodeStart)
         ) &&
         "Number of elements i nnodal data can not be smaller than number of "
         "nodes.\n");

  // get Quadrature
  fe::BaseElem *elem;
  if (mesh_p->getElementType() == util::vtk_type_line)
    elem = new fe::LineElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_triangle)
    elem = new fe::TriElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_quad)
    elem = new fe::QuadElem(quadOrder);
  else if (mesh_p->getElementType() == util::vtk_type_tetra)
    elem = new fe::TetElem(quadOrder);
  else {
    std::cerr << "Error: Can not compute strain/stress as the element "
                 "type is not yet supported in this routine.\n";
    exit(EXIT_FAILURE);
  }

  // get total number of quadrature points by getting the number of quad
  // points in one element times the number of elements
  size_t numQuadPointsTotal = mesh_p->getNumElements() *
                              elem->getNumQuadPoints();

  assert((stress.size() >= numQuadPointsTotal + iStrainStart)
         && "Number of elements in stress data can not be less than "
            "total number of quadrature points.\n");

  // compute principal shear stress
  double max_stress = 0.;
  size_t max_stress_e = 0;
  size_t max_stress_q = 0;
  for (size_t e = 0; e < num_elems; e++) {
    for (size_t q=0; q<elem->getNumQuadPoints(); q++) {
      auto q_global_id = iStrainStart + e * elem->getNumQuadPoints() + q;
      const auto stress_e = stress[q_global_id];

      const auto principle_shear_stress =
              std::sqrt(0.25 * std::pow(stress_e.get(0) - stress_e.get(1), 2) +
                        std::pow(stress_e.get(5), 2));

      if (util::isLess(max_stress, principle_shear_stress)) {
        max_stress = principle_shear_stress;
        max_stress_e = e;
        max_stress_q = q;
      }
    }
  }

  // set data
  maxShearStress = max_stress;

  // now compute current and reference location of the quadrature point at which
  // stress is maximum
  {
    // get ids of nodes of element and reference coordinate of nodes
    auto id_nds = mesh_p->getElementConnectivity(max_stress_e);
    auto e_nds_start = iNodeStart + mesh_p->d_eNumVertex * max_stress_e;
    auto e_nds_end = e_nds_start + mesh_p->d_eNumVertex;
    std::vector<util::Point> nds(xRef.begin() + e_nds_start, xRef.begin() + e_nds_end);

    auto qds = elem->getQuadDatas(nds);
    auto qd_point_current = qds[max_stress_q].d_p;
    maxShearStressLocRef = qd_point_current;
    for (size_t i = 0; i < id_nds.size(); i++) {
      auto i_global_id = iNodeStart + id_nds[i];
      qd_point_current += u[i_global_id] * qds[max_stress_q].d_shapes[i];
    }

    maxShearStressLocCur = qd_point_current;
  }
}

