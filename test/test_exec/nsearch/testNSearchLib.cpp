/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "testNSearchLib.h"
#include "nsearch/nsearch.h"
#include "util/function.h"
#include "util/matrix.h"
#include "util/vecMethods.h"
#include "util/point.h"
#include "util/randomDist.h"
#include "util/parallelUtil.h"
#include <format>
#include <print>
#include <bitset>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

namespace {

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
             std::vector<util::Point> &x, int dim = 3) {

  RandGenerator gen(util::get_rd_gen(seed));
  UniformDistribution dist(-dL, dL);

  if (dim == 2)
    x.resize(Nx * Ny);
  else if (dim == 3)
    x.resize(Nx * Ny * Nz);
  else {
    std::cerr << "Dimension = " << dim << " not supprted.\n";
    exit(EXIT_FAILURE);
  }

  size_t count = 0;
  for (size_t i = 0; i < Nx; i++) {
    for (size_t j = 0; j < Ny; j++) {
      if (dim == 2) {
        auto y = util::Point();
        y.d_x = i * L + dist(gen);
        y.d_y = j * L + dist(gen);
        y.d_z = 0.;

        x[count] = y;
        count++;
      } else if (dim == 3) {
        for (size_t k = 0; k < Nz; k++) {
          auto y = util::Point();
          y.d_x = i * L + dist(gen);
          y.d_y = j * L + dist(gen);
          y.d_z = k * L + dist(gen);

          x[count] = y;
          count++;
        } // loop over k
      } // if-else
    } // loop over j
  } // loop over i
}

void assignRandomTags(std::vector<util::Point> &x,
                      int numTags,
                      int seed,
                      std::vector<size_t> &xTags) {
  RandGenerator gen(util::get_rd_gen(seed));
  UniformIntDistribution dist(0, numTags - 1);

  for (size_t i=0; i<x.size(); i++)
    xTags[i] = dist(gen);
}

template <class NSearch>
double neighSearchTree(const std::vector<util::Point> &x,
                       const std::unique_ptr<NSearch> &nsearch,
                       const double &r,
                       std::vector<std::vector<size_t>> &neigh,
                       std::vector<std::vector<float>> &neigh_sq_dist) {
  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());
  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, x.size(), (std::size_t) 1, [&x, &neigh, &neigh_sq_dist, &nsearch, r](std::size_t i) {
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
    }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2, "microseconds");
}

template <class NSearch>
double neighSearchTreeSizet(const std::vector<util::Point> &x,
                       const std::unique_ptr<NSearch> &nsearch,
                       const double &r,
                       std::vector<std::vector<size_t>> &neigh,
                       std::vector<std::vector<float>> &neigh_sq_dist) {
  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());
  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, x.size(), (std::size_t) 1, [&x, &neigh, &neigh_sq_dist, &nsearch, r](std::size_t i) {
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
    }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2, "microseconds");
}

template <class NSearch>
double neighSearchTreeSizetExcludeInclude(const std::vector<util::Point> &x,
                            const std::vector<size_t> &xTags,
                            const std::unique_ptr<NSearch> &nsearch,
                            const double &r,
                            std::vector<std::vector<size_t>> &neigh,
                            std::vector<std::vector<float>> &neigh_sq_dist,
                            int selection_criteria) {

  // selection_criteria = 1 --> exclude search
  // selection_criteria = 2 --> include search

  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());
  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0,
                          x.size(),
                          (std::size_t) 1,
                          [&x,
                           &xTags,
                           &neigh,
                           &neigh_sq_dist,
                           &nsearch,
                           r,
                           selection_criteria](std::size_t i) {

          std::vector<size_t> neighs;
          std::vector<double> sqr_dist;
          size_t n = 0;

          if (selection_criteria == 1)
            n = nsearch->radiusSearchExcludeTag(x[i], r,
                                                neighs,
                                                sqr_dist,
                                                xTags[i],
                                                xTags);
          else if (selection_criteria == 2)
            n = nsearch->radiusSearchIncludeTag(x[i], r,
                                                neighs,
                                                sqr_dist,
                                                xTags[i],
                                                xTags);
          if (n > 0) {
            for (std::size_t j = 0; j < neighs.size(); ++j)
              if (size_t(neighs[j]) != i) {
                neigh[i].push_back(size_t(neighs[j]));
                neigh_sq_dist[i].push_back(sqr_dist[j]);
              }
          }
      }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2, "microseconds");
}

