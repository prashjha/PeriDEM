/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef NSEARCH_NFLANNSETUP_H
#define NSEARCH_NFLANNSETUP_H

#include "util/point.h" // definition of Point
#include "nanoflann/include/nanoflann.hpp"

namespace nsearch {

/*! @brief Define list of points for tree search using nanoflann lib */
typedef std::vector<util::Point> PointCloud;

/*!
 * @brief Allows custom point cloud data structure to interface with nanoflann.
 * See https://github.com/jlblancoc/nanoflann for more details.
 */
struct PointCloudAdaptor {
  /*! @brief Define coordinate type */
  typedef double coord_t;

  /*! @brief Const reference to list of points */
  const PointCloud &d_obj;

  /*!
   * @brief Constructor
   *
   * @param obj Vector of points
   */
  PointCloudAdaptor(const PointCloud &obj) : d_obj(obj) {}

  /*!
   * @brief Get vector of points
   *
   * @return Vector Vector of points
   */
  inline const PointCloud &pointCloud() const { return d_obj; }

  /*!
   * @brief Get number of points in point cloud
   *
   * @return N Number of points
   */
  inline size_t kdtree_get_point_count() const { return pointCloud().size(); }


  /*!
   * @brief Get specific coordinate of a point
   *
   * @param idx Id of a point
   * @param dim Coordinate id (e.g. 0, 1, 2)
   * @return Coord Coordinate of a point
   */
  inline coord_t kdtree_get_pt(const size_t idx, const size_t dim) const {
    return pointCloud()[idx][dim];
  }

  /*!
   * @brief Optional bounding-box computation: return false to default to a standard
   * bbox computation loop.
   * Return true if the BBOX was already computed by the class and returned in
   * "bb" so it can be avoided to redo it again. Look at bb.size() to find out
   * the expected dimensionality (e.g. 2 or 3 for point clouds)
   *
   * @return Bool False if default bounding-box calculation
   */
  template <class BBOX> bool kdtree_get_bbox(BBOX & /*bb*/) const {
    return false;
  }
};

/*!
 * @brief To collect results of nanoflann tree search. Default result output of
 * nanoflann search uses std::vector<std::pair<size_t, double>>. We prefer to
 * get the index and distance in a separate list.
 */
template <typename _DistanceType, typename _IndexType = size_t>
class TreeSearchResult {
public:
  /*! @brief Distance type (double, float, etc) */
  typedef _DistanceType DistanceType;

  /*! @brief Index type (int, size_t, etc) */
  typedef _IndexType IndexType;

public:
  /*! @brief Define search radius. **Note** this should be square of radius,
   * where radius is a distance within which we are searching for points. */
  const DistanceType d_r;

  /*! @brief Indices within the search radius */
  std::vector<IndexType> &d_indices;

  /*! @brief Distance of points found within the search radius */
  std::vector<DistanceType> &d_dists;

  /*!
  * @brief Constructor
  *
  * @param radius_ Search radius (square of radius)
  * @param indices Reference to index vector
   * @param dists Reference to distance vector
  */
  inline TreeSearchResult(DistanceType radius_,
                            std::vector<IndexType> &indices,
                            std::vector<DistanceType> &dists)
      : d_r(radius_), d_indices(indices), d_dists(dists) {
    init();
  }

  /*!
  * @brief Initialize the data (clear)
  */
  inline void init() { clear(); }

  /*!
  * @brief Clear the data
  */
  inline void clear() {
    d_indices.clear();
    d_dists.clear();
  }

  /*!
  * @brief Get the size of currently stored (found so far) indices
  *
  * @return size Size of indices data
  */
  inline size_t size() const { return d_indices.size(); }

  /*!
  * @brief Check (not implemented)
   * @return bool Status
  */
  inline bool full() const { return true; }

  /*!
  * @brief Called during search to add an element matching the criteria.
  *
  * @param dist Distance of point from the search point
   * @param index Id of point
   * @return True True if continue the search further
  */
  inline bool addPoint(DistanceType dist, IndexType index) {
    if (dist < d_r) {
      d_indices.push_back(index);
      d_dists.push_back(dist);
    }
    return true;
  }

  /*!
  * @brief Return maximum distance for search
   * @return Radius Maximum distance for search
  */
  inline DistanceType worstDist() const { return d_r; }

