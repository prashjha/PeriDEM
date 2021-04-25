/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testNSearchLib.h"
#include "nsearch/nsearch.h"
#include "util/function.h"
#include "util/matrix.h"
#include "util/methods.h"
#include "util/point.h"
#include <bitset>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

// hpx lib
#include <hpx/include/parallel_algorithm.hpp>

typedef nsearch::NSearchKd NSearch;

typedef std::mt19937 RandGenerator;
typedef std::uniform_real_distribution<> UniformDistribution;

namespace {

RandGenerator get_rd_gen(int seed) {

  // return RandGenerator();

  if (seed < 0) {
    std::random_device rd;
    seed = rd();
  }

  return RandGenerator(seed);
}

bool isInList(const std::vector<size_t> *list, size_t i) {
  for (auto j : *list)
    if (j == i)
      return true;

  return false;
}

void stats(const std::vector<double> &x, double &mean, double &std) {
  double mu = 0.;
  for (auto &y : x)
    mu += y;
  mu = mu / x.size();

  double s = 0.;
  for (auto &y : x)
    s += (y - mu) * (y - mu);
  s = s / x.size();

  std = std::sqrt(s);
  mean = mu;
}

void lattice(double L, size_t Nx, size_t Ny, size_t Nz, double dL, int seed,
             std::vector<util::Point> &x) {

  RandGenerator gen(get_rd_gen(seed));
  UniformDistribution dist(-dL, dL);

  x.resize(Nx * Ny * Nz);

  size_t count = 0;
  for (size_t i = 0; i < Nx; i++)
    for (size_t j = 0; j < Ny; j++)
      for (size_t k = 0; k < Nz; k++) {
        auto y = util::Point();
        y.d_x = i * L + dist(gen);
        y.d_y = j * L + dist(gen);
        y.d_z = k * L + dist(gen);

        x[count] = y;
        count++;
      }
}

double neighSearchTree(const std::vector<util::Point> &x,
                       const std::unique_ptr<NSearch> &nsearch, const double &r,
                       std::vector<std::vector<size_t>> &neigh,
                       std::vector<std::vector<float>> &neigh_sq_dist) {
  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());
  auto t1 = steady_clock::now();
  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, x.size(),
      [&x, &neigh, &neigh_sq_dist, &nsearch, r](boost::uint64_t i) {
        std::vector<int> neighs;
        std::vector<float> sqr_dist;

        pcl::PointXYZ searchPoint;
        searchPoint.x = x[i].d_x;
        searchPoint.y = x[i].d_y;
        searchPoint.z = x[i].d_z;

        if (nsearch->d_tree.radiusSearch(searchPoint, r, neighs, sqr_dist) >
            0) {
          for (std::size_t j = 0; j < neighs.size(); ++j)
            if (size_t(neighs[j]) != i) {
              neigh[i].push_back(size_t(neighs[j]));
              neigh_sq_dist[i].push_back(sqr_dist[j]);
            }
        }
      });
  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2);
}

double neighSearchBrute(const std::vector<util::Point> &x, const double &r,
                        std::vector<std::vector<size_t>> &neigh,
                        std::vector<std::vector<float>> &neigh_sq_dist) {
  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());

  auto t1 = steady_clock::now();
  hpx::parallel::for_loop(hpx::parallel::execution::par, 0, x.size(),
                          [&x, &neigh, &neigh_sq_dist, r](boost::uint64_t i) {
                            auto searchPoint = x[i];

                            for (size_t j = 0; j < x.size(); j++) {
                              auto dx = searchPoint - x[j];
                              auto l = dx.length();
                              if (util::isLess(l, r) and j != i) {
                                neigh[i].push_back(j);
                                neigh_sq_dist[i].push_back(l);
                              }
                            }
                          });
  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2);
}

