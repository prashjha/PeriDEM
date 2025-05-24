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
#include "testNSearchLib.h"
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
              << ") -i <num-points> -o <select-test; 0 - test with different lattice, 1 - profile nanoflann, 2 - test closest point search>" << std::endl;
    //exit(EXIT_FAILURE);
  }

  // read input file
  int N;
  if (input.cmdOptionExists("-i")) N = std::stoi(input.getCmdOption("-i"));
  else {
    N = 20;
    std::cout << "Running test with default num-points = " << N << "\n";
  }

  int testSelect;
  if (input.cmdOptionExists("-o")) testSelect = std::stoi(input.getCmdOption("-o"));
  else {
    testSelect = 0;
    std::cout << "Running test with default test selection = " << testSelect << "\n";
  }

  std::vector<int> leafMaxSizes = {2, 5, 8, 10, 12, 15, 20};
  std::vector<double> L_test = {1., 0.01};
  std::vector<double> dL_test = {0.2, 0.5};
  std::vector<int> seeds = {1093};//, 13828, 78474};
  std::vector<int> N_test = {N};
  std::vector<int> dims{2, 3};
  std::vector<int> numTags{4, 8, 12};
  int test_count = 0;


  if (testSelect == 0) {
    // test 1
    std::cout << "\n\nTesting Exclude and Include tag for different lattice sizes\n\n";

    for (auto L: L_test) {
      for (auto dL: dL_test) {
        for (auto seed: seeds) {
          for (auto n: N_test) {
            for (auto dim: dims) {

              std::string msg;
              test::testNSearchData data;
              data.d_dim = dim;
              data.d_numTags = numTags[0];
              data.d_leafMaxSize = leafMaxSizes[0];

              std::cout << "\n**** Test number = " << test_count++ << " ****\n";
              std::cout << std::format("Test parameters: L = {}, lattice "
                                       "perturbation = {}, seed = {}, "
                                       "N = {}, leafMaxSize = {}, num_tags = {}, "
                                       "dim = {}\n\n",
                                       L, dL * L, seed, n,
                                       data.d_leafMaxSize,
                                       data.d_numTags, data.d_dim);


              if (dim == 2)
                msg = test::testNanoflannExcludeInclude<2>(n, L, dL * L, seed,
                                                           data);
              else if (dim == 3)
                msg = test::testNanoflannExcludeInclude<3>(n, L, dL * L, seed,
                                                           data);

              std::cout << msg;

            } // loop dim
          } // loop N
        } // loop seed
      } // loop dL
    } // loop L
  }
  else if (testSelect == 1) {
    // test 2
    std::cout
            << "\n\nTesting Exclude and Include tag type neighbor search for different leaf sizes and number of tags\n\n";

    test_count = 0;
    std::vector<std::vector<std::vector<test::testNSearchData> > > data_set(
            leafMaxSizes.size());

    for (size_t i = 0; i < leafMaxSizes.size(); i++) {

      data_set[i].resize(numTags.size());

      for (size_t j = 0; j < numTags.size(); j++) {

        data_set[i][j].resize(dims.size());

        for (size_t k = 0; k < dims.size(); k++) {

          auto L = L_test[0];
          auto dL = dL_test[0];
          auto seed = seeds[0];
          auto n = N_test[0];

          std::string msg;
          test::testNSearchData data;
          data.d_dim = dims[k];
          data.d_numTags = numTags[j];
          data.d_leafMaxSize = leafMaxSizes[i];

          std::cout << "\n**** Test number = " << test_count++ << " ****\n";
          std::cout << std::format("Test parameters: L = {}, lattice "
                                   "perturbation = {}, seed = {}, "
                                   "N = {}, leafMaxSize = {}, num_tags = {}, "
                                   "dim = {}\n\n",
                                   L, dL * L, seed, n,
                                   data.d_leafMaxSize,
                                   data.d_numTags, data.d_dim);


          if (data.d_dim == 2)
            msg = test::testNanoflannExcludeInclude<2>(n, L, dL * L, seed,
                                                       data);
          else if (data.d_dim == 3)
            msg = test::testNanoflannExcludeInclude<3>(n, L, dL * L, seed,
                                                       data);

          std::cout << msg;

          data_set[i][j][k] = data;
        } // loop dim
      }// loop numTag
    } // loop leaf

    std::cout << "\n\nSummarize results of test 2\n\n";
    for (size_t k = 0; k < dims.size(); k++) {
      std::cout << "Dim = " << dims[k] << "\n";
      for (size_t j = 0; j < numTags.size(); j++) {
        std::cout << std::format("    numTag = {:2d}\n", numTags[j]);
        for (size_t i = 0; i < leafMaxSizes.size(); i++) {
          const auto &data = data_set[i][j][k];
          std::cout << std::format(
                  "        leafMaxSize = {:2d}, numPoints = {:8d}, bld_time = {:8d}\n"
                  "          (brute-search) def_time = {:8d}, exc_time = {:8d}, inc_time = {:8d}\n"
                  "          (nflan-search) def_time = {:8d}, exc_time = {:8d}, inc_time = {:8d}\n",
                  data.d_leafMaxSize,
                  data.d_numPoints,
                  int(data.d_treeBuildTime),
                  int(data.d_defaultBruteSearchTime),
                  int(data.d_excludeBruteSearchTime),
                  int(data.d_includeBruteSearchTime),
                  int(data.d_defaultNFlannSearchTime),
                  int(data.d_excludeNFlannSearchTime),
                  int(data.d_includeNFlannSearchTime));
        }
      }
    }
  }
  else if (testSelect == 2) {
    // test 2
    std::cout << "\n\nTesting closest point search for different lattice sizes\n\n";

    //    L_test = {1.};
    //    dL_test = {0.2};
    for (auto L: L_test) {
      for (auto dL: dL_test) {
        for (auto seed: seeds) {
          for (auto n: N_test) {

              std::string msg;
              test::testNSearchData data;
              data.d_dim = 3;
              data.d_numTags = numTags[0];
              data.d_leafMaxSize = leafMaxSizes[0];

              std::cout << "\n**** Test number = " << test_count++ << " ****\n";
              std::cout << std::format("Test parameters: L = {}, lattice "
                                       "perturbation = {}, seed = {}, "
                                       "N = {}, leafMaxSize = {}, num_tags = {}"
                                       "\n\n",
                                       L, dL * L, seed, n,
                                       data.d_leafMaxSize,
                                       data.d_numTags);


              msg = test::testNanoflannClosestPoint(n, L, dL * L, seed);

              std::cout << msg;

          } // loop N
        } // loop seed
      } // loop dL
    } // loop L
  }

  return EXIT_SUCCESS;
}
