/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testParticleLib.h"
#include "util/io.h"
#include "util/mpiUtil.h"                       // MPI-related functions
#include <fmt/format.h>

int main() {

  // init mpi
  util::mpi::initMpi();
  int mpiSize = util::mpi::mpiSize(), mpiRank = util::mpi::mpiRank();
  util::io::print(fmt::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::mpi::getMpiStatus()->printStr());

  test::testTransform();

  return EXIT_SUCCESS;
}
