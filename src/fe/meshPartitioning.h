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

namespace fe {

// forward declare Mesh
class Mesh;

/*! @brief Partitions the nodes based on node neighborlist supplied.
 * Function first creates a graph with nodes as vertices and edges given by node neighbors.
 * Then the metis function is called to partition the graph into specified number of parts.
 *
 * @param partitionMethod Method to partition ("metis_recursive" or "metis_kway")
 * @param nodeNeighs Neighborlist of nodes
 * @param nodePartition Vector that stores partition number of nodes
 * @param nPartitions Number of partitions
 */
void metisGraphPartition(std::string partitionMethod,
                         const std::vector<std::vector<size_t>> &nodeNeighs,
                         std::vector<size_t> &nodePartition,
                         size_t nPartitions);

/*! @brief Partitions the nodes based on node neighborlist supplied.
 * Function first creates a graph with nodes as vertices and edges given by node neighbors.
 * Then the metis function is called to partition the graph into specified number of parts.
 *
 * @param partitionMethod Method to partition ("recursive" or "kway")
 * @param mesh_p Pointer to mesh
 * @param nodeNeighs Neighborlist of nodes
 * @param nPartitions Number of partitions
 */
void metisGraphPartition(std::string partitionMethod,
                         fe::Mesh *mesh_p,
                         const std::vector<std::vector<size_t>> &nodeNeighs,
                         size_t nPartitions);

} // namespace fe