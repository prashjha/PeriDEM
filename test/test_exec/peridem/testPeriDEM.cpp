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
#include "testPeriDEMLib.h"
#include "util/io.h"
#include "util/parallelUtil.h"                       // MPI-related functions
#include <format>
#include <iostream>

int main(int argc, char *argv[]) {

  // init parallel
  util::parallel::initMpi(argc, argv);
  int mpiSize = util::parallel::mpiSize(), mpiRank = util::parallel::mpiRank();
  util::io::print(std::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::parallel::getMpiStatus()->printStr());

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-i")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <data-filepath> -nThreads <number of threads to be used in taskflow>" << std::endl;
    exit(EXIT_FAILURE);
  }

  // read input file
  std::string filepath = input.getCmdOption("-i");

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = std::thread::hardware_concurrency();
    util::io::print(std::format("Running test with default number of threads = {}\n", nThreads));
  }
  // set number of threads
  util::parallel::initNThreads(nThreads);
  util::io::print(std::format("Number of threads = {}\n", util::parallel::getNThreads()));

  // run test
  auto msg = test::testPeriDEM(filepath);

  if (msg == "pass")
    std::cout << "testPeriDEM: Pass\n";
  else {
    std::cerr << "Error: " << msg << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
