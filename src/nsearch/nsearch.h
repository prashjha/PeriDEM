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
#include <cstdint> // uint8_t type
#include <string> // size_t type
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
  explicit NFlannSearchKd(const PointCloud &x, size_t debug = 0, double tree_resolution = 1.);

  double setInputCloud() override;

  double updatePointCloud(const std::vector<util::Point> &x,
                          bool parallel = true) override;

  size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) override;

  size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<int> &neighs,
      std::vector<float> &sqr_dist) override;

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
