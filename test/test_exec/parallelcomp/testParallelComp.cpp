/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <PeriDEMConfig.h>
#include "testParallelCompLib.h"
#include "util/io.h"
#include <fmt/format.h>
#include <iostream>
#include <mpi.h>

int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);
  int mpiSize, mpiRank;
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  std::cout << fmt::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank);

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-o")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -o <test-option; 0 - taskflow, 1 - mpi> -i <vector-size> -n <grid-size> -m <horizon-integer-factor>" << std::endl;
    std::cout << "To test taskflow, run\n";
    std::cout << argv[0] << "-o 0 -i 10000\n";
    std::cout << "To test mpi, run\n";
    std::cout << argv[0] << "-o 1 -n 50 -m 3\n";
    exit(EXIT_FAILURE);
  }

  // read input file
  size_t testOption(-1);

  if (input.cmdOptionExists("-o")) testOption = std::stoi(input.getCmdOption("-o"));

  // test
  if (testOption == 0) {
    std::cout << "\nTesting taskflow\n\n";

    size_t nTaskflow;
    if (input.cmdOptionExists("-i")) nTaskflow = std::stoi(input.getCmdOption("-i"));
    else {
      std::cout << "Running test with default vector-size = 100000\n";
      nTaskflow = 100000;
    }

    std::vector<size_t> N_test = {size_t(nTaskflow/100), size_t(nTaskflow/10), nTaskflow, 10*nTaskflow, 100*nTaskflow};
    int seed = 0;
    size_t test_count = 0;
    for (auto n : N_test) {
      std::cout << "**** Test number = " << test_count++ << " ****\n";
      std::cout << fmt::format("Test parameters: N = {}\n\n", n);
      auto msg = test::testTaskflow(n, seed);
      std::cout << msg;
    }
  } else if (testOption == 1) {
    std::cout << "\nTesting MPI parallelization\n\n";
    if (mpiSize < 2) {
      std::cout << "\nNo tests for mpiSize = 1. Skipping this test.\n\n";
    } else {
      size_t nGrid, mHorizon;
      if (input.cmdOptionExists("-n"))
        nGrid = size_t(std::stoi(input.getCmdOption("-n")));
      else {
        std::cout << "Running test with default grid size = 50\n";
        nGrid = 50;
      }

      if (input.cmdOptionExists("-m"))
        mHorizon = size_t(std::stoi(input.getCmdOption("-m")));
      else {
        std::cout
                << "Running test with default integer factor for horizon = 3\n";
        mHorizon = 3;
      }

      // call test function
      test::testMPI(nGrid, mHorizon);
    }
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}
