/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "testMeshPartitioningLib.h"
#include "fe/mesh.h"
#include "fe/meshPartitioning.h"
#include "fe/meshUtil.h"
#include "util/point.h"
#include "util/vecMethods.h"
#include "util/io.h"
#include "rw/writer.h"
#include "geom/geomIncludes.h"

#include <metis.h>
#include <format>
#include <print>
#include <algorithm>
#include <fstream>
#include <string>


namespace {

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
  std::cout << std::format("\n  Arguments: nPart = {}, nGrid = {}, mHorizon = {}\n", nPart, nGrid, mHorizon);

  // create uniform mesh on domain [0, Lx]x[0, Ly]
  auto t1 = steady_clock::now();

  // empty mesh object
  size_t dim(2);
  auto mesh = fe::Mesh(dim);
  mesh.d_spatialDiscretization = "finite_difference";

  // create mesh
  std::string outMeshFilename = "";
  if (testOption == 1) {
    // set geometry details
    std::pair<std::vector<double>, std::vector<double>> box;
    std::vector<size_t> nGridVec;
    for (size_t i=0; i<dim; i++) {
      box.first.push_back(0.);
      box.second.push_back(1.);
      nGridVec.push_back(nGrid);
    }

    // call utility function to create mesh
    fe::createUniformMesh(&mesh, dim, box, nGridVec);

    // filename for outputting
    outMeshFilename = std::format("uniform_mesh_Lx_{}_Ly_{}_Nx_{}_Ny_{}",
                                               box.second[0], box.second[1],
                                               nGridVec[0], nGridVec[1]);
  }
  else if (testOption == 2) {
    if (meshFilename.empty()) {
      std::cerr << "testGraphPartitioning(): mesh filename is empty.\n";
      exit(1);
    }

    // call in-built function of mesh to create data from file
    mesh.createData(meshFilename);

    // find the name of mesh file without path and without extension (i.e., remove .vtu/.msh/.csv extension)
    auto f1 = util::io::getFilenameFromPath(meshFilename);
    outMeshFilename = util::io::removeExtensionFromFile(f1);
  }
  else {
    std::cerr << "testGraphPartitioning() accepts either 0 or 1 for testOption. The value "
              << testOption << " is invalid.\n";
    exit(1);
  }

  // set nonlocal lengthscale
  double horizon = mHorizon*mesh.d_h;

  // print mesh data and write mesh to a file
  std::ostringstream msg;
  msg << mesh.printStr();
  std::cout << msg.str();

  auto t2 = steady_clock::now();
  auto setup_time = util::methods::timeDiff(t1, t2, "microseconds");
  std::cout << std::format("Setup time (ms) = {}. \n", setup_time);

  // create neighborhood of each node (to be used in metis partitioning of the graph)
  std::vector<std::vector<size_t>> nodeNeighs(mesh.d_numNodes);
  geom::computeNonlocalNeighborhood(mesh.d_nodes, horizon, nodeNeighs);
  auto t3 = steady_clock::now();
  auto neigh_time = util::methods::timeDiff(t2, t3, "microseconds");
  std::cout << std::format("Neighborhood calculation time (ms) = {}.\n", neigh_time);

  // at this stage, we have mesh and nonlocal neighborhood
  // we are ready to cast the nonlocal neighborhood into graph and call metis for partitioning of nodes
  std::vector<size_t> nodePartitionRecursive(mesh.d_numNodes, 0);
  std::vector<size_t> nodePartitionKWay(mesh.d_numNodes, 0);

  // recursive method
  auto t4 = steady_clock::now();
  fe::metisGraphPartition("metis_recursive", nodeNeighs, nodePartitionRecursive, nPart);
  auto t5 = steady_clock::now();

  // K-way method
  fe::metisGraphPartition("metis_kway", nodeNeighs, nodePartitionKWay, nPart);
  auto t6 = steady_clock::now();

  auto partition_recursive_time = util::methods::timeDiff(t4, t5, "microseconds");
  auto partition_kway_time = util::methods::timeDiff(t5, t6, "microseconds");
  std::cout << std::format("Partition (Recursive) calculation time (ms) = {}.\n", partition_recursive_time);
  std::cout << std::format("Partition (KWay) calculation time (ms) = {}.\n", partition_kway_time);

  // write data to file
  outMeshFilename = outMeshFilename + std::format("_mHorizon_{}_nPart_{}", mHorizon, nPart);
  std::cout << "out mesh filename = " << outMeshFilename << std::endl;
  auto writer = rw::writer::Writer(outMeshFilename, "vtu");
  writer.appendMesh(&mesh.d_nodes, mesh.d_eType, &mesh.d_enc);
  writer.appendPointData("Nodal_Volume", &mesh.d_vol);
  writer.appendPointData("Nodal_Partition_Metis_Recursive_Index", &nodePartitionRecursive);
  writer.appendPointData("Nodal_Partition_Metis_KWay_Index", &nodePartitionKWay);
  writer.close();
}
