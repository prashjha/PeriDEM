/*
* -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTILS_IO_LOGGER_H
#define UTILS_IO_LOGGER_H

#include "loggerDeck.h"
#include "parallelUtil.h" // to make prints MPI aware
#include "constants.h"

namespace util {
    namespace io {
        /*!
         * @brief Prints log to std::cout and also write to the file
         */
        class Logger {
        public:
            /*!
             * @brief Constructor
             *
             * @param deck Logger deck
             */
            Logger(LoggerDeck *deck = nullptr) : d_deck_p(deck) {
                if (d_deck_p == nullptr)
                    d_deck_p = new LoggerDeck();
            }

            /*!
             * @brief Destructor
             */
            ~Logger() {
                if (d_deck_p->d_printFile)
                    d_logFile.close();
            }

            /*!
             * @brief Log the message
             *
             * @param oss Message
             * @param screen_out Specify if it goes to std::cout as well
             * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
             */
            void log(std::ostringstream &oss, bool screen_out = false,
                     int printMpiRank = print_default_mpi_rank) {
                log(oss.str(), screen_out, printMpiRank);

                // reset oss
                oss.str("");
                oss.clear();
            };

            /*!
             * @brief Log the message
             *
             * @param str Message
             * @param screen_out Specify if it goes to std::cout as well
             * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
             */
            void log(const std::string &str, bool screen_out = false,
                     int printMpiRank = print_default_mpi_rank) {
                if (printMpiRank < 0 or util::parallel::mpiRank() == printMpiRank) {
                    if (d_deck_p->d_printScreen || screen_out)
                        std::cout << str << std::flush;

                    // log
                    if (d_deck_p->d_printFile) {
                        d_logFile.open(d_deck_p->d_filename, std::ios_base::app);
                        d_logFile << str;
                        d_logFile.close();
                    }
                }
            };

            /*! @brief Pointer to logger deck */
            LoggerDeck *d_deck_p;

            /*! @brief Filestream for logging */
            std::ofstream d_logFile;
        };
    } // namespace io
} // namespace util

#endif // UTILS_IO_LOGGER_H

