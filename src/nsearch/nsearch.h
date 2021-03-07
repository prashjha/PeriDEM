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

#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/point_cloud.h>
#include <pcl/octree/octree_search.h>

// hpx lib
#include <hpx/include/parallel_algorithm.hpp>

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
      : d_debug(debug), d_treeType(name),
        d_cloud_p(new pcl::PointCloud<pcl::PointXYZ>) {}

  double updatePointCloud(const std::vector<util::Point> &x,
                        bool parallel = true) {

    auto t1 = steady_clock::now();
    if (parallel) {
      d_cloud_p->points.resize(x.size());

      hpx::parallel::for_loop(
          hpx::parallel::execution::par, 0, x.size(),
          [this, &x](boost::uint64_t i) {
            (*this->d_cloud_p)[i].x = x[i].d_x;
            (*this->d_cloud_p)[i].y = x[i].d_y;
            (*this->d_cloud_p)[i].z = x[i].d_z;
          });
    } else {
      for (size_t i=0; i<x.size(); i++) {
        (*this->d_cloud_p)[i].x = x[i].d_x;
        (*this->d_cloud_p)[i].y = x[i].d_y;
        (*this->d_cloud_p)[i].z = x[i].d_z;
      }
    }
    auto t2 = steady_clock::now();
    return util::methods::timeDiff(t1, t2);
  }

  virtual double setInputCloud() = 0;

public:
  /*@brief control the verbosity */
  size_t d_debug;
  /*@brief name of tree: kdtree or octree */
  std::string d_treeType;
  /*@brief coordinates of the points */
  pcl::PointCloud<pcl::PointXYZ>::Ptr d_cloud_p;
};

/*!
 * @brief A class for nearest neighbor search
 */
class NSearchKd : public BaseNSearch {

public:
  /*!
   * @brief Constructor
   */
  NSearchKd(size_t debug = 0, double tree_resolution = 1.)
      : BaseNSearch("kdtree", debug) {}

  double setInputCloud() override {
    auto t1 = steady_clock::now();
    d_tree.setInputCloud(d_cloud_p);
    auto t2 = steady_clock::now();
    return util::methods::timeDiff(t1, t2);
  }

public:
  pcl::KdTreeFLANN<pcl::PointXYZ> d_tree;
};

/*!
 * @brief A class for nearest neighbor search
 */
class NSearchOct : public BaseNSearch {

public:
  /*!
   * @brief Constructor
   */
  //NSearch() : d_cloud_p(new pcl::PointCloud<pcl::PointXYZ>), d_tree
  // (false) {};
  NSearchOct(size_t debug = 0, double tree_resolution = 1.)
      : BaseNSearch("octree", debug), d_tree(tree_resolution){}

  double setInputCloud() override {
    auto t1 = steady_clock::now();
    d_tree.setInputCloud(d_cloud_p);
    d_tree.addPointsFromInputCloud();
    auto t2 = steady_clock::now();
    return util::methods::timeDiff(t1, t2);
  }

public:
  pcl::octree::OctreePointCloudSearch<pcl::PointXYZ> d_tree;
};

} // namespace nsearch

#endif // NSEARCH_NSEARCH_H
