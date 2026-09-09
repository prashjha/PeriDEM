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

#include "inp/input.h"
#include "periDEMModel.h"
#include "util/io.h"
#include "util/json.h"
#include "util/parallelUtil.h"
#include "util/vecMethods.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {

  util::parallel::initMpi(argc, argv);

  std::cout << "Peridynamics (PeriDEM)"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl
            << std::flush;

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h")) {
    std::cout << "Syntax: ./Peridynamics -i <input.json> [-nThreads N] "
                 "[-numSteps N] [-finalTime T] [-outputDir DIR]\n";
  }

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads"))
    nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = 2;
    std::cout << std::format(
        "Running Peridynamics with number of threads = {}\n", nThreads);
  }
  util::parallel::initNThreads(nThreads);
  std::cout << std::format("Number of threads = {}\n",
                           util::parallel::getNThreads());

  std::string filename;
  if (input.cmdOptionExists("-i"))
    filename = input.getCmdOption("-i");
  else {
    filename = "./example/input_0.json";
    std::cout << std::format(
        "Running Peridynamics with example input file = {}\n", filename);
  }

  if (!std::filesystem::exists(filename)) {
    std::cerr << std::format("Input file {} does not exist.\n", filename);
    util::parallel::finalizeMpi();
    return 1;
  }

  auto begin = std::chrono::steady_clock::now();

  std::shared_ptr<inp::Input> deck;
  try {
    std::ifstream f(filename);
    auto j = json::parse(f);
    deck = std::make_shared<inp::Input>(j);
  } catch (const std::exception &e) {
    std::cerr << "Failed to load input: " << e.what() << "\n";
    util::parallel::finalizeMpi();
    return 1;
  }

  if (input.cmdOptionExists("-numSteps"))
    deck->getModelDeck()->d_Nt =
        static_cast<size_t>(std::stoul(input.getCmdOption("-numSteps")));
  if (input.cmdOptionExists("-finalTime"))
    deck->getModelDeck()->d_tFinal =
        std::stod(input.getCmdOption("-finalTime"));
  if (input.cmdOptionExists("-numSteps") || input.cmdOptionExists("-finalTime"))
    deck->getModelDeck()->d_dt =
        deck->getModelDeck()->d_tFinal /
        static_cast<double>(deck->getModelDeck()->d_Nt);
  if (input.cmdOptionExists("-outputDir")) {
    deck->getOutputDeck()->d_path = input.getCmdOption("-outputDir");
    if (deck->getOutputDeck()->d_path.back() != '/')
      deck->getOutputDeck()->d_path += "/";
    std::filesystem::create_directories(deck->getOutputDeck()->d_path);
  }

  if (deck->isPeriDEM()) {
    deck->getModelDeck()->d_populateElementNodeConnectivity = true;
    PeriDEMModel dem(deck);
    dem.run(deck);

    if (util::parallel::mpiRank() == 0) {
      double max_u = 0.;
      for (const auto &u : dem.d_u)
        max_u = std::max(max_u, u.length());
      const std::filesystem::path out =
          std::filesystem::path(deck->getOutputDeck()->d_path) /
          "pd_metric.txt";
      std::ofstream os(out);
      os << std::format("{:.12e}\n", max_u);
      std::cout << std::format("pd_metric: max|u|={:.12e}\n", max_u);
    }
  } else {
    std::cout << "PeriDEM model not found in input file.\n";
  }

  auto end = std::chrono::steady_clock::now();

  if (util::parallel::mpiRank() == 0) {
    std::cout << "Total simulation time (s) = "
              << util::methods::timeDiff(begin, end, "seconds") << std::endl;
  }

  util::parallel::finalizeMpi();
  return 0;
}
