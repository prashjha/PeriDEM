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
#include <hpx/hpx_main.hpp>
#include <iostream>
#include <boost/program_options.hpp>            // program options


int main(int argc, char *argv[]) {

  boost::program_options::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "num-points,i", boost::program_options::value<size_t>(),
      "Configuration file");

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  // read input file
  size_t N;
  if (vm.count("num-points")) N = vm["num-points"].as<size_t>();
  else {
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <num-points>" << std::endl;
    std::cout << "Runing test with num-points = 20\n";
    N = 20;
  }

  //
  // test transformation
  //
  //test::testNSearch(N);
  std::vector<double> L_test = {1., 1000., 0.01};
  std::vector<double> dL_test = {0.2, 0.5};
  std::vector<int> seeds = {1093};//, 13828, 78474};
  std::vector<int> N_test = {10, 20, 40};
  int test_count = 0;
  for (auto L : L_test)
    for (auto dL : dL_test)
      for (auto seed : seeds)
        for (auto N : N_test) {
          std::cout << "**** Test number = " << test_count++ << " ****\n";
          std::cout << fmt::format("Test parameters: L = {}, dL = {}, seed = {}, N = {}\n\n", L, dL, seed, N);
          auto msg = test::testNanoflann(N, L, dL*L, seed);
          std::cout << msg;
        }

  return EXIT_SUCCESS;
}
