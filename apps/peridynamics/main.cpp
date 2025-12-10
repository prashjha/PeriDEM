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

#include "model/dem/demModel.h"
#include "material/materialUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/geomObjects.h"
#include "util/matrix.h"
#include "util/methods.h"
#include "util/point.h"
#include "inp/pdecks/contactDeck.h"
#include "rw/reader.h"
#include "util/function.h"
#include "util/geom.h"
#include "util/methods.h"
#include "util/randomDist.h"
#include "util/parallelUtil.h"
#include "inp/decks/materialDeck.h"
#include "inp/decks/modelDeck.h"
#include "inp/decks/outputDeck.h"
#include "inp/decks/restartDeck.h"
#include "rw/vtkParticleWriter.h"
#include "rw/vtkParticleReader.h"
#include "fe/elemIncludes.h"
#include "fe/meshUtil.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
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
class Model : public model::DEMModel {

public:

  /*!
   * @brief Constructor
   *
   * @param deck Input deck
   */
  explicit Model(inp::Input *deck) : model::DEMModel(deck, "peridynamics::Model") {}

  /*!
   * @brief Compute forces
   */
  void computeForces() override {

    bool dbg_condition = d_n % d_infoN == 0;

    log("  Compute forces \n", 2, dbg_condition, 3);

    // reset force
    auto t1 = steady_clock::now();
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
            (std::size_t) 0, d_x.size(), (std::size_t) 1,
            [this](std::size_t i) { this->d_f[i] = util::Point(); }
    ); // for_each

    executor.run(taskflow).get();
    auto force_reset_time = util::methods::timeDiff(t1, steady_clock::now());

    // compute peridynamic forces
    t1 = steady_clock::now();
    computePeridynamicForces();
    auto pd_time = util::methods::timeDiff(t1, steady_clock::now());
    appendKeyData("pd_compute_time", pd_time);
    appendKeyData("avg_peridynamics_force_time", pd_time/d_infoN);

    // Compute external forces
    t1 = steady_clock::now();
    computeExternalForces();
    auto extf_time = util::methods::timeDiff(t1, steady_clock::now());
    appendKeyData("extf_compute_time", extf_time);
    appendKeyData("avg_extf_compute_time", extf_time/d_infoN);

    // output avg time info
    if (dbg_condition) {
      log(fmt::format("    Avg time (ms): \n"
                      "      {:48s} = {:8d}\n"
                      "      {:48s} = {:8d}\n",
                      "peridynamics force", size_t(getKeyData("avg_peridynamics_force_time")),
                      "external force", size_t(getKeyData("avg_extf_compute_time")/d_infoN)),
          2, dbg_condition, 3);

      appendKeyData("avg_peridynamics_force_time", 0.);
      appendKeyData("avg_extf_compute_time", 0.);
    }

    log(fmt::format("    {:50s} = {:8d} \n",
                    "Force reset time (ms)",
                    size_t(force_reset_time)
        ),
        2, dbg_condition, 3);

    log(fmt::format("    {:50s} = {:8d} \n",
                    "External force time (ms)",
                    size_t(extf_time)
        ),
        2, dbg_condition, 3);

    log(fmt::format("    {:50s} = {:8d} \n",
                    "Peridynamics force time (ms)",
                    size_t(pd_time)
        ),
        2, dbg_condition, 3);
  }

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
    util::io::print(fmt::format("Running Peridynamics with number of threads = {}\n", nThreads));
  }
  // set number of threads
  util::parallel::initNThreads(nThreads);
  util::io::print(fmt::format("Number of threads = {}\n", util::parallel::getNThreads()));

  std::string filename;
  if (input.cmdOptionExists("-i"))
    filename = input.getCmdOption("-i");
  else {
    filename = "./example/input_1.yaml";
    util::io::print(fmt::format("Running Peridynamics with example input file = {}\n", filename));
  }

  // current time
  auto begin = steady_clock::now();

  // create deck
  auto *deck = new inp::Input(filename);

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