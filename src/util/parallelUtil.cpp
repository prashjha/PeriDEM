/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "parallelUtil.h"
#include <iostream>
#include <sstream>

namespace {

    // MPI-related
    util::parallel::MpiStatus *mpistatus_p = nullptr;
    int mpiInitialized = 0;

    // Thread-related (via Taskflow)
    unsigned int numThreads = 0;
}

void util::parallel::initMpi(int argc, char *argv[]) {

  MPI_Initialized(&mpiInitialized);
  if (!mpiInitialized)
    MPI_Init(&argc, &argv);
  initMpiStatus();
}

util::parallel::MpiStatus::MpiStatus()  {
  d_comm = MPI_COMM_WORLD;
  MPI_Comm_size(d_comm, &d_mpiSize);
  MPI_Comm_rank(d_comm, &d_mpiRank);
  d_mpiEnabled = d_mpiSize > 1;
}

std::string util::parallel::MpiStatus::printStr(int nt, int lvl) const {
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

void util::parallel::initMpiStatus() {
  if (mpistatus_p != nullptr)
    return;

  mpistatus_p = new util::parallel::MpiStatus();
}

bool util::parallel::isMpiEnabled() {

  // for now, we do not call assert and rather create MpiStatus if it is not instantiated
  /*
  //assert((mpistatus_p != nullptr) && "mpistatus_p "
  //                                   "(pointer of struct type util::parallel::MpiStatus) is not initialized. "
  //                                   "Call util::parallel::initMpiStatus() possibly right after MPI_Init().");
  */
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::parallel::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    util::parallel::initMpi();

  return mpistatus_p->d_mpiEnabled;
}

int util::parallel::mpiSize() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::parallel::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    util::parallel::initMpi();

  return mpistatus_p->d_mpiSize;
}

int util::parallel::mpiRank() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::parallel::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    util::parallel::initMpi();

  return mpistatus_p->d_mpiRank;
}

MPI_Comm util::parallel::mpiComm() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::parallel::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    util::parallel::initMpi();

  return mpistatus_p->d_comm;
}

const util::parallel::MpiStatus *util::parallel::getMpiStatus() {
  //assert(mpiInitialized && "mpiInitialized is false indicating MPI_Init() or util::parallel::initMpi() has not been called.\n");
  if (mpistatus_p == nullptr)
    util::parallel::initMpi();

  return mpistatus_p;
}

void util::parallel::initNThreads(unsigned int nThreads) {
  if (numThreads > 0) {
    std::cout << "Number of threads numThreads is already initialized.\n";
  } else
    numThreads = nThreads;
}

unsigned int util::parallel::getNThreads() {
  if (numThreads == 0)
    numThreads = std::thread::hardware_concurrency();
  return numThreads;
}


