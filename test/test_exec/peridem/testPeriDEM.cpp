/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <PeriDEMConfig.h>
#include <hpx/hpx_main.hpp>           // Need main source file
#include "testPeriDEMLib.h"
#include <iostream>
#include <boost/program_options.hpp>            // program options

int main(int argc, char *argv[]) {

  boost::program_options::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "data-filepath,i", boost::program_options::value<std::string>(),
      "Data filepath");

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  // read input file
  std::string filepath;
  if (vm.count("data-filepath")) filepath = vm["data-filepath"].as<std::string>();

  if (filepath.empty()) {
    std::cerr << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <data-filepath> --hpx:threads=n" << std::endl;
    exit(1);
  }

  // run test
  auto msg = test::testPeriDEM(filepath);

  if (msg == "pass")
    std::cout << "testPeriDEM: Pass\n";
  else {
    std::cerr << "Error: " << msg << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