std::string compare_results(const std::vector<std::vector<size_t>> &neigh1,
                            const std::vector<std::vector<size_t>> &neigh2,
                            std::vector<std::string> tags,
                            int check_nodes_num = -1,
                            bool only_err_count = false) {
  size_t error_size = 0;
  size_t error_nodes = 0;
  size_t error_neighs = 0;
  std::ostringstream composs;
  for (size_t i = 0; i < neigh1.size(); i++) {

    if (check_nodes_num > 0 and i > check_nodes_num)
      continue;

    size_t err_neighs = 0;
    auto &n1 = neigh1[i];
    auto &n2 = neigh2[i];

    bool header_done = false;
    if (n1.size() != n2.size()) {
      composs << "    Node = " << i << " \n";
      composs << fmt::format("      size ({}) {} != {} ({}) not matching\n",
                             tags[0], n1.size(), n2.size(), tags[1]);
      header_done = true;
      error_size++;
    }

    for (auto j : n2) {
      if (!isInList(&n1, j)) {
        if (!header_done)
          composs << "    Node = " << i << " \n";

        composs << fmt::format("      neigh = {} in {} search not found in {} "
                               "search neighs list\n",
                               j, tags[1], tags[0]);
        err_neighs += 1;
      }
    }

    if (err_neighs > 0)
      error_neighs += err_neighs;
  }

  if (only_err_count)
    return fmt::format("    error_size = {}, error_neighs = {}\n", error_size,
                       error_neighs);
  else
    return fmt::format("    error_size = {}, error_neighs = {}\n", error_size,
                       error_neighs) +
           composs.str();
}
} // namespace

// nanoflann setup
namespace {
typedef std::vector<util::Point> PointCloud;

// And this is the "dataset to kd-tree" adaptor class:
struct PointCloudAdaptor {
  typedef double coord_t;

  const PointCloud &d_obj; //!< A const ref to the data set origin

  /// The constructor that sets the data set source
  PointCloudAdaptor(const PointCloud &obj_) : d_obj(obj_) {}

  /// CRTP helper method
  inline const PointCloud &pointCloud() const { return d_obj; }

  // Must return the number of data points
  inline size_t kdtree_get_point_count() const { return pointCloud().size(); }

  // Returns the dim'th component of the idx'th point in the class:
  // Since this is inlined and the "dim" argument is typically an immediate
  // value, the
  //  "if/else's" are actually solved at compile time.
  inline coord_t kdtree_get_pt(const size_t idx, const size_t dim) const {
    return pointCloud()[idx][dim];
  }

  // Optional bounding-box computation: return false to default to a standard
  // bbox computation loop.
  //   Return true if the BBOX was already computed by the class and returned in
  //   "bb" so it can be avoided to redo it again. Look at bb.size() to find out
  //   the expected dimensionality (e.g. 2 or 3 for point clouds)
  template <class BBOX> bool kdtree_get_bbox(BBOX & /*bb*/) const {
    return false;
  }

}; // end of PointCloudAdaptor

template <typename _DistanceType, typename _IndexType = size_t>
class RadiusResultSetNew {
public:
  typedef _DistanceType DistanceType;
  typedef _IndexType IndexType;

public:
  const DistanceType radius;

  std::vector<IndexType> &m_indices;
  std::vector<DistanceType> &m_dists;

  inline RadiusResultSetNew(DistanceType radius_,
                            std::vector<IndexType> &indices,
                            std::vector<DistanceType> &dists)
      : radius(radius_), m_indices(indices), m_dists(dists) {
    init();
  }

  inline void init() { clear(); }
  inline void clear() {
    m_indices.clear();
    m_dists.clear();
  }

  inline size_t size() const { return m_indices.size(); }

  inline bool full() const { return true; }

  /**
   * Called during search to add an element matching the criteria.
   * @return true if the search should be continued, false if the results are
   * sufficient
   */
  inline bool addPoint(DistanceType dist, IndexType index) {
    if (dist < radius) {
      m_indices.push_back(index);
      m_dists.push_back(dist);
    }
    return true;
  }

  inline DistanceType worstDist() const { return radius; }

  /**
   * Find the worst result (furtherest neighbor) without copying or sorting
   * Pre-conditions: size() > 0
   */
  std::pair<IndexType, DistanceType> worst_item() const {
    if (m_indices.empty())
      throw std::runtime_error("Cannot invoke RadiusResultSet::worst_item() on "
                               "an empty list of results.");
    return std::make_pair(0, 0.);
  }
};
} // namespace

