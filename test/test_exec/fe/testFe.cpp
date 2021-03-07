/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testFeLib.h"
#include <PeriDEMConfig.h>
#include <iostream>
#include <boost/program_options.hpp>            // program options

int main(int argc, char *argv[]) {

  boost::program_options::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "data-filepath,i", boost::program_options::value<std::string>(),
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
  std::string filepath;
  if (vm.count("data-filepath")) filepath = vm["data-filepath"].as<std::string>();

  if (filepath.empty()) {
    std::cerr << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <data-filepath>" << std::endl;
    exit(1);
  }

  //
  // test quadrature method for triangle
  //
  {
    // test quad data for line element
    for (size_t i = 1; i < 6; i++)
      test::testLineElem(i, filepath);

    // test quad data for triangle element
    for (size_t i = 1; i < 6; i++)
      test::testTriElem(i, filepath);

    // test quad data for quadrangle element
    for (size_t i = 1; i < 6; i++)
      test::testQuadElem(i, filepath);

    //    // test quad data for triangle element
    //    for (size_t i = 1; i < 4; i++)
    //      test::testTetElem(i, filepath);

    // test additional time in computing quad points instead of storing it
    if (false) {
      for (size_t i = 1; i < 6; i++) {
        test::testTriElemTime(i, 1000);

        test::testTriElemTime(i, 10000);

        test::testTriElemTime(i, 100000);

        test::testTriElemTime(i, 1000000);
      }
    }
  }

  return EXIT_SUCCESS;
}
