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

struct testNSearchData {
    int d_dim = 3;
    int d_leafMaxSize = 10;
    int d_numTags = 4;
    int d_numPoints = 0;
    double d_treeBuildTime = 0.;
    double d_defaultNFlannSearchTime = 0.;
    double d_excludeNFlannSearchTime = 0.;
    double d_includeNFlannSearchTime = 0.;
    double d_defaultBruteSearchTime = 0.;
    double d_excludeBruteSearchTime = 0.;
    double d_includeBruteSearchTime = 0.;
};

/*!
 * @brief Perform test on nsearch
 * @param N size of particle cloud in each dimension. total size would be N^3
 * @param L Size of unit cell to create crystal lattice point cloud
 * @param dL Perturbation of lattice sites
 * @param seed Seed
 * @param dim Dimension
 * @return str String containing various information
 */
template <int dim = 3>
std::string testNanoflann(size_t N, double L, double dL, int seed);

/*!
 * @brief Perform test on nsearch
 * @param N size of particle cloud in each dimension. total size would be N^3
 * @param L Size of unit cell to create crystal lattice point cloud
 * @param dL Perturbation of lattice sites
 * @param seed Seed
 * @param dim Dimension
 * @param data Search data
 * @return str String containing various information
 */
template <int dim = 3>
std::string testNanoflannExcludeInclude(size_t N, double L,
                                        double dL, int seed,
                                        testNSearchData &data);

/*!
 * @brief Perform test on nsearch
 * @param N size of particle cloud in each dimension. total size would be N^3
 * @param L Size of unit cell to create crystal lattice point cloud
 * @param dL Perturbation of lattice sites
 * @param seed Seed
 * @return str String containing various information
 */
std::string testNanoflannClosestPoint(size_t N, double L, double dL, int seed);

} // namespace test

#endif // TEST_NSEARCH_LIB_H
