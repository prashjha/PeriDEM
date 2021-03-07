/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "methods.h"
#include "function.h"
#include <hpx/include/parallel_algorithm.hpp>
#include <hpx/include/parallel_minmax.hpp>
#include <hpx/include/parallel_reduce.hpp>

static bool compare_point(const util::Point &a, const util::Point &b) {

  return util::isLess(a.length(), b.length());
}

double util::methods::add(const std::vector<double> &data) {

  return hpx::parallel::reduce(hpx::parallel::execution::par, data.begin(),
                               data.end());
}

double util::methods::max(const std::vector<double> &data, size_t *i) {

  auto max_i = hpx::parallel::max_element(hpx::parallel::execution::par,
                                          data.begin(), data.end());

  if (i != nullptr)
    *i = std::distance(data.begin(), max_i);
  return data[std::distance(data.begin(), max_i)];
}

double util::methods::min(const std::vector<double> &data, size_t *i) {

  auto min_i = hpx::parallel::min_element(hpx::parallel::execution::par,
                                          data.begin(), data.end());

  if (i != nullptr)
    *i = std::distance(data.begin(), min_i);
  return data[std::distance(data.begin(), min_i)];
}

float util::methods::add(const std::vector<float> &data) {

  return hpx::parallel::reduce(hpx::parallel::execution::par, data.begin(),
                               data.end());
}

float util::methods::max(const std::vector<float> &data, size_t *i) {

  auto max_i = hpx::parallel::max_element(hpx::parallel::execution::par,
                                          data.begin(), data.end());

  if (i != nullptr)
    *i = std::distance(data.begin(), max_i);
  return data[std::distance(data.begin(), max_i)];
}

float util::methods::min(const std::vector<float> &data, size_t *i) {

  auto min_i = hpx::parallel::min_element(hpx::parallel::execution::par,
                                          data.begin(), data.end());

  if (i != nullptr)
    *i = std::distance(data.begin(), min_i);
  return data[std::distance(data.begin(), min_i)];
}

util::Point util::methods::maxLength(const std::vector<util::Point> &data) {

  auto max_i = hpx::parallel::max_element(
      hpx::parallel::execution::par, data.begin(), data.end(), &compare_point);

  return data[std::distance(data.begin(), max_i)];
}

bool util::methods::isFree(const int &i, const unsigned int &dof) {
  return !(i >> dof & 1UL);
}

bool util::methods::isFree(const uint8_t &i, const unsigned int &dof) {
  return !(i >> dof & 1UL);
}

bool util::methods::isTagInList(const std::string &tag,
                                const std::vector<std::string> &tags) {
  for (const auto &s : tags)
    if (s == tag)
      return true;

  return false;
}

float util::methods::timeDiff(std::chrono::steady_clock::time_point begin,
                std::chrono::steady_clock::time_point end) {

  return std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                               begin).count();
}