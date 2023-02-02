/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <PeriDEMConfig.h>
#include "testNSearchLib.h"
#include "util/io.h"
#include <iostream>


int main(int argc, char *argv[]) {

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-i")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <num-points>" << std::endl;
    //exit(EXIT_FAILURE);
  }

  // read input file
  int N;
  if (input.cmdOptionExists("-i")) N = input.getCmdOption("-i");
  else {
    std::cout << "Running test with default num-points = 20\n";
    N = 20;
  }

  // test
  std::vector<double> L_test = {1., 1000., 0.01};
  std::vector<double> dL_test = {0.2, 0.5};
  std::vector<int> seeds = {1093};//, 13828, 78474};
  std::vector<int> N_test = {N};
  int test_count = 0;
  for (auto L : L_test)
    for (auto dL : dL_test)
      for (auto seed : seeds)
        for (auto n : N_test) {
          std::cout << "**** Test number = " << test_count++ << " ****\n";
          std::cout << fmt::format("Test parameters: L = {}, lattice "
                                   "perturbation = {}, seed = {}, N = {}\n\n",
                                   L, dL * L, seed, n);
          auto msg = test::testNanoflann(n, L, dL*L, seed);
          std::cout << msg;
        }

  return EXIT_SUCCESS;
}
