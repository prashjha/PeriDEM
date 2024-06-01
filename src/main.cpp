/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <PeriDEMConfig.h>

// std includes
#include <iostream>
#include <algorithm>
#include <ctime>

// PeriDEM includes
#include "inp/input.h"                          // Input class
#include "model/dem/demModel.h"                 // Model class
#include "util/io.h"                            // InputParser class
#include "util/mpiUtil.h"                       // MPI-related functions
#include "util/methods.h"
#include <fmt/format.h>

int main(int argc, char *argv[]) {

  // init mpi
  util::mpi::initMpi(argc, argv);
  int mpiSize = util::mpi::mpiSize(), mpiRank = util::mpi::mpiRank();
  util::io::print(fmt::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::mpi::getMpiStatus()->printStr());

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-i")) {
    // print help
    std::cout << "Syntax to run PeriDEM: PeriDEM -i <input file> -n <number of threads>" << std::endl;
    std::cout << "Example: PeriDEM -i input.yaml -n 4" << std::endl;
    exit(EXIT_FAILURE);
  }
  
  // print program version
  std::cout << "PeriDEM"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl;

  // current time
  auto begin = steady_clock::now();

  // read input data
  std::string filename = input.getCmdOption("-i");
  auto *deck = new inp::Input(filename);

  // check which model to run
  if (deck->isPeriDEM()) {
    model::DEMModel dem(deck);
    dem.run(deck);
  } else {
    std::cout << "PeriDEM model not found in input file.\n";
  }

  // get time elapsed
  auto end = steady_clock::now();

  std::cout << "Total simulation time (s) = " 
            << util::methods::timeDiff(begin, end, "seconds") 
            << std::endl;
}
