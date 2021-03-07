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

  //return RandGenerator();

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
  for (auto &y: x)
    mu += y;
  mu = mu/x.size();

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

  x.resize(Nx*Ny*Nz);

  size_t count = 0;
  for (size_t i=0; i<Nx; i++)
    for (size_t j=0; j<Ny; j++)
      for (size_t k=0; k<Nz; k++) {
        auto y = util::Point();
        y.d_x = i*L + dist(gen);
        y.d_y = j*L + dist(gen);
        y.d_z = k*L + dist(gen);

        x[count] = y;
        count++;
      }
}

double neighSearchTree(const std::vector<util::Point> &x,
                       const std::unique_ptr<NSearch> &nsearch,
                       const double &r,
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

        if (nsearch->d_tree.radiusSearch(
            searchPoint, r, neighs, sqr_dist) > 0) {
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

double neighSearchBrute(const std::vector<util::Point> &x,
                        const double &r,
                        std::vector<std::vector<size_t>> &neigh,
                        std::vector<std::vector<float>> &neigh_sq_dist) {
  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());

  auto t1 = steady_clock::now();
  hpx::parallel::for_loop(
      hpx::parallel::execution::par, 0, x.size(),
      [&x, &neigh, &neigh_sq_dist, r]
          (boost::uint64_t i) {

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
}

void test::testNSearch(size_t N) {

  // create 3D lattice and perturb each lattice point
  int seed = 1000;
  double L = 1.;
  double dL = 0.2;
  size_t Nx, Ny, Nz;
  Nx = Ny = Nz = N;
  size_t N_tot = Nx*Ny*Nz;

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
            << " \n" << std::flush;

  //
  double search_r = 1.5 * L;
  std::cout << "Step 2: Search time \n";
  auto tree_search_time = neighSearchTree(
      x, nsearch, search_r, neigh_tree, neigh_sq_dist_tree);
  std::cout << "    tree_search_time = " << tree_search_time << " \n"
            << std::flush;

  auto brute_search_time = neighSearchBrute(x, search_r, neigh, neigh_sq_dist);
  std::cout << "    brute_search_time = " << brute_search_time << " \n"
            << std::flush;

  //
  std::cout << "Step 3: Compare tree and brute results (match is not "
               "necessary!! \n";
  size_t error_size = 0;
  size_t error_nodes = 0;
  size_t error_neighs = 0;
  std::ostringstream composs;
  for (size_t i=0; i<x.size(); i++) {

    size_t err_neighs = 0;
    auto &tree_neigh = neigh_tree[i];
    auto &brute_neigh = neigh[i];

    bool header_done = false;
    if (tree_neigh.size() != brute_neigh.size()) {
      composs << "    Node = " << i << " \n";
      composs << "      size (tree) " << tree_neigh.size() << " != "
              << brute_neigh.size() << " (brute) not matching\n";

      header_done = true;

      error_size++;
    }

    for (auto j : brute_neigh) {
      if (!isInList(&tree_neigh, j)) {
        if (!header_done)
          composs << "    Node = " << i << " \n";

        composs << "      neigh = " << j << " not found in tree neighs\n";
        err_neighs += 1;
      }
    }

    if (err_neighs > 0)
      error_neighs += err_neighs;
  }
  std::cout << "    error_size = " << error_size << ", error_neighs = "
            << error_neighs << "\n";
  std::cout << composs.str() << "\n";


  //
  std::cout << "Step 4: Change points and redo calculations multiple times \n";
  size_t N_test = 5;
  // to change perturbation size
  RandGenerator gen(get_rd_gen(seed*39));
  UniformDistribution dist(dL*0.5, dL*2.);

  std::vector<double> compute_times_tree(N_test, 0.);
  std::vector<double> compute_times_brute(N_test, 0.);
  for (size_t i=0; i<N_test; i++) {
    double dL_rand = dist(gen);
    lattice(L, Nx, Ny, Nz, dL_rand, seed + i + 1, x);

    std::cout << "    Test number = " << i << "\n";

    auto tree_search_time = neighSearchTree(
        x, nsearch, search_r, neigh_tree, neigh_sq_dist_tree);
    std::cout << "    tree_search_time = " << tree_search_time << " \n"
              << std::flush;

    auto brute_search_time = neighSearchBrute(
        x, search_r, neigh, neigh_sq_dist);
    std::cout << "    brute_search_time = " << brute_search_time << " \n"
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
  std::cout << "    brute state: mean = " << mean_brute
            << ", std = " << std_brute << "\n";
  std::cout << "    tree state: mean = " << mean_tree
            << ", std = " << std_tree << "\n";
}
