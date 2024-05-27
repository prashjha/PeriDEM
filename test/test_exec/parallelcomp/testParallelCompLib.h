/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <string>
#include <vector>

namespace test {

/*!
 * @brief Perform test on taskflow
 * @param N size of vector to profile taskflow
 * @param seed Seed
 * @return str String containing various information
 */
std::string testTaskflow(size_t N, int seed);

/*!
 * @brief Perform parallelization test using MPI on mesh partition based on metis
 * @param nGrid Number of element along a line (total number of elements is N*N)
 * @param m Integer factor that is used to compute nonlocal radius, i.e., horizon (epsilon = m * h, h being mesh size)
 */
void testMPI(size_t nGrid = 10, size_t mHorizon = 3);

} // namespace test