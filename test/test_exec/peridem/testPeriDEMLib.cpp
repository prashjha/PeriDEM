/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "testPeriDEMLib.h"

// PeriDEM includes
#include "inp/input.h"                          // Input class
#include "model/dem/demModel.h"                 // Model class
#include "inp/decks/outputDeck.h"
#include "util/methods.h"
#include "util/io.h"
#include <fmt/format.h>
#include <fstream>
#include <string>

std::string test::testPeriDEM(std::string filepath) {

  // current time
  auto begin = steady_clock::now();

  // read input data
  auto *deck = new inp::Input(filepath + "/input.yaml");
  deck->getOutputDeck()->d_path = filepath + "/out/";
  std::cout << "filepath = " << deck->getOutputDeck()->d_path << "\n";

  // check which model to run
  if (deck->isPeriDEM()) {
    model::DEMModel dem(deck);
    dem.run(deck);
  } else {
    std::cout << "PeriDEM model not found in input file.\n";
    return "PeriDEM not found in input file";
  }

  // get time elapsed
  auto end = steady_clock::now();
  double elapsed_secs = util::methods::timeDiff(begin, end, "seconds");

  std::cout << "Total simulation time = " << elapsed_secs
            << " (seconds)" << std::endl;

  return "pass";
}