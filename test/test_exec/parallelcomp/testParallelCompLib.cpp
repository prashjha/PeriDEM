/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testParallelCompLib.h"
#include "util/methods.h"
#include "fe/mesh.h"
#include "fe/meshPartitioning.h"
#include "fe/meshUtil.h"
#include "geometry/geometryUtil.h"
#include <mpi.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

typedef std::mt19937 RandGenerator;
typedef std::uniform_real_distribution<> UniformDistribution;

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

namespace {

  RandGenerator get_rd_gen(int seed) {

    // return RandGenerator();

    if (seed < 0) {
      std::random_device rd;
      seed = rd();
    }

    return RandGenerator(seed);
  }

  double f1(const double &x){
    return x*x*x + std::exp(x) - std::sin(x);
  }

  double f2(const double &x){
    return 2*(x-0.5)*(x-0.5)*(x-0.5) + std::exp(x-0.5) - std::cos(x-0.5);
  }
}


std::string test::testTaskflow(size_t N, int seed) {

  // task: perform N computations in serial and using taskflow for_each

  // generate vector of random numbers
  RandGenerator gen(get_rd_gen(seed));
  UniformDistribution dist(0, 1);

  std::vector<double> x(N);
  std::vector<double> y1(N);
  std::vector<double> y2(N);
  std::generate(std::begin(x), std::end(x), [&] { return dist(gen); });

  // now do serial calculation
  auto t1 = steady_clock::now();
  for (size_t i = 0; i < N; i++) {
    if (x[i] < 0.5)
      y1[i] = f1(x[i]);
    else
      y1[i] = f2(x[i]);
  }
  auto t2 = steady_clock::now();
  auto dt12 = util::methods::timeDiff(t1, t2, "microseconds");

  // now do parallel calculation using taskflow
  tf::Executor executor;
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, N, (std::size_t) 1, [&x, &y2](std::size_t i) {
      if (x[i] < 0.5)
        y2[i] = f1(x[i]);
      else
        y2[i] = f2(x[i]);
                          }
  ); // for_each

  executor.run(taskflow).get();
  auto t3 = steady_clock::now();
  auto dt23 = util::methods::timeDiff(t2, t3, "microseconds");

  // compare results
  double y_err = 0.;
  for (size_t i=0; i<N; i++)
    y_err += std::pow(y1[i] - y2[i], 2);

  if (y_err > 1.e-10) {
    std::cerr << fmt::format("Error: Serial and taskflow computation results do not match (squared error = {})\n",
                            y_err);
    exit(1);
  }

  // get time
  std::ostringstream msg;
  msg << fmt::format("  Serial computation took = {}ms\n", dt12);
  msg << fmt::format("  Taskflow computation took = {}ms\n", dt23);
  msg << fmt::format("  Speed-up factor = {}\n\n\n", dt12/dt23);

  return msg.str();
}

void test::testMPI(size_t nGrid, size_t mHorizon) {
  int mpiSize, mpiRank;
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

  // create uniform mesh
  size_t dim(2);
  auto mesh = fe::Mesh(dim);
  mesh.d_spatialDiscretization = "finite_difference";

  // set geometry details
  std::pair<std::vector<double>, std::vector<double>> box;
  std::vector<size_t> nGridVec;
  for (size_t i = 0; i < dim; i++) {
    box.first.push_back(0.);
    box.second.push_back(1.);
    nGridVec.push_back(nGrid);
  }

  // call utility function to create mesh
  fe::createUniformMesh(&mesh, dim, box, nGridVec);

  // print mesh data and write mesh to a file
  std::ostringstream msg;
  msg << mesh.printStr();
  std::cout << msg.str();

  // calculate nonlocal neighborhood
  double horizon = mHorizon*mesh.d_h;
  std::vector<std::vector<size_t>> nodeNeighs(mesh.d_numNodes);
  geometry::computeNonlocalNeighborhood(mesh.d_nodes, horizon, nodeNeighs);

  // filename for outputting
  std::string outMeshFilename = fmt::format("uniform_mesh_Lx_{}_Ly_{}_Nx_{}_Ny_{}",
                                            box.second[0], box.second[1],
                                            nGridVec[0], nGridVec[1]);

  // partition the mesh
  size_t nPart(mpiSize);
  fe::metisGraphPartition("metis_recursive", &mesh, nodeNeighs, nPart);

  // Tasks:
  // 1. Create list of ghost nodes associated to neighboring processors
  // 2. Create a method that updates displacement of ghost nodes via MPI communication
  // 3. Create two set of nodes owned by this processor; internal set will have nodes
  // that do not depend on ghost nodes and boundary set that depend on ghost nodes

  // store id of nodes owned by this processor
  std::vector<size_t> ownedNodes, ownedInternalNodes, ownedBdryNodes;

  // for each neighboring processor, store id of ghost nodes owned by that processor
  std::vector<std::vector<size_t>> ghostNodes(nPart);
  std::vector<std::vector<double>> dispGhostNodes(nPart);

  // fill the owned and ghost node vectors
  {
    for (size_t i=0; i<mesh.d_numNodes; i++) {
      if (mesh.d_nodePartition[i] == mpiRank) {
        // this processor owns this node
        ownedNodes.push_back(i);

        // ascertain if this node has neighboring nodes owned by other processors
        bool ghostExist = false;
        for (auto j : nodeNeighs[i]) {
          auto j_proc = mesh.d_nodePartition[j];
          if (j_proc != mpiRank) {
            ghostExist = true;
            // add to ghost node list
            ghostNodes[j_proc].push_back(j);
            dispGhostNodes[j_proc].insert(dispGhostNodes[j_proc].end(), {0., 0., 0.});
          }
        }

        if (ghostExist)
          ownedBdryNodes.push_back(i);
        else
          ownedInternalNodes.push_back(i);
      } // loop over neighboring nodes
    } // loop over nodes
  }

  // create dummy displacement vector with random values
  std::vector<util::Point> dispNodes(mesh.d_numNodes);

}
