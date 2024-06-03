/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "nsearch.h"

nsearch::NFlannSearchKd::NFlannSearchKd(const PointCloud &x, size_t debug,
                                        double tree_resolution)
        : BaseNSearch("nflann_kdtree", debug), d_cloud(x), d_tree(3, d_cloud,
                                                                  nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */)) {
  d_params.sorted = false;
}

double nsearch::NFlannSearchKd::setInputCloud() {
  auto t1 = steady_clock::now();
  d_tree.buildIndex();
  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2);
}

double
nsearch::NFlannSearchKd::updatePointCloud(const std::vector<util::Point> &x,
                                          bool parallel) {
  return 0;
}

size_t nsearch::NFlannSearchKd::radiusSearch(const util::Point &searchPoint,
                                             const double &search_r,
                                             std::vector<size_t> &neighs,
                                             std::vector<double> &sqr_dist) {

  double query_pt[3] = {searchPoint[0], searchPoint[1], searchPoint[2]};

  TreeSearchRes resultSet(search_r * search_r, neighs, sqr_dist);
  return d_tree.radiusSearchCustomCallback(&query_pt[0], resultSet, d_params);
}

size_t nsearch::NFlannSearchKd::radiusSearch(const util::Point &searchPoint,
                                             const double &search_r,
                                             std::vector<int> &neighs,
                                             std::vector<float> &sqr_dist) {

  // ugly but quick fix
  // first, get results using int and float and then convert
  std::vector<size_t> neighs_temp;
  std::vector<double> sqr_dist_temp;
  auto N =
          this->radiusSearch(searchPoint, search_r, neighs_temp, sqr_dist_temp);

  if (N > 0) {
    neighs.resize(N);
    sqr_dist.resize(N);
    for (size_t i=0; i<N; i++) {
      neighs.push_back(int(neighs_temp[i]));
      sqr_dist.push_back(float(sqr_dist_temp[i]));
    }
  }

  return N;
}
