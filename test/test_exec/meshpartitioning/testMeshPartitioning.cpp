/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testMeshPartitioningLib.h"
#include <PeriDEMConfig.h>
#include <iostream>
#include "util/io.h"                            // InputParser class

int main(int argc, char *argv[]) {

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-o")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -o <0 to test on uniform mesh and 1 to test on user mesh> -i <grid-size> -p <number-partitions> -m <horizon-integer-factor> -f <mesh-filename>" << std::endl;
    std::cout << "To test on uniform mesh" << std::endl;
    std::cout << argv[0] << " -o 0 -i 10 -p 4 -m 3" << std::endl;
    std::cout << "To test on user-provided mesh (filename = filepath/meshfile.vtu)" << std::endl;
    std::cout << argv[0] << " -o 1 -p 4 -m 3 -f filepath/meshfile.vtu" << std::endl;
    exit(EXIT_FAILURE);
  }

  // read input
  size_t testOption, nGrid, nPart, mHorizon;
  std::string meshFilename;

  if (input.cmdOptionExists("-o")) testOption = size_t(std::stoi(input.getCmdOption("-o")));
  else {
    std::cerr << "Test requires following arguments -o <option integer> -i <grid-size> -p <number-partition> -m <horizon-integer-factor> -f <mesh-filename>\n";
    exit(1);
  }

  if (input.cmdOptionExists("-i")) nGrid = size_t(std::stoi(input.getCmdOption("-i")));
  else {
    std::cout << "Running test with default grid-size = 10\n";
    nGrid = 10;
  }

  if (input.cmdOptionExists("-f")) meshFilename = input.getCmdOption("-f");
  else {
    std::cout << "Running test on uniform mesh only\n";
    meshFilename = "null";
  }

  // check if nGrid and meshFilename are compatible
  if ((nGrid > 0 and testOption == 1) or (meshFilename == "null" and testOption == 0)) {
    std::cerr << "Please specify either using uniform mesh (in-built) or user-defined mesh "
                 "to perform the partitioning test. "
                 "That is, either specify '-o 0 -i <grid-size>' "
                 "or '-o 1 -f <mesh-filename>'.\n";
    exit(1);
  }

  if (input.cmdOptionExists("-p")) nPart = size_t(std::stoi(input.getCmdOption("-p")));
  else {
    std::cout << "Running test with default number of partitions = 4\n";
    nPart = 4;
  }

  if (input.cmdOptionExists("-m")) mHorizon = size_t(std::stoi(input.getCmdOption("-m")));
  else {
    std::cout << "Running test with default integer factor for horizon = 3\n";
    mHorizon = 3;
  }

  // test partitioning
  std::cout << "testMeshPartitioning: Simple test of metis graph partitioning\n\n";
  test::testGraphPartitioningSimple();

  std::cout << "\n\ntestMeshPartitioning: Test of metis graph partitioning on 2-D mesh with nonlocal interaction\n\n";
  test::testGraphPartitioning(nPart, nGrid, mHorizon, testOption, meshFilename);

  return EXIT_SUCCESS;
}
