/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "geometryUtil.h"
#include "nsearch/nsearch.h"
#include <fmt/format.h>

typedef nsearch::NFlannSearchKd NSearch;

void geometry::computeNonlocalNeighborhood(const std::vector<util::Point> &nodes,
                                      double horizon,
                                      std::vector<std::vector<size_t>> &nodeNeighs) {
  nodeNeighs.resize(nodes.size());

  auto nsearch_p = std::make_unique<NSearch>(nodes);
  double set_tree_time = nsearch_p->updatePointCloud(nodes, true);
  set_tree_time += nsearch_p->setInputCloud();
  std::cout << fmt::format("Tree setup time (ms) = {}. \n", set_tree_time);

  for (size_t i=0; i<nodes.size(); i++) {
    std::vector<size_t> neighs;
    std::vector<double> sqr_dist;
    nodeNeighs[i].resize(0);

    if (nsearch_p->radiusSearch(nodes[i], horizon, neighs, sqr_dist) > 0) {
      for (std::size_t j = 0; j < neighs.size(); ++j)
        if (neighs[j] != i) {
          nodeNeighs[i].push_back(neighs[j]);
        }
    }
  }
}
