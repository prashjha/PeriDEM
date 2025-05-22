/*
* -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTILS_IO_INPUTPARSER_H
#define UTILS_IO_INPUTPARSER_H

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace util {
    /*! @brief Provides utility function for input and output */
    namespace io {
        /*!
         * @brief Input command line argument parser
         *
         * source - https://stackoverflow.com/a/868894
         * @author iain
         */
        class InputParser {
        public:
            /*!
             * @brief Constructor
             * @param argc Number of arguments
             * @param argv Strings
             */
            InputParser(int &argc, char **argv) {
                for (int i = 1; i < argc; ++i)
                    this->tokens.push_back(std::string(argv[i]));
            }

            /*!
             * @brief Get value of argument specified by key
             * @param option Argument name/key
             * @return string Value of argument
             */
            const std::string &getCmdOption(const std::string &option) const {
                std::vector<std::string>::const_iterator itr;
                itr = std::find(this->tokens.begin(), this->tokens.end(), option);
                if (itr != this->tokens.end() && ++itr != this->tokens.end())
                    return *itr;

                static const std::string empty_string("");
                return empty_string;
            }

            /*!
             * @brief Check if argument exists
             * @param option Argument name/key
             * @return bool True if argument exists
             */
            bool cmdOptionExists(const std::string &option) const {
                return std::find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
            }

        private:
            /*! @brief Tokens */
            std::vector<std::string> tokens;
        };
    } // namespace io
} // namespace util

#endif // UTILS_IO_INPUTPARSER_H
