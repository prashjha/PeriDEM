/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testMeshPartitioningLib.h"
#include "fe/mesh.h"
#include "fe/meshPartitioning.h"
#include "util/point.h"
#include "util/feElementDefs.h"
#include "util/methods.h"
#include "util/io.h"
#include "rw/writer.h"
#include "nsearch/nsearch.h"
#include <fmt/format.h>
#include <algorithm>
#include <fstream>
#include <string>

typedef nsearch::NFlannSearchKd NSearch;

namespace {

    std::string createUniformMesh(fe::Mesh * mesh_p, const size_t &nGrid) {

      // set geometry details
      double Lx(1.), Ly(1.), h;
      size_t Nx, Ny;
      Nx = nGrid;
      Ny = nGrid;
      h = double(Lx/Nx);

      // create mesh
      mesh_p->d_h = h;
      mesh_p->d_numNodes = (Nx + 1) * (Ny + 1);
      mesh_p->d_numElems = Nx * Ny;
      mesh_p->d_eType = util::vtk_type_quad; // quad element
      mesh_p->d_eNumVertex = util::vtk_map_element_to_num_nodes[mesh_p->d_eType]; // value is 4
      mesh_p->d_numDofs = mesh_p->d_numNodes * mesh_p->d_dim;
      mesh_p->d_bbox.first = std::vector<double>{0., 0., 0.};
      mesh_p->d_bbox.second = std::vector<double>{Lx, Ly, 0.};

      // local nodal data
      mesh_p->d_nodes.resize(mesh_p->d_numNodes);
      mesh_p->d_enc.resize(mesh_p->d_numElems * mesh_p->d_eNumVertex);
      mesh_p->d_fix = std::vector<uint8_t>(mesh_p->d_nodes.size(), uint8_t(0));
      mesh_p->d_vol.resize(mesh_p->d_numNodes);

      // create nodal data
      for (size_t j = 0; j <= Ny; j++)
        for (size_t i = 0; i <= Nx; i++) {
          // node number
          size_t n = j * (Nx + 1) + i;
          mesh_p->d_nodes[n] = util::Point(double(i) * mesh_p->d_h, double(j) * mesh_p->d_h, 0.);

          mesh_p->d_vol[n] = mesh_p->d_h * mesh_p->d_h;
          if (i == 0 || i == Nx) mesh_p->d_vol[n] *= 0.5;
          if (j == 0 || j == Ny) mesh_p->d_vol[n] *= 0.5;
        }  // loop over j

      // compute element-node connectivity
      for (size_t j = 0; j < Ny; j++)
        for (size_t i = 0; i < Nx; i++) {

          // element number
          auto n = j * Nx + i;

          // element node connectivity (put it in anti clockwise order)
          mesh_p->d_enc[4 * n + 0] = j * (Nx + 1) + i;
          mesh_p->d_enc[4 * n + 1] = j * (Nx + 1) + i + 1;
          mesh_p->d_enc[4 * n + 2] = (j + 1) * (Nx + 1) + i + 1;
          mesh_p->d_enc[4 * n + 3] = (j + 1) * (Nx + 1) + i;
        }

      return fmt::format("uniform_mesh_Lx_{}_Ly_{}_Nx_{}_Ny_{}", Lx, Ly, Nx, Ny);
    }

    std::string readMeshFromFile(fe::Mesh * mesh_p, const std::string &meshFilename) {
      if (meshFilename.empty()) {
        std::cerr << "readMeshFromFile(): mesh filename is empty.\n";
        exit(1);
      }

      // call in-built function of mesh to create data from file
      mesh_p->createData(meshFilename);

      // find the name of mesh file without path and without extension (i.e., remove .vtu/.msh/.csv extension)
      auto f1 = util::io::getFilenameFromPath(meshFilename);
      auto f2 = util::io::removeExtensionFromFile(f1);
      auto f3 = util::io::getExtensionFromFile(f1);

      // for debugging
      std::cout << fmt::format("Mesh filename = {}\n"
                               "Filename without path = {}\n"
                               "Filename without path and extension = {}\n"
                               "File extension = {}\n", meshFilename, f1, f2, f3);

      return f2;
    }

