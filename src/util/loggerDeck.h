/*
* -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTILS_IO_LOGGERDECK_H
#define UTILS_IO_LOGGERDECK_H


#include <fstream>

namespace util {
    namespace io {
        /*!
         * @brief Deck to store log parameters
         */
        struct LoggerDeck {
            /*! @brief Debug level */
            int d_debugLevel;

            /*! @brief Filename to print log */
            std::string d_filename;

            /*! @brief Print to std::cout? */
            bool d_printScreen;

            /*! @brief Print to file? */
            bool d_printFile;

            /*!
             * @brief Constructor
             */
            LoggerDeck() : d_debugLevel(5), d_printScreen(true), d_printFile(false) {
            }

            /*!
             * @brief Constructor
             *
             * @param debug_level Specify debug level/verbosity
             * @param filename Specify log filename
             */
            LoggerDeck(int debug_level, std::string filename)
                : d_debugLevel(debug_level), d_filename(filename), d_printScreen
                  (d_debugLevel > 0), d_printFile(!d_filename.empty()) {
            }
        };
    } // namespace io
} // namespace util

#endif // UTILS_IO_LOGGERDECK_H

