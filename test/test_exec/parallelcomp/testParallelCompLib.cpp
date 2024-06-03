/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "testParallelCompLib.h"
#include "util/methods.h"
#include "util/randomDist.h"
#include "util/point.h"
#include "util/methods.h" // declares std::chrono and defines timeDiff()
#include "util/io.h"
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

  void printMsg(std::string msg, int mpiRank, int printMpiRank) {
    if (printMpiRank < 0) {
      std::cout << msg;
    } else {
      if (mpiRank == printMpiRank)
        std::cout << msg;
    }
  }

  double f1(const double &x){
    return x*x*x + std::exp(x) - std::sin(x);
  }

  double f2(const double &x){
    return 2*(x-0.5)*(x-0.5)*(x-0.5) + std::exp(x-0.5) - std::cos(x-0.5);
  }

  void setupOwnerAndGhost(size_t mpiSize,
          size_t mpiRank,
          const std::vector<size_t> &nodePartition,
          const std::vector<std::vector<size_t>> &nodeNeighs,
          std::vector<size_t> &ownedNodes,
          std::vector<size_t> &ownedInternalNodes,
          std::vector<size_t> &ownedBdryNodes,
          std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> &ghostData) {

    auto numNodes = nodePartition.size();

    // clear data
    ownedNodes.clear();
    ownedInternalNodes.clear();
    ownedBdryNodes.clear();
    ghostData.resize(mpiSize);
    for (size_t i_proc=0; i_proc<mpiSize; i_proc++) {
      ghostData[i_proc].first.clear();
      ghostData[i_proc].second.clear();
    }

    // setup
    for (size_t i=0; i<numNodes; i++) {
      if (nodePartition[i] == mpiRank) {
        // this processor owns this node
        ownedNodes.push_back(i);

        // ascertain if this node has neighboring nodes owned by other processors
        bool ghostExist = false;
        for (auto j : nodeNeighs[i]) {
          auto j_proc = nodePartition[j];
          if (j_proc != mpiRank) {
            ghostExist = true;

            // add j to ghost node list (to receive from j_proc)
            ghostData[j_proc].first.push_back(j);

            // add i to ghost node list (to send to j_proc)
            ghostData[j_proc].second.push_back(i);
          }
        }

        if (ghostExist)
          ownedBdryNodes.push_back(i);
        else
          ownedInternalNodes.push_back(i);
      } // loop over neighboring nodes
    } // loop over nodes

    //// DEBUG
    bool debugGhostData = false;
    if (debugGhostData and mpiRank == 0) {
      std::vector<std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>>> ghostDataAllProc(
              mpiSize);
      std::vector<std::vector<size_t>> numGhostDataAllProc(mpiSize);
      for (size_t i_proc = 0; i_proc < mpiSize; i_proc++) {
        numGhostDataAllProc[i_proc].resize(mpiSize);
        ghostDataAllProc[i_proc].resize(mpiSize);
      }

      for (size_t i_proc = 0; i_proc < mpiSize; i_proc++) {
        // assume we are i_proc and then create ghostData for i_proc
        for (size_t i = 0; i < numNodes; i++) {
          if (nodePartition[i] == i_proc) {
            // i_proc processor owns this node
            for (auto j: nodeNeighs[i]) {
              auto j_proc = nodePartition[j];
              if (j_proc != i_proc) {
                // add to ghost node list
                ghostDataAllProc[i_proc][j_proc].first.push_back(j);
              }
            }
          } // loop over neighboring nodes
        } // loop over nodes

        // total number of ghost nodes from neighboring processors
        for (size_t j_proc = 0; j_proc < mpiSize; j_proc++)
          numGhostDataAllProc[i_proc][j_proc] = ghostDataAllProc[i_proc][j_proc].first.size();
      } // loop over i_proc

      // print the information
      std::cout << "\n\nGhost data debug output\n\n";
      bool found_asym = false;
      for (size_t i_proc = 0; i_proc < mpiSize; i_proc++) {
        for (size_t j_proc = 0; j_proc < mpiSize; j_proc++) {
          std::cout << fmt::format("(i,j) = ({}, {}), num data = {}\n",
                                   i_proc, j_proc,
                                   numGhostDataAllProc[i_proc][j_proc]);

          if (j_proc > i_proc) {
            if (numGhostDataAllProc[i_proc][j_proc] == numGhostDataAllProc[j_proc][i_proc])
              std::cout << fmt::format("    symmetric: data ({}, {}) = data ({}, {})\n",
                                       i_proc, j_proc, j_proc, i_proc);
            else {
              found_asym = true;
              std::cout << fmt::format(
                      "    asymmetric: data ({}, {}) != data ({}, {})\n",
                      i_proc, j_proc, j_proc, i_proc);
            }
          }
        }
      }
      if (found_asym)
        std::cout << "Found asymetric ghost data\n";
      else
        std::cout << "No asymetric ghost data\n";

    }
    //// END OF DEBUG
  } // setupOwnerAndGhost()

  void exchangeDispData(size_t mpiSize,
                        size_t mpiRank,
                        const std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> &ghostData,
                        std::vector<std::pair<std::vector<util::Point>, std::vector<util::Point>>> &dispGhostData,
                        std::vector<util::Point> &dispNodes) {
    // resize dispGhostData if not done
    dispGhostData.resize(mpiSize);
    for (size_t j_proc = 0; j_proc < mpiSize; j_proc++) {
      auto j_data_size = ghostData[j_proc].first.size(); // same size for second
      dispGhostData[j_proc].first.resize(j_data_size);
      dispGhostData[j_proc].second.resize(j_data_size);
    }

    // exchange data
    util::io::print("\n\nBegin exchange data\n\n");
    util::io::print(fmt::format("\n\nThis processor's rank = {}\n\n", mpiRank), util::io::print_default_tab, -1);
    MPI_Request mpiRequests[2*(mpiSize-1)];
    size_t requestCounter = 0;
    for (size_t j_proc=0; j_proc<mpiSize; j_proc++) {
      auto & sendIds = ghostData[j_proc].second;
      auto & recvIds = ghostData[j_proc].first;

      if (j_proc != mpiRank and recvIds.size() != 0) {

        util::io::print(fmt::format("\n\n    Processing j_proc = {}\n\n", j_proc), util::io::print_default_tab, -1);

        // fill in ghost displacement data that we are sending from nodal displacement data
        for (size_t k = 0; k<sendIds.size(); k++)
          dispGhostData[j_proc].second[k] = dispNodes[sendIds[k]];

        // send data from this process to j_proc
        MPI_Isend(dispGhostData[j_proc].second.data(), 3*sendIds.size(),
                  MPI_DOUBLE, j_proc, 0, MPI_COMM_WORLD,
                  &mpiRequests[requestCounter++]);

        // receive data from j_proc
        MPI_Irecv(dispGhostData[j_proc].first.data(), 3*recvIds.size(),
                  MPI_DOUBLE, j_proc, 0, MPI_COMM_WORLD,
                  &mpiRequests[requestCounter++]);
      }
    } // loop over j_proc

    util::io::print("\n\nCalling MPI_Waitall\n\n");
    MPI_Waitall(requestCounter, mpiRequests, MPI_STATUSES_IGNORE);

    util::io::print("\n\nUpdate dispNodes data\n\n");

    for (size_t j_proc=0; j_proc<mpiSize; j_proc++) {
      auto & recvIds = ghostData[j_proc].first;
      if (j_proc != mpiRank and recvIds.size() != 0) {
        for (size_t k = 0; k<recvIds.size(); k++)
          dispNodes[recvIds[k]] = dispGhostData[j_proc].first[k];
      }
    } // loop over j_proc
  } // exchangeDispData()
} // namespace