    // source - https://code.vt.edu/ARC/examples/-/blob/master/metis/metis_test.c?ref_type=heads
    void partGraphRecursiveTestSimple() {
      // The number of vertices.
      idx_t nvtxs = 6;

      // Number of balancing constraints, which must be at least 1.
      idx_t ncon = 1;

      // Pointers to initial entries in adjacency array. size of array is nvtxs + 1
      idx_t xadj[7] = {0, 2, 5, 7, 9, 12, 14};

      // Adjacent vertices in consecutive index order.
      int nedges = 7;
      idx_t adjncy[14] = {1, 3, 0, 4, 2, 1, 5, 0, 4, 3, 1, 5, 4, 2};

      // The number of parts requested for the partition.
      idx_t nParts = 3;

      // On return, the edge cut volume of the partitioning solution.
      idx_t objval;

      // On return, the partition vector for the graph.
      idx_t part[6];

      printf("\n");
      printf("partGraphRecursiveTest:\n");
      printf("  METIS_PartGraphRecursive partitions a graph into K parts\n");
      printf("  using multilevel recursive bisection.\n");

      int ret = METIS_PartGraphRecursive(&nvtxs, &ncon, xadj, adjncy, NULL, NULL,
                                         NULL, &nParts, NULL, NULL, NULL, &objval, part);

      printf("\n");
      printf("  Return code = %d\n", ret);
      printf("  Edge cuts for partition = %d\n", (int) objval);

      printf("\n");
      printf("  Partition vector:\n");
      printf("\n");
      printf("  Node  Part\n");
      printf("\n");
      for (unsigned part_i = 0; part_i < nvtxs; part_i++) {
        printf("     %d     %d\n", part_i, (int) part[part_i]);
      }

    }

    // source - https://code.vt.edu/ARC/examples/-/blob/master/metis/metis_test.c?ref_type=heads
    void partGraphKwayTestSimple() {

      // The number of vertices.
      idx_t nvtxs = 6;

      // Number of balancing constraints, which must be at least 1.
      idx_t ncon = 1;

      // Pointers to initial entries in adjacency array.
      idx_t xadj[7] = {0, 2, 5, 7, 9, 12, 14};

      // Adjacent vertices in consecutive index order.
      int nedges = 7;
      idx_t adjncy[14] = {1, 3, 0, 4, 2, 1, 5, 0, 4, 3, 1, 5, 4, 2};

      // The number of parts requested for the partition.
      idx_t nParts = 3;

      // On return, the edge cut volume of the partitioning solution.
      idx_t objval;

      // On return, the partition vector for the graph.
      idx_t part[6];

      printf("\n");
      printf("partGraphKwayTest:\n");
      printf("  METIS_PartGraphKway partitions a graph into K parts\n");
      printf("  using multilevel K-way partition.\n");

      int ret = METIS_PartGraphKway(&nvtxs, &ncon, xadj, adjncy, NULL, NULL,
                                    NULL, &nParts, NULL, NULL, NULL, &objval, part);

      printf("\n");
      printf("  Return code = %d\n", ret);
      printf("  Edge cuts for partition = %d\n", (int) objval);

      printf("\n");
      printf("  Partition vector:\n");
      printf("\n");
      printf("  Node  Part\n");
      printf("\n");
      for (unsigned part_i = 0; part_i < nvtxs; part_i++) {
        printf("     %d     %d\n", part_i, (int) part[part_i]);
      }

    }

