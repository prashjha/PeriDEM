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
#include "util/io.h"                            // InputParser class
#include "util/methods.h"

int main(int argc, char *argv[]) {

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h") or !input.cmdOptionExists("-i")) {
    // print help
    std::cout << argv[0] << " (Version " << MAJOR_VERSION << "."
              << MINOR_VERSION << "." << UPDATE_VERSION
              << ") -i <data-filepath>" << std::endl;
    exit(EXIT_FAILURE);
  }

  // read input file
  std::string filepath = input.getCmdOption("-i");

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

    // test quad data for triangle element
    for (size_t i = 1; i < 4; i++)
      test::testTetElem(i, filepath);

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
