/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef TEST_NSEARCH_LIB_H
#define TEST_NSEARCH_LIB_H

#include <hpx/config.hpp>
#include <string>
#include <vector>
#include "fmt/format.h"

namespace test {

/*!
 * @brief Perform test on nsearch
 * @param N size of particle cloud in each dimension. total size would be N^3
 */
void testNSearch(size_t N);

std::string testNanoflannAndPCL(size_t N, double L, double dL, int seed);

} // namespace test

#endif // TEST_NSEARCH_LIB_H