    void partGraphRecursiveTest(const fe::Mesh &mesh,
                                const std::vector<std::vector<size_t>> &nodeNeighs,
                                std::vector<size_t> &nodePartition,
                                size_t nPartitions = 4) {
      idx_t nvtxs = mesh.d_numNodes;
      idx_t ncon = 1; // # of balancing constraints (at least 1)
      idx_t objval;
      idx_t nWeights  = 1;
      std::vector<idx_t> part(nvtxs, 0);
      std::vector<idx_t> vwgt(nvtxs * nWeights, 0);
      idx_t nParts = idx_t(nPartitions);

      // create adjacency data based on nodeNeighs
      std::vector<idx_t> xadj(nvtxs, 0);
      std::vector<idx_t> adjncy;
      for (size_t i=0; i<mesh.d_numNodes; i++) {
        adjncy.insert(adjncy.end(), nodeNeighs[i].begin(), nodeNeighs[i].end());
        xadj[i+1] = xadj[i] + idx_t(nodeNeighs[i].size());
      }
      std::cout << fmt::format("adjcny size = {}, xadj[end] = {}\n",
                                adjncy.size(), xadj[nvtxs]);

      printf("\n");
      printf("partGraphRecursiveTest:\n");
      printf("  METIS_PartGraphRecursive partitions a graph into K parts\n");
      printf("  using multilevel recursive bisection.\n");

      int ret = METIS_PartGraphRecursive(&nvtxs, &ncon, xadj.data(), adjncy.data(), NULL, NULL,
                                         NULL, &nParts, NULL, NULL, NULL, &objval, part.data());

      printf("\n");
      printf("  Return code = %d\n", ret);
      printf("  Edge cuts for partition = %d\n", (int) objval);

      // cast the part vector into nodalPartition vector
      nodePartition.resize(0);
      nodePartition.insert(nodePartition.end(), part.begin(), part.end());
    }

    void partGraphKwayTest(const fe::Mesh &mesh,
                                const std::vector<std::vector<size_t>> &nodeNeighs,
                                std::vector<size_t> &nodePartition,
                                size_t nPartitions = 4) {
      idx_t nvtxs = mesh.d_numNodes;
      idx_t ncon = 1; // # of balancing constraints (at least 1)
      idx_t objval;
      idx_t nWeights  = 1;
      std::vector<idx_t> part(nvtxs, 0);
      std::vector<idx_t> vwgt(nvtxs * nWeights, 0);
      idx_t nParts = idx_t(nPartitions);

      // create adjacency data based on nodeNeighs
      std::vector<idx_t> xadj(nvtxs, 0);
      std::vector<idx_t> adjncy;
      for (size_t i=0; i<mesh.d_numNodes; i++) {
        adjncy.insert(adjncy.end(), nodeNeighs[i].begin(), nodeNeighs[i].end());
        xadj[i+1] = xadj[i] + idx_t(nodeNeighs[i].size());
      }
      std::cout << fmt::format("adjcny size = {}, xadj[end] = {}\n",
                               adjncy.size(), xadj[nvtxs]);

      printf("\n");
      printf("partGraphKwayTest:\n");
      printf("  METIS_PartGraphKway partitions a graph into K parts\n");
      printf("  using multilevel K-way partition.\n");

      int ret = METIS_PartGraphKway(&nvtxs, &ncon, xadj.data(), adjncy.data(), NULL, NULL,
                                         NULL, &nParts, NULL, NULL, NULL, &objval, part.data());

      printf("\n");
      printf("  Return code = %d\n", ret);
      printf("  Edge cuts for partition = %d\n", (int) objval);

      // cast the part vector into nodalPartition vector
      nodePartition.resize(0);
      nodePartition.insert(nodePartition.end(), part.begin(), part.end());
    }
} // namespace

void test::testGraphPartitioningSimple() {

  printf("\n");
  printf("METIS_TEST\n");
  printf("  Test the METIS library for graph partitioning (simple).\n");

  // perform basic test that shows metis is linked correctly with the code
  partGraphKwayTestSimple();
  partGraphRecursiveTestSimple();
}

