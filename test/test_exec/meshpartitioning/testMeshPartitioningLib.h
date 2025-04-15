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

/*! @brief Namespace to group the methods used in testing of the library */
namespace test {

/*!
 * @brief Tests metis partitioning of graph
 */
void testGraphPartitioningSimple();

/*!
 * @brief Tests metis partitioning of graph from a 2-D mesh with nonlocal interaction
 * @param nPart Number of partitions
 * @param nGrid Number of element along a line (total number of elements is N*N)
 * @param mHorizon Integer factor that is used to compute nonlocal radius, i.e., horizon (epsilon = m * h, h being mesh size)
 * @param testOption Test otion flag. 0 - use in-built uniform mesh, 1 - use user-specified mesh
 * @param meshFilename Mesh filename with relative path from the directory where test command is run
 */
void testGraphPartitioning(size_t nPart = 4, size_t nGrid = 10, size_t mHorizon = 3, size_t testOption = 0, std::string meshFilename = "");

} // namespace test
