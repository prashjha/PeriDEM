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

int main(int argc, char *argv[]) {

  InputParser input(argc, argv);

  if (input.cmdOptionExists("-h")) {
    // print help
    std::cout << "Syntax to run PeriDEM: ./PeriDEM -i <input file> -n <number of threads>"
    std::cout << "Example: ./PeriDEM -i input.yaml -n 4"
  }
  
  // print program version
  std::cout << "PeriDEM"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl;

  // current time
  auto begin = std::clock();

  // read input data
  filename = input.getCmdOption("-f");
  auto *deck = new inp::Input(filename);

  // check which model to run
  if (deck->isPeriDEM()) {
    model::DEMModel dem(deck);
    dem.run(deck);
  } else {
    std::cout << "PeriDEM model not found in input file.\n";
  }

  // get time elapsed
  auto end = std::clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

  std::cout << "Total simulation time (s) = " << elapsed_secs
            << std::endl;
}
