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
   * @param name Name of tree
   * @param debug Debug level to print information
   */
  BaseNSearch(std::string name, size_t debug = 0)
      : d_debug(debug), d_treeType(name) {}

  /*!
   * @brief Function to implement point cloud update
   * @param x Vector of positions of points
   * @param parallel Specify if this is done in parallel
   * @return double Time taken to update the point cloud
   */
  virtual double updatePointCloud(const std::vector<util::Point> &x,
                        bool parallel = true) = 0;

  /*!
   * @brief Set input cloud
   * @return double Time taken to update the point cloud
   */
  virtual double setInputCloud() = 0;

  /*!
   * @brief Perform radius search to find points in a point cloud within specified distance from a given point
   * @param searchPoint Point near which we want neighbors
   * @param search_r Search radius
   * @param neighs Indices of points in neighborhood
   * @param sqr_dist Squared distance of neighboring points from search point
   * @return number Number of points in neighborhood
   */
  virtual size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<int> &neighs,
      std::vector<float> &sqr_dist) = 0;

  /*!
   * @brief Perform radius search to find points in a point cloud within specified distance from a given point
   * @param searchPoint Point near which we want neighbors
   * @param search_r Search radius
   * @param neighs Indices of points in neighborhood
   * @param sqr_dist Squared distance of neighboring points from search point
   * @return number Number of points in neighborhood
   */
  virtual size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) = 0;

public:
  /*! @brief control the verbosity */
  size_t d_debug;

  /*! @brief name of tree: nflann_kdtree */
  std::string d_treeType;
};

/*!
 * @brief A class for nearest neighbor search using nanoflann library
 */
class NFlannSearchKd : public BaseNSearch {

public:
  /*!
   * @brief Constructor
   *
   * @param x Point cloud
   * @param debug Debug level to print information
   * @param tree_resolution Tree resolution
   */
  explicit NFlannSearchKd(const PointCloud &x, size_t debug = 0, double tree_resolution = 1.);

  /*!
   * @brief Set input cloud
   * @return double Time taken to update the point cloud
   */
  double setInputCloud() override;

  /*!
   * @brief Function to implement point cloud update
   *
   * TODO Implement this function
   *
   * @param x Vector of positions of points
   * @param parallel Specify if this is done in parallel
   * @return double Time taken to update the point cloud
   */
  double updatePointCloud(const std::vector<util::Point> &x,
                          bool parallel = true) override;

  /*!
   * @brief Perform radius search to find points in a point cloud within specified distance from a given point
   * @param searchPoint Point near which we want neighbors
   * @param search_r Search radius
   * @param neighs Indices of points in neighborhood
   * @param sqr_dist Squared distance of neighboring points from search point
   * @return number Number of points in neighborhood
   */
  size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) override;

  /*!
   * @brief Perform radius search to find points in a point cloud within specified distance from a given point
   * @param searchPoint Point near which we want neighbors
   * @param search_r Search radius
   * @param neighs Indices of points in neighborhood
   * @param sqr_dist Squared distance of neighboring points from search point
   * @return number Number of points in neighborhood
   */
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
