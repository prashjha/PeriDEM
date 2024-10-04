/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
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
 * @param mHorizon Integer factor that is used to compute nonlocal radius, i.e., horizon (epsilon = m * h, h being mesh size)
 * @param testOption Test otion flag. 0 - use in-built uniform mesh, 1 - use user-specified mesh
 * @param meshFilename Mesh filename with relative path from the directory where test command is run
 */
void testMPI(size_t nGrid = 10, size_t mHorizon = 3, size_t testOption = 0, std::string meshFilename = "");

} // namespace test