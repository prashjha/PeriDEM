/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "mpiUtil.h"
#include <iostream>
#include <sstream>

namespace {

    util::mpi::MpiStatus *mpistatus_p = nullptr;
    int mpiInitialized = 0;
}

void util::mpi::initMpi(int argc, char *argv[]) {

  MPI_Initialized(&mpiInitialized);
  if (!mpiInitialized)
    MPI_Init(&argc, &argv);
  initMpiStatus();
}

util::mpi::MpiStatus::MpiStatus()  {
  d_comm = MPI_COMM_WORLD;
  MPI_Comm_size(d_comm, &d_mpiSize);
  MPI_Comm_rank(d_comm, &d_mpiRank);
  d_mpiEnabled = d_mpiSize > 1;
}

std::string util::mpi::MpiStatus::printStr(int nt, int lvl) const {
  std::string tabS = "";
  for (int i = 0; i < nt; i++)
    tabS += "\t";
  std::ostringstream oss;
  oss << tabS << "------- MpiStatus --------" << std::endl << std::endl;
  //oss << tabS << "Comm = " << d_comm << std::endl;
  oss << tabS << "MPI Size = " << d_mpiSize << std::endl;
  oss << tabS << "MPI Rank = " << d_mpiRank << std::endl;
  oss << tabS << "MPI Enabled = " << d_mpiEnabled << std::endl;
  oss << tabS << std::endl;

  return oss.str();
}

void util::mpi::initMpiStatus() {
  if (mpistatus_p != nullptr)
    return;

  mpistatus_p = new util::mpi::MpiStatus();
}

bool util::mpi::isMpiEnabled() {

  // for now, we do not call assert and rather create MpiStatus if it is not instantiated
  /*
  //assert((mpistatus_p != nullptr) && "mpistatus_p "
  //                                   "(pointer of struct type util::mpi::MpiStatus) is not initialized. "
  //                                   "Call util::mpi::initMpiStatus() possibly right after MPI_Init().");
  */
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::mpi::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    mpistatus_p = new util::mpi::MpiStatus();

  return mpistatus_p->d_mpiEnabled;
}

int util::mpi::mpiSize() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::mpi::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    mpistatus_p = new util::mpi::MpiStatus();

  return mpistatus_p->d_mpiSize;
}

int util::mpi::mpiRank() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::mpi::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    mpistatus_p = new util::mpi::MpiStatus();

  return mpistatus_p->d_mpiRank;
}

MPI_Comm util::mpi::mpiComm() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::mpi::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    mpistatus_p = new util::mpi::MpiStatus();

  return mpistatus_p->d_comm;
}

const util::mpi::MpiStatus *util::mpi::getMpiStatus() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::mpi::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    mpistatus_p = new util::mpi::MpiStatus();

  return mpistatus_p;
}