std::string test::testTaskflow(size_t N, int seed) {

  auto nThreads = util::parallel::getNThreads();
  util::io::print(fmt::format("\n\ntestTaskflow(): Number of threads = {}\n\n", nThreads));

  // task: perform N computations in serial and using taskflow for_each

  // generate vector of random numbers
  RandGenerator gen(util::get_rd_gen(seed));
  auto dist = util::DistributionSample<UniformDistribution>(0., 1., seed);

  std::vector<double> x(N);
  std::vector<double> y1(N);
  std::vector<double> y2(N);
  std::generate(std::begin(x), std::end(x), [&] { return dist(); });

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
  tf::Executor executor(nThreads);
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

void test::testMPI(size_t nGrid, size_t mHorizon,
                   size_t testOption, std::string meshFilename) {
  int mpiSize, mpiRank;
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

  // number of partitions
  size_t nPart(mpiSize);

  // create uniform mesh
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
    util::io::print("\n\nCreating uniform mesh\n\n");
    fe::createUniformMesh(&mesh, dim, box, nGridVec);

    // filename for outputting
    outMeshFilename = fmt::format("uniform_mesh_Lx_{}_Ly_{}_Nx_{}_Ny_{}",
                                  box.second[0], box.second[1],
                                  nGridVec[0], nGridVec[1]);
  }
  else if (testOption == 2) {
    if (meshFilename.empty()) {
      std::cerr << "testGraphPartitioning(): mesh filename is empty.\n";
      exit(1);
    }

    // call in-built function of mesh to create data from file
    util::io::print("\n\nReading mesh\n\n");
    mesh.createData(meshFilename);

    // find the name of mesh file excluding path and extension
    outMeshFilename = util::io::removeExtensionFromFile(util::io::getFilenameFromPath(meshFilename));
  }
  else {
    std::cerr << "testMPI() accepts either 1 or 2 for testOption. The value "
              << testOption << " is invalid.\n";
    exit(1);
  }

  // print mesh data and write mesh to a file
  util::io::print(mesh.printStr());

  // calculate nonlocal neighborhood
  double horizon = mHorizon*mesh.d_h;
  std::vector<std::vector<size_t>> nodeNeighs(mesh.d_numNodes);
  geometry::computeNonlocalNeighborhood(mesh.d_nodes, horizon, nodeNeighs);

  // partition the mesh on root processor and broadcast to other processors
  mesh.d_nodePartition.resize(mesh.d_numNodes);
  util::io::print("\n\nCreating partition of mesh\n\n");
  if (mpiRank == 0)
    fe::metisGraphPartition("metis_kway", &mesh, nodeNeighs, nPart);

  util::io::print("\n\nBroadcasting partition to all processors\n\n");
  MPI_Bcast(mesh.d_nodePartition.data(), mesh.d_numNodes,
            MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);


  // Tasks:
  // 1. Create list of ghost nodes associated to neighboring processors
  // 2. Create a method that updates displacement of ghost nodes via MPI communication
  // 3. Create two set of nodes owned by this processor; internal set will have nodes
  // that do not depend on ghost nodes and boundary set that depend on ghost nodes

  // store id of nodes owned by this processor
  std::vector<size_t> ownedNodes, ownedInternalNodes, ownedBdryNodes;

  // for each neighboring processor, store id of ghost nodes owned by that processor
  std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> ghostData;

  // fill the owned and ghost node vectors
  util::io::print("\n\nCalling setupOwnerAndGhost()\n\n");
  setupOwnerAndGhost(mpiSize, mpiRank,
                     mesh.d_nodePartition, nodeNeighs,
                     ownedNodes, ownedInternalNodes, ownedBdryNodes,
                     ghostData);

  // create dummy displacement vector with random values
  std::vector<util::Point> dispNodes(mesh.d_numNodes, util::Point(-1., -1., -1.));
  {
    //  int seed = mpiRank;
    //  RandGenerator gen(util::get_rd_gen(seed));
    //  auto dist = util::DistributionSample<UniformDistribution>(0., 1., seed);
    //  for (auto i: ownedNodes)
    //    dispNodes[i] = util::Point(dist(), dist(), dist());
    // assign value to owned nodes
    for (auto i: ownedNodes)
      dispNodes[i] = util::Point(mpiRank + 1, (mpiRank + 1)*100, (mpiRank + 1)*10000);
  }

  // MPI communication to send and receive ghost nodes data
  printMsg("\n\nCalling exchangeDispData()\n\n", mpiRank, 0);
  std::vector<std::pair<std::vector<util::Point>, std::vector<util::Point>>> dispGhostData;
  exchangeDispData(mpiSize, mpiRank, ghostData, dispGhostData, dispNodes);

  //// DEBUG exchanged displacement data
  if (true) {
    printMsg("\n\nDebugging dispGhostData()\n\n", mpiRank, -1);
    bool debug_failed = false;
    for (size_t j_proc = 0; j_proc < mpiSize; j_proc++) {
      auto &recvIds = ghostData[j_proc].first;

      if (j_proc != mpiRank and recvIds.size() != 0) {
        for (size_t k = 0; k < recvIds.size(); k++) {
          auto &uk = dispNodes[recvIds[k]];

          // verify we received correct value of uk
          if (uk[0] != j_proc + 1 or uk[1] != 100 * (j_proc + 1)
              or uk[2] != 10000 * (j_proc + 1)) {
            debug_failed = true;
            util::io::print(fmt::format("    MPI exchange error: j_proc = {}, "
                                     "uk = ({}, {}, {})\n", j_proc,
                                     uk[0], uk[1], uk[2]),
                     util::io::print_default_tab, -1);
          }
        }
      }
    } // loop over j_proc

    if (debug_failed)
      util::io::print(fmt::format("\n\nDEBUG failed for processor = {}\n\n", mpiRank), util::io::print_default_tab, -1);
    else
      util::io::print(fmt::format("\n\nDEBUG passed for processor = {}\n\n", mpiRank), util::io::print_default_tab, -1);
  }
  //// END OF DEBUG
}
