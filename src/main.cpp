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

// std includes
#include <iostream>
#include <algorithm>
#include <ctime>
#include <filesystem>

// PeriDEM includes
#include "inp/input.h"                          // Input class
#include "model/dem/demModel.h"                 // Model class
#include "util/io.h"                            // InputParser class
#include "util/parallelUtil.h"                       // MPI-related functions
#include "util/methods.h"
#include <fmt/format.h>

int main(int argc, char *argv[]) {

  // init parallel
  util::parallel::initMpi(argc, argv);
  int mpiSize = util::parallel::mpiSize(), mpiRank = util::parallel::mpiRank();
  util::io::print(fmt::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::parallel::getMpiStatus()->printStr());

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-i")) {
    // print help
    std::cout << "Syntax to run PeriDEM: PeriDEM -i <input file> -nThreads <number of threads>" << std::endl;
    std::cout << "Example: PeriDEM -i input.yaml -nThreads 4" << std::endl;
    exit(EXIT_FAILURE);
  }

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = std::thread::hardware_concurrency();
    util::io::print(fmt::format("Running test with default number of threads = {}\n", nThreads));
  }
  // set number of threads
  util::parallel::initNThreads(nThreads);
  util::io::print(fmt::format("Number of threads = {}\n", util::parallel::getNThreads()));
  
  // print program version
  std::cout << "PeriDEM"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl;

  // current time
  auto begin = steady_clock::now();

  // read input data
  std::string filename = input.getCmdOption("-i");
  if (!std::filesystem::exists(filename)) {
    throw std::runtime_error(fmt::format("Input file {} does not exist.", filename));
  }
  std::ifstream f(filename);
  auto j = json::parse(f);
  auto deck = std::make_shared<inp::Input>(j);

  // run model
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
