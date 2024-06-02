/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <mpi.h>
#include <string>
#include <vector>
#include <thread>

namespace util {

    /*! @brief Implements some key functions and classes regularly used in the code when running with MPI */
    namespace parallel {
        struct MpiStatus {
            bool d_mpiEnabled;
            int d_mpiSize;
            int d_mpiRank;
            MPI_Comm d_comm;

            MpiStatus();

            std::string printStr(int nt = 0, int lvl = 0) const;
        };

        void initMpi(int argc = 0, char *argv[] = nullptr);

        void initMpiStatus();

        bool isMpiEnabled();

        int mpiSize();

        int mpiRank();

        MPI_Comm mpiComm();

        const MpiStatus * getMpiStatus();

        void initNThreads(unsigned int nThreads = std::thread::hardware_concurrency());

        unsigned int getNThreads();
    } // namespace parallel
} // namespace util