  /*!
  * @brief Find the worst result (furtherest neighbor) without copying or sorting
   * Pre-conditions: size() > 0
   *
   * Currently, we return pair(0, 0.)
   *
   * @return Pair Pair of id and max distanced point
  */
  std::pair<IndexType, DistanceType> worst_item() const {
    if (d_indices.empty())
      throw std::runtime_error("Cannot invoke RadiusResultSet::worst_item() on "
                               "an empty list of results.");
    return std::make_pair(0, 0.);
  }
};

/*!
 * @brief To collect results of nanoflann tree search. In this class, we check the tag of a potential point
 * and add the point to search list if the tag does not match the tag of a point for which neighbors are computed.
 * This is useful when computing neighbor list for contact.
 * In this case, we do not want points from the same particle to be in the contact neighbor lsit.
 * Default result output of
 * nanoflann search uses std::vector<std::pair<size_t, double>>. We prefer to
 * get the index and distance in a separate list.
 */
template <typename _DistanceType, typename _IndexType = size_t>
class TreeSearchCheckIDExcludeResult {
public:
    /*! @brief Distance type (double, float, etc) */
    typedef _DistanceType DistanceType;

    /*! @brief Index type (int, size_t, etc) */
    typedef _IndexType IndexType;

public:
    /*! @brief Define search radius. **Note** this should be square of radius,
     * where radius is a distance within which we are searching for points. */
    const DistanceType d_r;

    /*! @brief Tag of the point we are searching for neighboring points */
    const IndexType &d_tag;

    /*! @brief Indices within the search radius */
    std::vector<IndexType> &d_indices;

    /*! @brief Distance of points found within the search radius */
    std::vector<DistanceType> &d_dists;

    /*! @brief Tag of point data that we want to check with given point to be added to the search result */
    const std::vector<IndexType> &d_dataTags;

    /*!
    * @brief Constructor
    *
    * @param radius_ Search radius (square of radius)
    * @param indices Reference to index vector
    * @param dists Reference to distance vector
     * @param searchPointTag Tag of a search point
     * @param dataTags Vector of tags for each point in the pointcloud
    */
    inline TreeSearchCheckIDExcludeResult(DistanceType radius_,
                            std::vector<IndexType> &indices,
                            std::vector<DistanceType> &dists,
                            const IndexType &searchPointTag,
                            const std::vector<IndexType> &dataTags)
            : d_r(radius_),
              d_tag(searchPointTag),
              d_indices(indices),
              d_dists(dists),
              d_dataTags(dataTags) {
      init();
    }

    /*!
    * @brief Initialize the data (clear)
    */
    inline void init() { clear(); }

    /*!
    * @brief Clear the data
    */
    inline void clear() {
      d_indices.clear();
      d_dists.clear();
    }

    /*!
    * @brief Get the size of currently stored (found so far) indices
    *
    * @return size Size of indices data
    */
    inline size_t size() const { return d_indices.size(); }

    /*!
    * @brief Check (not implemented)
     * @return bool Status
    */
    inline bool full() const { return true; }

    /*!
    * @brief Called during search to add an element matching the criteria.
    *
    * @param dist Distance of point from the search point
     * @param index Id of point
     * @return True True if continue the search further
    */
    inline bool addPoint(DistanceType dist, IndexType index) {
      if (dist < d_r) {
        if (d_dataTags[index] != d_tag) {
          d_indices.push_back(index);
          d_dists.push_back(dist);
        }
      }
      return true;
    }

    /*!
    * @brief Return maximum distance for search
     * @return Radius Maximum distance for search
    */
    inline DistanceType worstDist() const { return d_r; }

    /*!
    * @brief Find the worst result (furtherest neighbor) without copying or sorting
     * Pre-conditions: size() > 0
     *
     * Currently, we return pair(0, 0.)
     *
     * @return Pair Pair of id and max distanced point
    */
    std::pair<IndexType, DistanceType> worst_item() const {
      if (d_indices.empty())
        throw std::runtime_error("Cannot invoke RadiusResultSet::worst_item() on "
                                 "an empty list of results.");
      return std::make_pair(0, 0.);
    }
};

/*!
 * @brief To collect results of nanoflann tree search. In this class, we check the tag of a potential point
 * and add the point to search list if the tag matches the tag of a point for which neighbors are computed.
 * This is useful when computing neighbor list for peridynamics.
 * In this case, we want points from the same particle to be in the peridynamics neighbor lsit.
 * Default result output of
 * nanoflann search uses std::vector<std::pair<size_t, double>>. We prefer to
 * get the index and distance in a separate list.
 */
template <typename _DistanceType, typename _IndexType = size_t>
class TreeSearchCheckIDIncludeResult {
public:
    /*! @brief Distance type (double, float, etc) */
    typedef _DistanceType DistanceType;