double neighSearchBrute(const std::vector<util::Point> &x, const double &r,
                        std::vector<std::vector<size_t>> &neigh,
                        std::vector<std::vector<float>> &neigh_sq_dist) {

  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());

  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0,
                          x.size(),
                          (std::size_t) 1,
                          [&x,
                           &neigh,
                           &neigh_sq_dist,
                           r](std::size_t i) {
      auto searchPoint = x[i];

      for (size_t j = 0; j < x.size(); j++) {
        auto dx = searchPoint - x[j];
        auto l = dx.length();
        if (util::isLess(l, r) and j != i) {
          neigh[i].push_back(j);
          neigh_sq_dist[i].push_back(l);
        }
      }
    }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2, "microseconds");
}


double neighSearchBruteExcludeInclude(const std::vector<util::Point> &x,
                               const std::vector<size_t> &xTags,
                               const double &r,
                        std::vector<std::vector<size_t>> &neigh,
                        std::vector<std::vector<float>> &neigh_sq_dist,
                        int selection_criteria) {

  // selection_criteria = 1 --> exclude search
  // selection_criteria = 2 --> include search

  neigh.resize(x.size());
  neigh_sq_dist.resize(x.size());

  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0,
                          x.size(),
                          (std::size_t) 1,
                          [&x,
                           &xTags,
                           &neigh,
                           &neigh_sq_dist,
                           r,
                           selection_criteria](std::size_t i) {

        auto searchPoint = x[i];
        auto searchPointTag = xTags[i];

        for (size_t j = 0; j < x.size(); j++) {
          auto dx = searchPoint - x[j];
          auto l = dx.length();
          if (util::isLess(l, r) and j != i) {

            if (selection_criteria == 1) {
              if (xTags[j] != searchPointTag) {
                neigh[i].push_back(j);
                neigh_sq_dist[i].push_back(l);
              }
            }
            else if (selection_criteria == 2) {
              if (xTags[j] == searchPointTag) {
                neigh[i].push_back(j);
                neigh_sq_dist[i].push_back(l);
              }
            }
          }
        }
    }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2, "microseconds");
}


