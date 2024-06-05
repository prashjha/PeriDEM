/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef TEST_NSEARCH_LIB_H
#define TEST_NSEARCH_LIB_H

#include <string>
#include <vector>

namespace test {

/*!
 * @brief Perform test on nsearch
 * @param N size of particle cloud in each dimension. total size would be N^3
 * @param L Size of unit cell to create crystal lattice point cloud
 * @param dL Perturbation of lattice sites
 * @param seed Seed
 * @return str String containing various information
 */
std::string testNanoflann(size_t N, double L, double dL, int seed);

} // namespace test

#endif // TEST_NSEARCH_LIB_H
