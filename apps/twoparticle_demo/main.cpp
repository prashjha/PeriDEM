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

#include "appPostprocess.h"
#include "inp/input.h"
#include "periDEMModel.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "util/vecMethods.h"

#include <chrono>
#include <format>
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {

  std::cout << "TwoParticle_Demo (PeriDEM)"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl << std::flush;

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h")) {
    std::cout << "Syntax to run the app: ./TwoParticle_Demo -i <input file> -nThreads <number of threads>";
    std::cout << "Example: ./TwoParticle_Demo -i input.json -nThreads 2";
  }

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = 2;
    std::cout << std::format("Running TwoParticle_Demo with number of threads = {}\n", nThreads);
  }
  util::parallel::initNThreads(nThreads);
  std::cout << std::format("Number of threads = {}\n", util::parallel::getNThreads());

  std::string filename;
  if (input.cmdOptionExists("-i"))
    filename = input.getCmdOption("-i");
  else {
    filename = "./example/input_0.yaml";
    std::cout << std::format("Running TwoParticle_Demo with example input file = {}\n", filename);
  }

  auto begin = std::chrono::steady_clock::now();

  std::shared_ptr<inp::Input> deck = std::make_shared<inp::Input>(filename);

  if (deck->isPeriDEM()) {
    deck->getModelDeck()->d_populateElementNodeConnectivity = true;

    PeriDEMModel dem(deck);
    dem.setPostprocess(std::make_unique<twoparticle_demo::AppPostprocess>());
    dem.run(deck);
  }

  auto end = std::chrono::steady_clock::now();

  std::cout << "Total simulation time (s) = "
            << util::methods::timeDiff(begin, end, "seconds")
            << std::endl;
}
