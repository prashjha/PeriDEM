/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "meshPartitioning.h"
#include "mesh.h"
#include "util/methods.h" // declares std::chrono and defines timeDiff()

#include <metis.h>
#include <fmt/format.h>

void fe::metisGraphPartition(std::string partitionMethod,
                         const std::vector<std::vector<size_t>> &nodeNeighs,
                         std::vector<size_t> &nodePartition,
                         size_t nPartitions) {
  // record time
  auto t1 = steady_clock::now();
  idx_t nvtxs = nodeNeighs.size();
  idx_t ncon = 1; // # of balancing constraints (at least 1)
  idx_t objval;
  int metis_return;
  idx_t nWeights  = 1;
  std::vector<idx_t> part(nvtxs, 0);
  std::vector<idx_t> vwgt(nvtxs * nWeights, 0);
  auto nParts = idx_t(nPartitions);

  // create adjacency data based on nodeNeighs
  std::vector<idx_t> xadj(nvtxs, 0);
  std::vector<idx_t> adjncy;
  for (size_t i=0; i<nvtxs; i++) {
    adjncy.insert(adjncy.end(), nodeNeighs[i].begin(), nodeNeighs[i].end());
    xadj[i+1] = xadj[i] + idx_t(nodeNeighs[i].size());
  }
  std::cout << fmt::format("adjcny size = {}, xadj[end] = {}\n",
                           adjncy.size(), xadj[nvtxs]);

  std::cout << "\nmetisGraphPartition():\n";
  if (partitionMethod == "metis_recursive") {
    std::cout << "  METIS_PartGraphRecursive partitions a graph into K parts\n";
    std::cout << "  using multilevel recursive bisection.\n";

    metis_return = METIS_PartGraphRecursive(&nvtxs, &ncon, xadj.data(),
                                       adjncy.data(), NULL, NULL,
                                       NULL, &nParts, NULL, NULL, NULL, &objval,
                                       part.data());
  } else if (partitionMethod == "metis_kway") {
    std::cout << "  METIS_PartGraphKway partitions a graph into K parts\n";
    std::cout << "  using multilevel K-way partition.\n";

    metis_return = METIS_PartGraphKway(&nvtxs, &ncon, xadj.data(),
                                       adjncy.data(), NULL, NULL,
                                       NULL, &nParts, NULL, NULL, NULL, &objval,
                                       part.data());
  } else {
    std::cerr << "Argument partitionMethod = "
              << partitionMethod << " is invalid.\n"
              << "Valid values are {'metis_recursive', 'metis_kway'}.\n";
    exit(1);
  }

  // record time
  auto t2 = steady_clock::now();

  std::cout << fmt::format("\n  Return code = {}\n"
                           "  Edge cuts for partition = {}\n"
                           "  Partition calculation time (ms) = {}\n",
                           metis_return, (int) objval,
                           util::methods::timeDiff(t1, t2, "microseconds"));

  // cast the part vector into nodePartition vector
  nodePartition.resize(0);
  nodePartition.insert(nodePartition.end(), part.begin(), part.end());
}

void fe::metisGraphPartition(std::string partitionMethod,
                         fe::Mesh *mesh_p,
                         const std::vector<std::vector<size_t>> &nodeNeighs,
                         size_t nPartitions) {
  mesh_p->d_nPart = nPartitions;
  mesh_p->d_partitionMethod = partitionMethod;
  fe::metisGraphPartition(partitionMethod, nodeNeighs,
                          mesh_p->d_nodePartition, nPartitions);
}
