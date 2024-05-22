/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "testParallelCompLib.h"
#include "util/methods.h"
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

typedef std::mt19937 RandGenerator;
typedef std::uniform_real_distribution<> UniformDistribution;

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

namespace {

  RandGenerator get_rd_gen(int seed) {

    // return RandGenerator();

    if (seed < 0) {
      std::random_device rd;
      seed = rd();
    }

    return RandGenerator(seed);
  }

  double f1(const double &x){
    return x*x*x + std::exp(x) - std::sin(x);
  }

  double f2(const double &x){
    return 2*(x-0.5)*(x-0.5)*(x-0.5) + std::exp(x-0.5) - std::cos(x-0.5);
  }
}


std::string test::testTaskflow(size_t N, int seed) {

  // task: perform N computations in serial and using taskflow for_each

  // generate vector of random numbers
  RandGenerator gen(get_rd_gen(seed));
  UniformDistribution dist(0, 1);

  std::vector<double> x(N);
  std::vector<double> y1(N);
  std::vector<double> y2(N);
  std::generate(std::begin(x), std::end(x), [&] { return dist(gen); });

  // now do serial calculation
  auto t1 = steady_clock::now();
  for (size_t i = 0; i < N; i++) {
    if (x[i] < 0.5)
      y1[i] = f1(x[i]);
    else
      y1[i] = f2(x[i]);
  }
  auto t2 = steady_clock::now();
  auto dt12 = util::methods::timeDiff(t1, t2, "microseconds");

  // now do parallel calculation using taskflow
  tf::Executor executor;
  tf::Taskflow taskflow;

  taskflow.for_each_index((std::size_t) 0, N, (std::size_t) 1, [&x, &y2](std::size_t i) {
      if (x[i] < 0.5)
        y2[i] = f1(x[i]);
      else
        y2[i] = f2(x[i]);
                          }
  ); // for_each

  executor.run(taskflow).get();
  auto t3 = steady_clock::now();
  auto dt23 = util::methods::timeDiff(t2, t3, "microseconds");

  // compare results
  double y_err = 0.;
  for (size_t i=0; i<N; i++)
    y_err += std::pow(y1[i] - y2[i], 2);

  if (y_err > 1.e-10) {
    std::cerr << fmt::format("Error: Serial and taskflow computation results do not match (squared error = {})\n",
                            y_err);
    exit(1);
  }

  // get time
  std::ostringstream msg;
  msg << fmt::format("  Serial computation took = {}ms\n", dt12);
  msg << fmt::format("  Taskflow computation took = {}ms\n", dt23);
  msg << fmt::format("  Speed-up factor = {}\n\n\n", dt12/dt23);

  return msg.str();
}