void test::testGraphPartitioning(size_t nPart, size_t nGrid, size_t mHorizon, size_t testOption, std::string meshFilename) {

  std::cout << "\nMETIS_TEST\n";
  std::cout << "\n  Test the METIS library for graph partitioning for realistic mesh with nonlocal interaction.\n" ;
  std::cout << fmt::format("\n  Arguments: nPart = {}, nGrid = {}, mHorizon = {}\n", nPart, nGrid, mHorizon);


  // create uniform mesh on domain [0, Lx]x[0, Ly]
  auto t1 = steady_clock::now();

  // empty mesh object
  size_t dim(2);
  auto mesh = fe::Mesh(dim);
  mesh.d_spatialDiscretization = "finite_difference";

  // create mesh
  std::string outMeshFilename = "";
  if (testOption == 0)
    outMeshFilename = createUniformMesh(&mesh, nGrid);
  else if (testOption == 1)
    outMeshFilename = readMeshFromFile(&mesh, meshFilename);
  else {
    std::cerr << "testGraphPartitioning() accepts either 0 or 1 for testOption. The value "
              << testOption << " is invalid.\n";
    exit(1);
  }

  // set nonlocal lengthscale
  double epsilon = mHorizon*mesh.d_h;

  // print mesh data and write mesh to a file
  std::ostringstream msg;
  msg << mesh.printStr();
  std::cout << msg.str();

  auto t2 = steady_clock::now();
  auto setup_time = util::methods::timeDiff(t1, t2, "microseconds");
  std::cout << fmt::format("Setup time (ms) = {}. \n", setup_time);

  // create neighborhood of each node (to be used in metis partitioning of the graph)
  std::vector<std::vector<size_t>> nodeNeighs(mesh.d_numNodes);
  auto nsearch_p = std::make_unique<NSearch>(mesh.d_nodes);
  double set_tree_time = nsearch_p->updatePointCloud(mesh.d_nodes, true);
  set_tree_time += nsearch_p->setInputCloud();
  std::cout << fmt::format("Tree setup time (ms) = {}. \n", set_tree_time);

  for (size_t i=0; i<mesh.d_numNodes; i++) {
    std::vector<size_t> neighs;
    std::vector<double> sqr_dist;
    if (nsearch_p->radiusSearch(mesh.d_nodes[i], epsilon, neighs, sqr_dist) > 0) {
      for (std::size_t j = 0; j < neighs.size(); ++j)
        if (neighs[j] != i) {
          nodeNeighs[i].push_back(neighs[j]);
        }
    }
  }

  auto t3 = steady_clock::now();
  auto neigh_time = util::methods::timeDiff(t2, t3, "microseconds");
  std::cout << fmt::format("Neighborhood calculation time (ms) = {}.\n", neigh_time);

  // at this stage, we have mesh and nonlocal neighborhood
  // we are ready to cast the nonlocal neighborhood into graph and call metis for partitioning of nodes
  std::vector<size_t> nodePartitionRecursive(mesh.d_numNodes, 0);
  std::vector<size_t> nodePartitionKWay(mesh.d_numNodes, 0);

  // recursive method
  auto t4 = steady_clock::now();
  partGraphRecursiveTest(mesh, nodeNeighs, nodePartitionRecursive, nPart);
  auto t5 = steady_clock::now();

  // K-way method
  partGraphKwayTest(mesh, nodeNeighs, nodePartitionKWay, nPart);
  auto t6 = steady_clock::now();

  auto partition_recursive_time = util::methods::timeDiff(t4, t5, "microseconds");
  auto partition_kway_time = util::methods::timeDiff(t5, t6, "microseconds");
  std::cout << fmt::format("Partition (Recursive) calculation time (ms) = {}.\n", partition_recursive_time);
  std::cout << fmt::format("Partition (KWay) calculation time (ms) = {}.\n", partition_kway_time);

  // write data to file
  outMeshFilename = outMeshFilename + fmt::format("_mHorizon_{}_nPart_{}", mHorizon, nPart);
  std::cout << "out mesh filename = " << outMeshFilename << std::endl;
  auto writer = rw::writer::Writer(outMeshFilename, "vtu");
  writer.appendMesh(&mesh.d_nodes, mesh.d_eType, &mesh.d_enc);
  writer.appendPointData("Nodal_Volume", &mesh.d_vol);
  writer.appendPointData("Nodal_Partition_Metis_Recursive_Index", &nodePartitionRecursive);
  writer.appendPointData("Nodal_Partition_Metis_KWay_Index", &nodePartitionKWay);
  writer.close();
}
