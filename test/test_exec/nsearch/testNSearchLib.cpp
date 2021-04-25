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

template <class NSearch>
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

        if (nsearch->radiusSearch(x[i], r, neighs, sqr_dist) >
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

template <class NSearch>
double neighSearchTreeSizet(const std::vector<util::Point> &x,
                       const std::unique_ptr<NSearch> &nsearch, const double &r,
                       std::vector<std::vector<size_t>> &neigh,
                       std::vector<std::vector<float>> &neigh_sq_dist) {
  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());
  auto t1 = steady_clock::now();
  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, x.size(),
      [&x, &neigh, &neigh_sq_dist, &nsearch, r](boost::uint64_t i) {
        std::vector<size_t> neighs;
        std::vector<double> sqr_dist;

        if (nsearch->radiusSearch(x[i], r, neighs, sqr_dist) >
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

  //
  // brute-force search
  //
  double search_r = 1.5 * L;
  auto brute_force_search_time =
      neighSearchBrute(x, search_r, neigh_brute, neigh_brute_sq_dist);

  //
  // pcl tree search
  //
  //  std::unique_ptr<nsearch::NSearchKd> pcl_nsearch = std::make_unique<nsearch::NSearchKd>(0, 1.);
  //  auto pcl_tree_set_time = pcl_nsearch->updatePointCloud(x, true);
  //  pcl_tree_set_time += pcl_nsearch->setInputCloud();
  //
  //  auto pcl_tree_search_time =
  //      neighSearchTree(x, pcl_nsearch, search_r, neigh_pcl, neigh_pcl_sq_dist);

  //
  // nanoflann tree search
  //
  std::unique_ptr<nsearch::NFlannSearchKd> nflann_nsearch = std::make_unique<nsearch::NFlannSearchKd>(x, 0);
  auto nflann_tree_set_time = nflann_nsearch->updatePointCloud(x, true);
  nflann_tree_set_time += nflann_nsearch->setInputCloud();
  auto nflann_tree_search_time =
      neighSearchTreeSizet(x, nflann_nsearch, search_r, neigh_nflann, neigh_nflann_sq_dist);

  //
  // Compare three search lists
  //
  auto nflann_brute_compare = compare_results(
      neigh_nflann, neigh_brute, {"nflann_tree", "brute_force"}, -1, true);

  //  auto nflann_pcl_compare = compare_results(
  //      neigh_nflann, neigh_pcl, {"nflann_tree", "pcl_tree"}, -1, true);
  //
  //  auto pcl_nflann_compare = compare_results(
  //      neigh_pcl, neigh_nflann, {"pcl_tree", "nflann_tree"}, -1, true);

  //  auto pcl_brute_compare = compare_results(
  //      neigh_pcl, neigh_brute, {"pcl_tree", "brute_force"}, -1, true);

  std::ostringstream msg;
  msg << fmt::format("  Setup times: \n"
                     "    nflann_tree_set_time = {}\n",
                     nflann_tree_set_time);

  msg << fmt::format("  Search times: \n"
                     "    brute_force_search_time = {}\n"
                     "    nflann_tree_search_time = {}\n",
                     brute_force_search_time,
                     nflann_tree_search_time);

  msg << fmt::format("  Comparison results: \n"
                     "    nflann_brute_compare: \n{}\n",
                     nflann_brute_compare);

  return msg.str();
}