    /*! @brief Index type (int, size_t, etc) */
    typedef _IndexType IndexType;

public:
    /*! @brief Define search radius. **Note** this should be square of radius,
     * where radius is a distance within which we are searching for points. */
    const DistanceType d_r;

    /*! @brief Tag of the point we are searching for neighboring points */
    const IndexType &d_tag;

    /*! @brief Indices within the search radius */
    std::vector<IndexType> &d_indices;

    /*! @brief Distance of points found within the search radius */
    std::vector<DistanceType> &d_dists;

    /*! @brief Tag of point data that we want to check with given point to be added to the search result */
    const std::vector<IndexType> &d_dataTags;

    /*!
    * @brief Constructor
    *
    * @param radius_ Search radius (square of radius)
    * @param indices Reference to index vector
    * @param dists Reference to distance vector
     * @param searchPointTag Tag of a search point
     * @param dataTags Vector of tags for each point in the pointcloud
    */
    inline TreeSearchCheckIDIncludeResult(DistanceType radius_,
                                          std::vector<IndexType> &indices,
                                          std::vector<DistanceType> &dists,
                                          const IndexType &searchPointTag,
                                          const std::vector<IndexType> &dataTags)
            : d_r(radius_),
              d_tag(searchPointTag),
              d_indices(indices),
              d_dists(dists),
              d_dataTags(dataTags) {
      init();
    }

    /*!
    * @brief Initialize the data (clear)
    */
    inline void init() { clear(); }

    /*!
    * @brief Clear the data
    */
    inline void clear() {
      d_indices.clear();
      d_dists.clear();
    }

    /*!
    * @brief Get the size of currently stored (found so far) indices
    *
    * @return size Size of indices data
    */
    inline size_t size() const { return d_indices.size(); }

    /*!
    * @brief Check (not implemented)
     * @return bool Status
    */
    inline bool full() const { return true; }

    /*!
    * @brief Called during search to add an element matching the criteria.
    *
    * @param dist Distance of point from the search point
     * @param index Id of point
     * @return True True if continue the search further
    */
    inline bool addPoint(DistanceType dist, IndexType index) {
      if (dist < d_r) {
        if (d_dataTags[index] == d_tag) {
          d_indices.push_back(index);
          d_dists.push_back(dist);
        }
      }
      return true;
    }

    /*!
    * @brief Return maximum distance for search
     * @return Radius Maximum distance for search
    */
    inline DistanceType worstDist() const { return d_r; }

    /*!
    * @brief Find the worst result (furtherest neighbor) without copying or sorting
     * Pre-conditions: size() > 0
     *
     * Currently, we return pair(0, 0.)
     *
     * @return Pair Pair of id and max distanced point
    */
    std::pair<IndexType, DistanceType> worst_item() const {
      if (d_indices.empty())
        throw std::runtime_error("Cannot invoke RadiusResultSet::worst_item() on "
                                 "an empty list of results.");
      return std::make_pair(0, 0.);
    }
};


/*! @brief Define result attributes */
typedef TreeSearchResult<double, size_t> TreeSearchRes;
typedef TreeSearchCheckIDIncludeResult<double, size_t> TreeSearchCheckIDIncludeRes;
typedef TreeSearchCheckIDExcludeResult<double, size_t> TreeSearchCheckIDExcludeRes;


/*! @brief Define tree data type for nanoflann */
typedef nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<double, PointCloudAdaptor>, PointCloudAdaptor,
    3 /* dim */
>
NFlannKdTree;

/*! @brief Define tree data type for nanoflann (3D) */
typedef nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<double, PointCloudAdaptor>, PointCloudAdaptor,
        3 /* dim */
>
NFlannKdTree3D;

/*! @brief Define tree data type for nanoflann (2D) */
typedef nanoflann::KDTreeSingleIndexAdaptor<
  nanoflann::L2_Simple_Adaptor<double, PointCloudAdaptor>, PointCloudAdaptor,
  2 /* dim */
>
NFlannKdTree2D;

} // namespace nsearch

#endif // NSEARCH_NFLANNSETUP_H