void test::testNSearch(size_t N) {

  // create 3D lattice and perturb each lattice point
  int seed = 1000;
  double L = 1.;
  double dL = 0.2;
  size_t Nx, Ny, Nz;
  Nx = Ny = Nz = N;
  size_t N_tot = Nx * Ny * Nz;

  std::vector<util::Point> x(N_tot, util::Point());
  lattice(L, Nx, Ny, Nz, dL, seed, x);
  std::cout << "Total points = " << x.size() << "\n";

  std::vector<std::vector<size_t>> neigh_tree(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_sq_dist_tree(N_tot,
                                                     std::vector<float>());
  std::vector<std::vector<size_t>> neigh(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_sq_dist(N_tot, std::vector<float>());

  // create search object and report time needed for creation
  std::cout << "Step 1: Initial setup \n";
  std::unique_ptr<NSearch> nsearch = std::make_unique<NSearch>(0, 1.);
  auto set_cloud_pts_time = nsearch->updatePointCloud(x, true);
  auto set_tree_time = nsearch->setInputCloud();
  std::cout << "    tree_setup_time = " << set_cloud_pts_time + set_tree_time
            << " \n"
            << std::flush;

  //
  double search_r = 1.5 * L;
  std::cout << "Step 2: Search time \n";
  auto tree_search_time =
      neighSearchTree(x, nsearch, search_r, neigh_tree, neigh_sq_dist_tree);
  std::cout << "    tree_search_time = " << tree_search_time << " \n"
            << std::flush;

  auto brute_search_time = neighSearchBrute(x, search_r, neigh, neigh_sq_dist);
  std::cout << "    brute_force_search_time = " << brute_search_time << " \n"
            << std::flush;

  //
  std::cout << "Step 3: Compare tree and brute results (match is not "
               "necessary!! \n";
  std::cout << compare_results(neigh_tree, neigh, {"pcl_tree", "brute_force"});

  //
  std::cout << "Step 4: Change points and redo calculations multiple times \n";
  size_t N_test = 5;
  // to change perturbation size
  RandGenerator gen(get_rd_gen(seed * 39));
  UniformDistribution dist(dL * 0.5, dL * 2.);

  std::vector<double> compute_times_tree(N_test, 0.);
  std::vector<double> compute_times_brute(N_test, 0.);
  for (size_t i = 0; i < N_test; i++) {
    double dL_rand = dist(gen);
    lattice(L, Nx, Ny, Nz, dL_rand, seed + i + 1, x);

    std::cout << "    Test number = " << i << "\n";

    auto tree_search_time =
        neighSearchTree(x, nsearch, search_r, neigh_tree, neigh_sq_dist_tree);
    std::cout << "    tree_search_time = " << tree_search_time << " \n"
              << std::flush;

    auto brute_search_time =
        neighSearchBrute(x, search_r, neigh, neigh_sq_dist);
    std::cout << "    brute_force_search_time = " << brute_search_time << " \n"
              << std::flush;

    compute_times_tree[i] = tree_search_time;
    compute_times_brute[i] = brute_search_time;
  }

  // compute stats and report
  double mean_brute = 0., std_brute = 0.;
  stats(compute_times_brute, mean_brute, std_brute);

  double mean_tree = 0., std_tree = 0.;
  stats(compute_times_tree, mean_tree, std_tree);

  std::cout << "\n";
  std::cout << "    brute_force search: mean = " << mean_brute
            << ", std = " << std_brute << "\n";
  std::cout << "    pcl_tree search: mean = " << mean_tree
            << ", std = " << std_tree << "\n";
}

std::string test::testNanoflann(size_t N, double L, double dL, int seed) {

  // create 3D lattice and perturb each lattice point
  // int seed = 1020;
  // double L = 1.;
  // double dL = 0.2;
  size_t Nx, Ny, Nz;
  Nx = Ny = Nz = N;
  size_t N_tot = Nx * Ny * Nz;

  std::vector<util::Point> x(N_tot, util::Point());
  lattice(L, Nx, Ny, Nz, dL, seed, x);
  std::cout << "Total points = " << x.size() << "\n";

  std::vector<std::vector<size_t>> neigh_nflann(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_nflann_sq_dist(N_tot,
                                                       std::vector<float>());

  std::vector<std::vector<size_t>> neigh_brute(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_brute_sq_dist(N_tot,
                                                      std::vector<float>());

  std::vector<std::vector<size_t>> neigh_pcl(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_pcl_sq_dist(N_tot,
                                                    std::vector<float>());

  // brute-force search
  // std::cout << "Step 1: Search time \n";
  double search_r = 1.5 * L;
  auto brute_search_time =
      neighSearchBrute(x, search_r, neigh_brute, neigh_brute_sq_dist);
  // std::cout << "    brute_force_search_time = " << brute_search_time << " \n"
  //          << std::flush;

  // nsearch pcl
  std::unique_ptr<NSearch> nsearch = std::make_unique<NSearch>(0, 1.);
  auto set_pcl_cloud_pts_time = nsearch->updatePointCloud(x, true);
  auto set_pcl_tree_time = nsearch->setInputCloud() + set_pcl_cloud_pts_time;
  // std::cout << "    pcl_tree_setup_time = " << set_pcl_tree_time
  //          << " \n" << std::flush;

  auto tree_pcl_search_time =
      neighSearchTree(x, nsearch, search_r, neigh_pcl, neigh_pcl_sq_dist);
  // std::cout << "    pcl_tree_search_time = " << tree_pcl_search_time << " \n"
  //          << std::flush;

  const PointCloudAdaptor pc2kd(x); // The adaptor

  // construct a kd-tree index:
  typedef nanoflann::KDTreeSingleIndexAdaptor<
      nanoflann::L2_Simple_Adaptor<double, PointCloudAdaptor>,
      PointCloudAdaptor, 3 /* dim */
      >
      myKdtree_t;

  myKdtree_t index(
      3 /*dim*/, pc2kd,
      nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
  auto t1 = steady_clock::now();
  index.buildIndex();
  auto set_nflann_tree_time = util::methods::timeDiff(t1, steady_clock::now());
  // std::cout << "    nflann_tree_setup_time = " << set_nflann_tree_time
  //          << " \n" << std::flush;

  // search points in neighborhood for all points in x
  t1 = steady_clock::now();
  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, x.size(),
      [&x, &index, &neigh_nflann, &neigh_nflann_sq_dist,
       search_r](boost::uint64_t i) {
        auto searchPoint = x[i];

        double query_pt[3] = {searchPoint[0], searchPoint[1], searchPoint[2]};

        std::vector<size_t> neigh_test;
        std::vector<double> neigh_dist_test;

        nanoflann::SearchParams params;
        params.sorted = false;

        typedef RadiusResultSetNew<double, size_t> ResSetNew;
        ResSetNew resultSet(search_r * search_r, neigh_test, neigh_dist_test);
        const size_t nMatches =
            index.radiusSearchCustomCallback(&query_pt[0], resultSet, params);

        std::vector<std::pair<size_t, double>> ret_matches;
        // const size_t nMatches = index.radiusSearch(&query_pt[0],
        // search_r*search_r, ret_matches, params);

        if (nMatches > 0) {
          for (std::size_t j = 0; j < nMatches; ++j)
            // if (ret_matches[j].first != i) {
            //  neigh_tree[i].push_back(size_t(ret_matches[j].first));
            //  neigh_sq_dist_tree[i].push_back(ret_matches[j].second);
            //}

            if (neigh_test[j] != i) {
              neigh_nflann[i].push_back(size_t(neigh_test[j]));
              neigh_nflann_sq_dist[i].push_back(neigh_dist_test[j]);
            }
        }
      });

  auto nflann_tree_search_time =
      util::methods::timeDiff(t1, steady_clock::now());
  // std::cout << "    nflann_tree_search_time = " << nflann_tree_search_time <<
  // " \n"
  //          << std::flush;

  // std::cout << "\n\nCompare nflann tree and brute force results \n";
  auto nflann_brute_compare = compare_results(
      neigh_nflann, neigh_brute, {"nflann_tree", "brute_force"}, -1, true);

  // std::cout << "\n\nCompare nflann tree and pcl tree results \n";
  auto nflann_pcl_compare = compare_results(
      neigh_nflann, neigh_pcl, {"nflann_tree", "pcl_tree"}, -1, true);

  // std::cout << "\n\nCompare pcl tree and brute force results \n";
  auto pcl_brute_compare = compare_results(
      neigh_pcl, neigh_brute, {"pcl_tree", "brute_force"}, -1, true);

  std::ostringstream msg;
  msg << fmt::format("  Setup times: \n"
                     "    pcl_tree_setup_time = {}\n"
                     "    nflann_tree_setup_time = {}\n",
                     set_pcl_tree_time, set_nflann_tree_time);

  msg << fmt::format("  Search times: \n"
                     "    brute_force_search_time = {}\n"
                     "    pcl_tree_search_time = {}\n"
                     "    nflann_tree_search_time = {}\n",
                     brute_search_time,
                     tree_pcl_search_time, nflann_tree_search_time);

  msg << fmt::format("  Comparison results: \n"
                     "    nflann_pcl_compare: \n{}\n"
                     "    nflann_brute_compare: \n{}\n"
                     "    pcl_brute_compare: \n{}\n",
                     nflann_pcl_compare, nflann_brute_compare,
                     pcl_brute_compare);

  return msg.str();
}
