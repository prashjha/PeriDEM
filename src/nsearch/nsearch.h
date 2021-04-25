/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef NSEARCH_NSEARCH_H
#define NSEARCH_NSEARCH_H

#include "util/point.h" // definition of Point
#include "util/methods.h"
#include <stdint.h> // uint8_t type
#include <string.h> // size_t type
#include <vector>

#include "nflannSetup.h"

/*! @brief Methods for performing efficient search of neighboring points */
namespace nsearch {

/*!
 * @brief A class for nearest neighbor search
 */
class BaseNSearch {

public:
  /*!
   * @brief Constructor
   */
  //NSearch() : d_cloud_p(new pcl::PointCloud<pcl::PointXYZ>), d_tree
  // (false) {};
  BaseNSearch(std::string name, size_t debug = 0)
      : d_debug(debug), d_treeType(name) {}

  virtual double updatePointCloud(const std::vector<util::Point> &x,
                        bool parallel = true) = 0;

  virtual double setInputCloud() = 0;

  virtual size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<int> &neighs,
      std::vector<float> &sqr_dist) = 0;

  virtual size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) = 0;

public:
  /*@brief control the verbosity */
  size_t d_debug;

  /*@brief name of tree: nflann_kdtree */
  std::string d_treeType;
};

/*!
 * @brief A class for nearest neighbor search using nanoflann library
 */
class NFlannSearchKd : public BaseNSearch {

public:
  /*!
   * @brief Constructor
   */
  NFlannSearchKd(const PointCloud &x, size_t debug = 0, double tree_resolution = 1.)
      : BaseNSearch("nflann_kdtree", debug), d_cloud(x), d_tree(3, d_cloud,
      nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */)) {
    d_params.sorted = false;
  }

  double setInputCloud() override {
    auto t1 = steady_clock::now();
    d_tree.buildIndex();
    auto t2 = steady_clock::now();
    return util::methods::timeDiff(t1, t2);
  }

  double updatePointCloud(const std::vector<util::Point> &x,
                          bool parallel = true) override {
    return 0;
  }

  size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) override {

    double query_pt[3] = {searchPoint[0], searchPoint[1], searchPoint[2]};

    TreeSearchRes resultSet(search_r * search_r, neighs, sqr_dist);
    return d_tree.radiusSearchCustomCallback(&query_pt[0], resultSet, d_params);
  }

  size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<int> &neighs,
      std::vector<float> &sqr_dist) override {

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

public:
  /*! @brief coordinates of the points */
  PointCloudAdaptor d_cloud;

  /*! @brief Tree */
  NFlannKdTree d_tree;

  /*! @brief Tree search parameters */
  nanoflann::SearchParams d_params;
};

} // namespace nsearch

#endif // NSEARCH_NSEARCH_H
