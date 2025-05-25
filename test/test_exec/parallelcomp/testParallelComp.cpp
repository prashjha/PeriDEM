/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include <PeriDEMConfig.h>
#include "testParallelCompLib.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include <format>
#include <print>
#include <iostream>

int main(int argc, char *argv[]) {

  // init parallel
  util::parallel::initMpi(argc, argv);
  int mpiSize = util::parallel::mpiSize(), mpiRank = util::parallel::mpiRank();
  util::io::print(std::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::parallel::getMpiStatus()->printStr());

  // init logger
  util::io::initLogger();

  // parse input arguments
  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-o")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -o <test-option; 0 - taskflow, 1 - parallel on in-built mesh, 2 - user-defined mesh>"
                  " -i <vector-size> -n <grid-size>"
                  " -m <horizon-integer-factor>"
                  " -nThreads <number of threads to be used in taskflow>" << std::endl;
    std::cout << "To test taskflow, run\n";
    std::cout << argv[0] << " -o 0 -i 10000"
                            " -nThreads <number of threads to be used in taskflow>\n";
    std::cout << "To test parallel using in-built mesh, run\n";
    std::cout << argv[0] << " -o 1 -m 4 -n 50\n";
    std::cout << "To test parallel on user-provided mesh (filename = filepath/meshfile.vtu)" << std::endl;
    std::cout << argv[0] << " -o 2 -m 4 -f filepath/meshfile.vtu" << std::endl;
    exit(EXIT_FAILURE);
  }

  // read input file
  size_t testOption(-1);

  if (input.cmdOptionExists("-o")) testOption = std::stoi(input.getCmdOption("-o"));

  // test
  if (testOption == 0) {
    util::io::print("\nTesting taskflow\n\n");

    size_t nTaskflow;
    if (input.cmdOptionExists("-i")) nTaskflow = std::stoi(input.getCmdOption("-i"));
    else {
      nTaskflow = 100000;
      util::io::print(std::format("Running test with default vector-size = {}\n", nTaskflow));
    }

    unsigned int nThreads;
    if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
    else {
      nThreads = std::thread::hardware_concurrency();
      util::io::print(std::format("Running test with default number of threads = {}\n", nThreads));
    }
    // set number of threads
    util::parallel::initNThreads(nThreads);
    util::io::print(std::format("Number of threads = {}\n", util::parallel::getNThreads()));

    std::vector<size_t> N_test = {size_t(nTaskflow/100), size_t(nTaskflow/10), nTaskflow, 10*nTaskflow, 100*nTaskflow};
    int seed = 0;
    size_t test_count = 0;
    for (auto n : N_test) {
      util::io::print(std::format("**** Test number = {} ****\n"
                                  "Test parameters: N = {}\n\n",
                                  test_count++, n));
      auto msg = test::testTaskflow(n, seed);
      util::io::print(msg);
    }
  } else if (testOption == 1 or testOption == 2) {

    util::io::print("\nTesting MPI parallelization\n\n");

    if (mpiSize < 2) {
      std::cout << "\nNo tests for mpiSize = 1. Skipping this test.\n\n";
    } else {
      // read arguments
      size_t nGrid(0), mHorizon;
      std::string meshFilename("");

      if (input.cmdOptionExists("-n"))
        nGrid = size_t(std::stoi(input.getCmdOption("-n")));
      else {
        // set only if we are running test using in-built mesh
        if (testOption == 1) {
          nGrid = 50;
          util::io::print(std::format("Running test with default grid size = {}\n", nGrid));
        }
      }

      if (input.cmdOptionExists("-m"))
        mHorizon = size_t(std::stoi(input.getCmdOption("-m")));
      else {
        mHorizon = 4;
        util::io::print(std::format("Running test with default integer factor for horizon = {}\n", mHorizon));
      }

      // read filename
      if (input.cmdOptionExists("-f")) meshFilename = input.getCmdOption("-f");

      // check if nGrid and meshFilename are compatible
      if ((nGrid > 0 and testOption == 2) or
          (!meshFilename.empty() and testOption == 1)) {
        std::cerr
                << "Please specify either using uniform mesh (in-built) or user-defined mesh "
                   "to perform the MPI test. "
                   "That is, either specify '-o 1 -n <grid-size>' "
                   "or '-o 2 -f <mesh-filename>'.\n";
        exit(1);
      }

      // call test function
      test::testMPI(nGrid, mHorizon, testOption, meshFilename);
    }
  } else {
    util::io::print("Invalid option -o argument.\n");
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}
