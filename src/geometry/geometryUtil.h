/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "util/point.h"
#include <vector>

namespace geometry {

/*! @brief Partitions the nodes based on node neighborlist supplied.
 * Function first creates a graph with nodes as vertices and edges given by node neighbors.
 * Then the metis function is called to partition the graph into specified number of parts.
 *
 * @param nodes Nodal coordinates
 * @param horizon Nonlocal radius
 * @param nodeNeighs Neighborlist of nodes
 */
void computeNonlocalNeighborhood(const std::vector<util::Point> &nodes,
                                 double horizon,
                                 std::vector<std::vector<size_t>> &nodeNeighs);

} // namespace geometry