template <class NSearch>
double neighSearchTreeClosestPointSizet(const std::vector<util::Point> &x,
                            const std::unique_ptr<NSearch> &nsearch,
                            const int &seed,
                            const double &L,
                            const double &dL,
                            std::vector<util::Point> &search_points,
                            std::vector<size_t> &err_points,
                            std::vector<double> &err_dist) {

  search_points.resize(x.size());
  err_points.resize(x.size());
  err_dist.resize(x.size());

  // set perturbation length much smaller than the dL
  double dx_perturb = dL/100;

  // random number generator
  RandGenerator gen(util::get_rd_gen(seed));
  UniformDistribution dist(-dx_perturb, dx_perturb);

  for (size_t i=0; i<x.size(); i++) {
    const auto &xi = x[i];

    // get a query point that is close to xi
    auto xi_perturb = xi;
    for (size_t k=0; k<3; k++)
      xi_perturb[k] += dist(gen);

    search_points[i] = xi_perturb;
  }

  auto t1 = steady_clock::now();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, x.size(), (std::size_t) 1, [&x,
                                                                       &search_points,
                                                                       &err_points, &err_dist, &nsearch](std::size_t i) {

      const auto &xi = x[i];

      // get a query point that is close to xi
      const auto &xi_perturb = search_points[i];

      // set true values
      size_t true_neigh = i;
      double true_dist = (xi - xi_perturb).length();

      size_t found_neigh;
      double found_dist = 0.;

      // find closest neighbor using nsearch
      nsearch->closestPoint(xi_perturb, found_neigh, found_dist);

      if (true_neigh != found_neigh) {
        err_points[i] = found_neigh;
        err_dist[i] = true_dist - found_dist;
      }
      else {
        err_points[i] = -1;
        err_dist[i] = 0.;
      }
    }
  ); // for_each

  executor.run(taskflow).get();

  auto t2 = steady_clock::now();
  return util::methods::timeDiff(t1, t2, "microseconds");
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
      composs << std::format("      size ({}) {} != {} ({}) not matching\n",
                             tags[0], n1.size(), n2.size(), tags[1]);
      header_done = true;
      error_size++;
    }

    for (auto j : n2) {
      if (!isInList(&n1, j)) {
        if (!header_done)
          composs << "    Node = " << i << " \n";

        composs << std::format("      neigh = {} in {} search not found in {} "
                               "search neighs list\n",
                               j, tags[1], tags[0]);
        err_neighs += 1;
      }
    }

    if (err_neighs > 0)
      error_neighs += err_neighs;
  }

  if (only_err_count)
    return std::format("    error_size = {}, error_neighs = {}\n", error_size,
                       error_neighs);
  else
    return std::format("    error_size = {}, error_neighs = {}\n", error_size,
                       error_neighs) +
           composs.str();
}

    std::string compare_closest_point_results(
            const std::vector<util::Point> &x,
            const std::vector<util::Point> &search_points,
            const std::vector<size_t> &err_points,
            const std::vector<double> &err_dist,
            bool only_err_count = false) {

      size_t error_size = 0;
      std::ostringstream composs;

      for (size_t i = 0; i < err_points.size(); i++) {

        if (err_points[i] != -1) {
          composs << std::format("    Node = {} at location = {}, "
                                 "Search point = {}, closest point id = {} at"
                                 " location = {}. True dist = {}, found "
                                 "dist = {}\n",
                                 i, x[i].printStr(), search_points[i]
                                 .printStr(), err_points[i], x[err_points[i]]
                                 .printStr(), (x[i] - search_points[i])
                                 .length(), err_dist[i]);
          error_size++;
        }
      }

      if (only_err_count)
        return std::format("    error_size = {}\n", error_size);
      else
        return std::format("    error_size = {}\n", error_size) +
               composs.str();
    }
} // namespace

