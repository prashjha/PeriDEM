/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testUtilLib.h"
#include "util/io.h"
#include "util/mpiUtil.h"                       // MPI-related functions
#include <fmt/format.h>
#include <iostream>

int main(int argc, char *argv[]) {

  // init mpi
  util::mpi::initMpi(argc, argv);
  int mpiSize = util::mpi::mpiSize(), mpiRank = util::mpi::mpiRank();
  util::io::print(fmt::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::mpi::getMpiStatus()->printStr());

  test::testUtilMethods();
  return EXIT_SUCCESS;
}
