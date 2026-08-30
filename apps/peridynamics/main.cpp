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

#include "periDEMModel.h"
#include "material/materialUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/matrix.h"
#include "util/vecMethods.h"
#include "util/point.h"
#include "rw/reader.h"
#include "util/function.h"
#include "geom/geomIncludes.h"
#include "util/randomDist.h"
#include "util/parallelUtil.h"
#include "inp/input.h"
#include "rw/vtkParticleWriter.h"
#include "rw/vtkParticleReader.h"
#include "fe/elemIncludes.h"
#include "mesh/meshUtil.h"

#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

/*!
 * @brief Namespace to define peridynamics app for single particle deformation.
 */
namespace peridynamics {

using util::io::log;

/*!
 * @brief Main model class to simulate peridynamics deformation of single particle
 */
class Model : public PeriDEMModel {

public:

  /*!
   * @brief Constructor
   *
   * @param deck Input deck
   */
  explicit Model(std::shared_ptr<inp::Input> & deck)
      : PeriDEMModel(deck, "peridynamics::Model") {}
};
} // namespace peridynamics

int main(int argc, char *argv[]) {

  // print program version
  std::cout << "Peridynamics (PeriDEM)"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl << std::flush;

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h")) {
    // print help
    std::cout << "Syntax to run the app: ./Peridynamics -i <input file> -nThreads <number of threads>";
    std::cout << "Example: ./Peridynamics -i input.yaml -nThreads 2";
  }

  // read input arguments
  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = 2;
    std::cout << std::format("Running Peridynamics with number of threads = {}\n", nThreads);
  }
  // set number of threads
  util::parallel::initNThreads(nThreads);
  std::cout << std::format("Number of threads = {}\n", util::parallel::getNThreads());

  std::string filename;
  if (input.cmdOptionExists("-i"))
    filename = input.getCmdOption("-i");
  else {
    filename = "./example/input_1.yaml";
    std::cout << std::format("Running Peridynamics with example input file = {}\n", filename);
  }

  // current time
  auto begin = steady_clock::now();

  // create deck
  std::shared_ptr<inp::Input> deck = std::make_shared<inp::Input>(filename);

  // check which model to run
  if (deck->isPeriDEM()) {
    // ensure two variables in the deck are set
    deck->getModelDeck()->d_populateElementNodeConnectivity = true;

    // simulate model
    peridynamics::Model dem(deck);
    dem.run(deck);
  }

  // get time elapsed
  auto end = steady_clock::now();

  std::cout << "Total simulation time (s) = " 
            << util::methods::timeDiff(begin, end, "seconds") 
            << std::endl;
}