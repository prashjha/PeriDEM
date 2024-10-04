/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
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
   * @copydoc radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<int> &neighs,
      std::vector<float> &sqr_dist) = 0
   */
  virtual size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) = 0;

    /*!
     * @brief Perform radius search to find points in a point cloud within specified distance from a given point.
     * This function also checks the tag of potential point and only if the point has a different tag it will be added to the list.
     * This is useful for getting neighbor lists for contact. In this case, tag of a point is the particle id (particle it belongs to).
     *
     * @param searchPoint Point near which we want neighbors
     * @param search_r Search radius
     * @param neighs Indices of points in neighborhood
     * @param sqr_dist Squared distance of neighboring points from search point
     * @param searchPointTag Tag of a search point
     * @param dataTags Vector of tags for each point in the pointcloud
     * @return number Number of points in neighborhood
     */
    virtual size_t radiusSearchExcludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) = 0;

    /*!
     * @copydoc radiusSearchExcludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            std::vector<size_t> &dataTags) = 0
     */
    virtual size_t radiusSearchExcludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<int> &neighs,
            std::vector<float> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) = 0;

    /*!
     * @brief Perform radius search to find points in a point cloud within specified distance from a given point.
     * This function also checks the tag of potential point and only if the point has a same tag it will be added to the list.
     * This is useful for getting neighbor lists for peridynamics. In this case, tag of a point is the particle id (particle it belongs to).
     *
     * @param searchPoint Point near which we want neighbors
     * @param search_r Search radius
     * @param neighs Indices of points in neighborhood
     * @param sqr_dist Squared distance of neighboring points from search point
     * @param searchPointTag Tag of a search point
     * @param dataTags Vector of tags for each point in the pointcloud
     * @return number Number of points in neighborhood
     */
    virtual size_t radiusSearchIncludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) = 0;

    /*!
     * @copydoc radiusSearchIncludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            std::vector<size_t> &dataTags) = 0
     */
    virtual size_t radiusSearchIncludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<int> &neighs,
            std::vector<float> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) = 0;

public:
  /*! @brief control the verbosity */
  size_t d_debug;

  /*! @brief name of tree: nflann_kdtree */
  std::string d_treeType;
};

/*!
 * @brief A class for nearest neighbor search using nanoflann library
 */
template <int dim = 3>
class NFlannSearchKd : public BaseNSearch {

public:
  /*!
   * @brief Constructor
   *
   * @param x Point cloud
   * @param debug Debug level to print information
   * @param max_leaf Maximum number of leafs
   */
  explicit NFlannSearchKd(const PointCloud &x, size_t debug = 0, size_t max_leafs = 10)
          : BaseNSearch("nflann_kdtree", debug),
            d_cloud(x),
            d_tree(dim, d_cloud,
                   nanoflann::KDTreeSingleIndexAdaptorParams(max_leafs /* max leaf */)) {
    d_params.sorted = false;
  };

  /*!
   * @brief Set input cloud
   * @return double Time taken to update the point cloud
   */
  double setInputCloud() override  {
    auto t1 = steady_clock::now();
    d_tree.buildIndex();
    auto t2 = steady_clock::now();
    return util::methods::timeDiff(t1, t2);
  };

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
                          bool parallel = true) override {
    return 0;
  };

  /*!
   * @copydoc BaseNSearch::radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) = 0
   */
  size_t radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<size_t> &neighs,
      std::vector<double> &sqr_dist) override {

    double query_pt[3] = {searchPoint[0], searchPoint[1], searchPoint[2]};

    TreeSearchRes resultSet(search_r * search_r, neighs, sqr_dist);
    return d_tree.radiusSearchCustomCallback(&query_pt[0], resultSet, d_params);
  };

  /*!
   * @copydoc BaseNSearch::radiusSearch(
      const util::Point &searchPoint, const double &search_r,
      std::vector<int> &neighs,
      std::vector<float> &sqr_dist) = 0
   */
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
  };


    /*!
     * @copydoc BaseNSearch::radiusSearchExcludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            std::vector<size_t> &dataTags) = 0
     */
    size_t radiusSearchExcludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) override {

      double query_pt[3] = {searchPoint[0], searchPoint[1], searchPoint[2]};

      TreeSearchCheckIDExcludeRes resultSet(search_r * search_r,
                            neighs, sqr_dist, searchPointTag, dataTags);

      return d_tree.radiusSearchCustomCallback(&query_pt[0], resultSet, d_params);
    };

    /*!
     * @copydoc BaseNSearch::radiusSearchExcludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            std::vector<size_t> &dataTags) = 0
     */
    size_t radiusSearchExcludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<int> &neighs,
            std::vector<float> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) override {

      // first, get results using int and float and then convert
      std::vector<size_t> neighs_temp;
      std::vector<double> sqr_dist_temp;
      auto N =
              this->radiusSearchExcludeTag(searchPoint, search_r,
                                           neighs_temp, sqr_dist_temp,
                                           searchPointTag, dataTags);

      if (N > 0) {
        neighs.resize(N);
        sqr_dist.resize(N);
        for (size_t i=0; i<N; i++) {
          neighs.push_back(int(neighs_temp[i]));
          sqr_dist.push_back(float(sqr_dist_temp[i]));
        }
      }

      return N;
    };

    /*!
     * @copydoc BaseNSearch::radiusSearchIncludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            std::vector<size_t> &dataTags) = 0
     */
    size_t radiusSearchIncludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) override {

      double query_pt[3] = {searchPoint[0], searchPoint[1], searchPoint[2]};

      TreeSearchCheckIDIncludeRes resultSet(search_r * search_r,
                                            neighs, sqr_dist,
                                            searchPointTag, dataTags);

      return d_tree.radiusSearchCustomCallback(&query_pt[0], resultSet, d_params);
    };

    /*!
     * @copydoc BaseNSearch::radiusSearchIncludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<size_t> &neighs,
            std::vector<double> &sqr_dist,
            const size_t &searchPointTag,
            std::vector<size_t> &dataTags) = 0
     */
    size_t radiusSearchIncludeTag(
            const util::Point &searchPoint,
            const double &search_r,
            std::vector<int> &neighs,
            std::vector<float> &sqr_dist,
            const size_t &searchPointTag,
            const std::vector<size_t> &dataTags) override {

      // first, get results using int and float and then convert
      std::vector<size_t> neighs_temp;
      std::vector<double> sqr_dist_temp;
      auto N =
              this->radiusSearchIncludeTag(searchPoint, search_r,
                                           neighs_temp, sqr_dist_temp,
                                           searchPointTag, dataTags);

      if (N > 0) {
        neighs.resize(N);
        sqr_dist.resize(N);
        for (size_t i=0; i<N; i++) {
          neighs.push_back(int(neighs_temp[i]));
          sqr_dist.push_back(float(sqr_dist_temp[i]));
        }
      }

      return N;
    };

public:
  /*! @brief coordinates of the points */
  PointCloudAdaptor d_cloud;

  /*! @brief Tree */
  nanoflann::KDTreeSingleIndexAdaptor<
          nanoflann::L2_Simple_Adaptor<double, PointCloudAdaptor>, PointCloudAdaptor,
          dim
  > d_tree;

  /*! @brief Tree search parameters */
  nanoflann::SearchParameters d_params;
};

} // namespace nsearch

#endif // NSEARCH_NSEARCH_H
