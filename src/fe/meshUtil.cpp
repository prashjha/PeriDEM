/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "meshUtil.h"
#include "mesh.h"
#include "util/feElementDefs.h"

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