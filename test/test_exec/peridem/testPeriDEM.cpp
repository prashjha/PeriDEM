/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <PeriDEMConfig.h>
#include "testPeriDEMLib.h"
#include "util/io.h"
#include "util/mpiUtil.h"                       // MPI-related functions
#include <fmt/format.h>
#include <iostream>

int main(int argc, char *argv[]) {

  // init mpi
  util::mpi::initMpi(argc, argv);
  int mpiSize = util::mpi::mpiSize(), mpiRank = util::mpi::mpiRank();
  util::io::print(fmt::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::mpi::getMpiStatus()->printStr());

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-i")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <data-filepath>" << std::endl;
    exit(EXIT_FAILURE);
  }

  // read input file
  std::string filepath = input.getCmdOption("-i");

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
