/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <PeriDEMConfig.h>

// HPX includes
#include <hpx/hpx_init.hpp>                     // Need main source file
#include <hpx/util/high_resolution_clock.hpp>

// std includes
#include <boost/program_options.hpp>            // program options
#include <iostream>

// PeriDEM includes
#include "inp/input.h"                          // Input class
#include "model/dem/demModel.h"                 // Model class

int hpx_main(boost::program_options::variables_map& vm)
{
  // display Help
  if (vm.count("help")) {
    std::cout << "To run the simulation: \n"
              << "PeriDEM -i input.yaml --hpx:threads=n" << "\n";
    return hpx::finalize();
  }

  // read input file
  std::string filename;
  if (vm.count("input-file"))
    filename = vm["input-file"].as<std::string>();

  if (filename.empty()) {
    std::cerr << "PeriDEM" << " (Version " << MAJOR_VERSION <<
    		"." << MINOR_VERSION << "." << UPDATE_VERSION <<
			") -i input.yaml --hpx:threads=n" << std::endl;
    return hpx::finalize();
  }

  // print program version
  std::cout << "PeriDEM"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl;

  // current time
  std::uint64_t begin = hpx::util::high_resolution_clock::now();

  // read input data
  auto *deck = new inp::Input(filename);

  // check which model to run
  if (deck->isPeriDEM()) {
    model::DEMModel dem(deck);
    dem.run(deck);
  } else {
    std::cout << "PeriDEM model not found in input file.\n";
  }

  // get time elapsed
  std::uint64_t end = hpx::util::high_resolution_clock::now();
  double elapsed_secs = double(end - begin) / 1.0e9;

  std::cout << "Total simulation time (s) = " << elapsed_secs
            << std::endl;
  return hpx::finalize();
}

int main(int argc, char *argv[]) {

  boost::program_options::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "input-file,i", boost::program_options::value<std::string>(),
      "Configuration file");

  return hpx::init(desc, argc, argv);
}