template <int dim>
std::string test::testNanoflann(size_t N, double L, double dL, int seed) {

  if (dim < 2 or dim > 3) {
    return "testNanoflann: only dim = 2, 3 are accepted.\n";
  }

  // create 3D lattice and perturb each lattice point
  size_t Nx, Ny, Nz;
  Nx = Ny = Nz = N;
  size_t N_tot = Nx * Ny;
  if (dim == 3)
    N_tot = N_tot * Nz;

  std::vector<util::Point> x(N_tot, util::Point());
  lattice(L, Nx, Ny, Nz, dL, seed, x, dim);
  std::cout << "Total points = " << x.size() << "\n";


  if (dim == 2) {
    std::vector<std::vector<size_t>> neigh_nflann(N_tot, std::vector<size_t>());
    std::vector<std::vector<float>> neigh_nflann_sq_dist(N_tot,
                                                         std::vector<float>());

    std::vector<std::vector<size_t>> neigh_nflann_3d(N_tot, std::vector<size_t>());
    std::vector<std::vector<float>> neigh_nflann_3d_sq_dist(N_tot,
                                                         std::vector<float>());

    std::vector<std::vector<size_t>> neigh_brute(N_tot, std::vector<size_t>());
    std::vector<std::vector<float>> neigh_brute_sq_dist(N_tot,
                                                        std::vector<float>());

    // brute-force search
    double search_r = 1.5 * L;
    auto brute_force_search_time =
            neighSearchBrute(x, search_r, neigh_brute, neigh_brute_sq_dist);

    // nanoflann tree search
    std::unique_ptr<nsearch::NFlannSearchKd<3>> nflann_nsearch_3d
            = std::make_unique<nsearch::NFlannSearchKd<3>>(x, 0);

    std::unique_ptr<nsearch::NFlannSearchKd<dim>> nflann_nsearch
            = std::make_unique<nsearch::NFlannSearchKd<dim>>(x, 0);

    auto nflann_tree_set_time_3d = nflann_nsearch_3d->setInputCloud();
    auto nflann_tree_search_time_3d =
            neighSearchTreeSizet(x, nflann_nsearch_3d, search_r, neigh_nflann_3d, neigh_nflann_3d_sq_dist);

    auto nflann_tree_set_time = nflann_nsearch->setInputCloud();
    auto nflann_tree_search_time =
            neighSearchTreeSizet(x, nflann_nsearch, search_r, neigh_nflann, neigh_nflann_sq_dist);

    // Compare search results
    auto nflann_brute_compare = compare_results(
            neigh_nflann, neigh_brute, {"nflann_tree", "brute_force"}, -1, true);

    auto nflann_brute_compare_3d = compare_results(
            neigh_nflann_3d, neigh_brute, {"nflann_tree-3d", "brute_force"}, -1, true);

    std::ostringstream msg;
    msg << std::format("  Setup times (microseconds): \n"
                       "    nflann_tree_set_time = {} \n "
                       "    nflann_tree_set_time_3d = {}\n",
                       nflann_tree_set_time, nflann_tree_set_time_3d);

    msg << std::format("  Search times (microseconds): \n"
                       "    brute_force_search_time = {}\n"
                       "    nflann_tree_search_time = {}\n"
                       "    nflann_tree_search_time_3d = {}\n",
                       brute_force_search_time,
                       nflann_tree_search_time,
                       nflann_tree_search_time_3d);

    msg << std::format("  Comparison results: \n"
                       "    nflann_brute_compare: \n{}\n",
                       nflann_brute_compare);

    msg << std::format("  Comparison results: \n"
                       "    nflann_brute_compare_3d: \n{}\n",
                       nflann_brute_compare_3d);

    return msg.str();
  }
  else if (dim == 3) {
    std::vector<std::vector<size_t>> neigh_nflann(N_tot, std::vector<size_t>());
    std::vector<std::vector<float>> neigh_nflann_sq_dist(N_tot,
                                                         std::vector<float>());

    std::vector<std::vector<size_t>> neigh_brute(N_tot, std::vector<size_t>());
    std::vector<std::vector<float>> neigh_brute_sq_dist(N_tot,
                                                        std::vector<float>());

    // brute-force search
    double search_r = 1.5 * L;
    auto brute_force_search_time =
            neighSearchBrute(x, search_r, neigh_brute, neigh_brute_sq_dist);

    // nanoflann tree search
    std::unique_ptr<nsearch::NFlannSearchKd<dim>> nflann_nsearch
            = std::make_unique<nsearch::NFlannSearchKd<dim>>(x, 0);

    auto nflann_tree_set_time = nflann_nsearch->setInputCloud();
    auto nflann_tree_search_time =
            neighSearchTreeSizet(x, nflann_nsearch, search_r, neigh_nflann, neigh_nflann_sq_dist);

    // Compare search results
    auto nflann_brute_compare = compare_results(
            neigh_nflann, neigh_brute, {"nflann_tree", "brute_force"}, -1, true);

    std::ostringstream msg;
    msg << std::format("  Setup times (microseconds): \n"
                       "    nflann_tree_set_time = {}\n",
                       nflann_tree_set_time);

    msg << std::format("  Search times (microseconds): \n"
                       "    brute_force_search_time = {}\n"
                       "    nflann_tree_search_time = {}\n",
                       brute_force_search_time,
                       nflann_tree_search_time);

    msg << std::format("  Comparison results: \n"
                       "    nflann_brute_compare: \n{}\n",
                       nflann_brute_compare);

    return msg.str();
  }


}


