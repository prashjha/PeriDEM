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


int main(int argc, char *argv[]) {

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-i")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <vector-size>" << std::endl;
    //exit(EXIT_FAILURE);
  }

  // read input file
  size_t N;
  if (input.cmdOptionExists("-i")) N = std::stoi(input.getCmdOption("-i"));
  else {
    std::cout << "Running test with default vector-size = 100000\n";
    N = 100000;
  }

  // test
  std::vector<size_t> N_test = {size_t(N/100), size_t(N/10), N, 10*N, 100*N};
  int seed = 0;
  size_t test_count = 0;
  for (auto n : N_test) {
    std::cout << "**** Test number = " << test_count++ << " ****\n";
    std::cout << fmt::format("Test parameters: N = {}\n\n", n);
    auto msg = test::testTaskflow(n, seed);
    std::cout << msg;
  }

  return EXIT_SUCCESS;
}
