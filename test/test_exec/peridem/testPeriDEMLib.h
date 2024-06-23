/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef TESTFELIB_H
#define TESTFELIB_H

#include <string>
#include <vector>

namespace test {

/*!
 * @brief Tests PeriDEM model class
 *
 * @param filepath Path where input file can be found (expects file 'input.yaml' in the filepath)
 * @return string 'pass' if test is successful
 */
std::string testPeriDEM(std::string filepath);

} // namespace test

#endif // TESTFELIB_H