template <int dim>
std::string test::testNanoflannExcludeInclude(size_t N, double L,
         double dL, int seed, testNSearchData &data) {

  if (dim < 2 or dim > 3) {
    return "testNanoflannExcludeInclude: only dim = 2, 3 are accepted.\n";
  }

  // create 3D lattice and perturb each lattice point
  size_t Nx, Ny, Nz;
  Nx = Ny = Nz = N;
  size_t N_tot = Nx * Ny;
  if (dim == 3)
    N_tot = N_tot * Nz;

  std::vector<util::Point> x(N_tot, util::Point());
  std::vector<size_t> xTags(N_tot, 0);
  lattice(L, Nx, Ny, Nz, dL, seed, x, dim);
  assignRandomTags(x, data.d_numTags, seed, xTags);
  data.d_numPoints = x.size();
  std::cout << "Total points = " << x.size() << "\n";

  std::vector<std::vector<size_t>> neigh_default_nflann(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_default_nflann_sq_dist(N_tot,
                                                               std::vector<float>());

  std::vector<std::vector<size_t>> neigh_exclude_nflann(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_exclude_nflann_sq_dist(N_tot,
                                                       std::vector<float>());

  std::vector<std::vector<size_t>> neigh_include_nflann(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_include_nflann_sq_dist(N_tot,
                                                               std::vector<float>());


  std::vector<std::vector<size_t>> neigh_default_brute(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_default_brute_sq_dist(N_tot,
                                                              std::vector<float>());

  std::vector<std::vector<size_t>> neigh_exclude_brute(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_exclude_brute_sq_dist(N_tot,
                                                      std::vector<float>());

  std::vector<std::vector<size_t>> neigh_include_brute(N_tot, std::vector<size_t>());
  std::vector<std::vector<float>> neigh_include_brute_sq_dist(N_tot,
                                                              std::vector<float>());

  // brute-force search
  double search_r = 3. * L;

  data.d_defaultBruteSearchTime =
          neighSearchBrute(x,
                           search_r,
                           neigh_default_brute,
                           neigh_default_brute_sq_dist);

  data.d_excludeBruteSearchTime =
          neighSearchBruteExcludeInclude(x, xTags, search_r,
                                  neigh_exclude_brute,
                                  neigh_exclude_brute_sq_dist,
                                  1);

  data.d_includeBruteSearchTime =
          neighSearchBruteExcludeInclude(x, xTags, search_r,
                                  neigh_include_brute,
                                  neigh_include_brute_sq_dist,
                                  2);

  // nanoflann tree search
  std::unique_ptr<nsearch::NFlannSearchKd<dim>> nflann_nsearch
          = std::make_unique<nsearch::NFlannSearchKd<dim>>(x, 0,
                  data.d_leafMaxSize);

  data.d_treeBuildTime = nflann_nsearch->setInputCloud();

  // default
  data.d_defaultNFlannSearchTime =
          neighSearchTreeSizet(x, nflann_nsearch,
                               search_r, neigh_default_nflann,
                               neigh_default_nflann_sq_dist);

  // exclude
  data.d_excludeNFlannSearchTime =
          neighSearchTreeSizetExcludeInclude(x, xTags, nflann_nsearch,
                                      search_r, neigh_exclude_nflann,
                                      neigh_exclude_nflann_sq_dist,
                                      1);

  // include
  data.d_includeNFlannSearchTime =
          neighSearchTreeSizetExcludeInclude(x, xTags, nflann_nsearch,
                                      search_r, neigh_include_nflann,
                                      neigh_include_nflann_sq_dist,
                                      2);

  // Compare search results
  auto nflann_brute_compare_default = compare_results(
          neigh_default_nflann, neigh_default_brute,
          {"nflann_tree_default", "brute_force_default"}, -1, true);

  auto nflann_brute_compare_exclude = compare_results(
          neigh_exclude_nflann, neigh_exclude_brute,
          {"nflann_tree_exclude", "brute_force_exclude"}, -1, true);

  auto nflann_brute_compare_include = compare_results(
          neigh_include_nflann, neigh_include_brute,
          {"nflann_tree_include", "brute_force_include"}, -1, true);

  std::ostringstream msg;
  msg << std::format("  Setup times (microseconds): \n"
                     "    nflann_tree_set_time = {}\n",
                     data.d_treeBuildTime);

  msg << std::format("  Default search times (microseconds): \n"
                     "    brute_force_search_time = {}\n"
                     "    nflann_tree_search_time = {}\n",
                     data.d_defaultBruteSearchTime,
                     data.d_defaultNFlannSearchTime);

  msg << std::format("  Exclude comparison results: \n"
                     "    nflann_brute_compare: \n{}\n",
                     nflann_brute_compare_default);

  msg << std::format("  Exclude search times (microseconds): \n"
                     "    brute_force_search_time = {}\n"
                     "    nflann_tree_search_time = {}\n",
                     data.d_excludeBruteSearchTime,
                     data.d_excludeNFlannSearchTime);

  msg << std::format("  Exclude comparison results: \n"
                     "    nflann_brute_compare: \n{}\n",
                     nflann_brute_compare_exclude);

  msg << std::format("  Include search times (microseconds): \n"
                     "    brute_force_search_time = {}\n"
                     "    nflann_tree_search_time = {}\n",
                     data.d_includeBruteSearchTime,
                     data.d_includeNFlannSearchTime);

  msg << std::format("  Include comparison results: \n"
                     "    nflann_brute_compare: \n{}\n",
                     nflann_brute_compare_include);


  msg << std::format("  Nflann all search times (microseconds): \n"
                     "    default = {}\n"
                     "    exclude = {}\n"
                     "    include = {}\n",
                     data.d_defaultNFlannSearchTime,
                     data.d_excludeNFlannSearchTime,
                     data.d_includeNFlannSearchTime);

  return msg.str();


}

std::string test::testNanoflannClosestPoint(size_t N, double L, double dL, int seed) {

  // create 3D lattice and perturb each lattice point
  size_t Nx, Ny, Nz;
  Nx = Ny = Nz = N;
  size_t N_tot = Nx * Ny * Nz;

  std::vector<util::Point> x(N_tot, util::Point());
  lattice(L, Nx, Ny, Nz, dL, seed, x, 3);
  std::cout << "Total points = " << x.size() << "\n";

  // nanoflann tree search
  auto nflann_nsearch
          = std::make_unique<nsearch::NFlannSearchKd<3>>(x, 0);

  auto nflann_tree_set_time = nflann_nsearch->setInputCloud();

  std::vector<size_t> err_points;
  std::vector<util::Point> search_points;
  std::vector<double> err_dist;
  auto nsearch_search_time = neighSearchTreeClosestPointSizet(
          x, nflann_nsearch, seed, L, dL, search_points, err_points, err_dist);

    // Compare search results
    auto nflann_compare = compare_closest_point_results(x,
                                                              search_points,
                                                              err_points,
                                                              err_dist, false);

    std::ostringstream msg;
    msg << std::format("  Setup times (microseconds): \n"
                       "    nflann_tree_set_time = {}\n",
                       nflann_tree_set_time);

    msg << std::format("  Comparison results: \n"
                       "    nflann_compare: \n{}\n",
                       nflann_compare);

    return msg.str();



}


template std::string test::testNanoflann<2>(size_t N, double L, double dL, int seed);
template std::string test::testNanoflann<3>(size_t N, double L, double dL, int seed);

template std::string test::testNanoflannExcludeInclude<2>(size_t N, double L,
        double dL, int seed, testNSearchData &data);
template std::string test::testNanoflannExcludeInclude<3>(size_t N, double L,
        double dL, int seed, testNSearchData &data);
