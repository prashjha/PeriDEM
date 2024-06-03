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
        /*! @brief Struct that stores MPI-related information */
        struct MpiStatus {

            /*! @brief Specifies if MPI is enabled (yes if code executed with more than one processor) */
            bool d_mpiEnabled;

            /*! @brief Size (number) of processors */
            int d_mpiSize;

            /*! @brief Rank (id) of this processor */
            int d_mpiRank;

            /*! @brief MPI comm */
            MPI_Comm d_comm;

            /*! @brief Constructor */
            MpiStatus();

            /*!
             * @brief Returns the string containing printable information about the object
             *
             * @param nt Number of tabs to append before printing
             * @param lvl Information level (higher means more information)
             * @return string String containing printable information about the object
             */
            std::string printStr(int nt = 0, int lvl = 0) const;
        };

        /*!
         * @brief Initializes MPI and also creates MpiStatus struct
         *
         * @param argc Command line argument number
         * @param argv Command line argument values
         */
        void initMpi(int argc = 0, char *argv[] = nullptr);

        /*! @brief Initializes MpiStatus struct */
        void initMpiStatus();

        /*!
         * @brief Function to check if MPI is enabled
         *
         * @return bool Bool
         * */
        bool isMpiEnabled();

        /*!
         * @brief Get size (number) of processors
         *
         * @return int Size of MPI processes
         * */
        int mpiSize();

        /*!
         * @brief get rank (id) of this processor
         *
         * @return int Rank of MPI process
         * */
        int mpiRank();

        /*!
         * @brief Get MPI comm
         *
         * @return comm MPI comm
         * */
        MPI_Comm mpiComm();

        /*!
         * @brief Returns pointer to MpiStatus struct
         *
         * @return pointer Pointer to MpiStatus object
         * */
        const MpiStatus *getMpiStatus();

        /*!
         * @brief Initializes MpiStatus struct
         *
         * @param nThreads Number of threads to be used by Taskflow in asynchronous parallelism
         */
        void initNThreads(
                unsigned int nThreads = std::thread::hardware_concurrency());

        /*!
         * @brief Get number of threads to be used by taskflow
         *
         * @return nThreads Number of threads
         */
        unsigned int getNThreads();
    } // namespace parallel
} // namespace util