/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "methods.h"
#include "function.h"
// #include <execution>
// #include <algorithm>
#include <taskflow/taskflow/taskflow.hpp>

static bool compare_point(const util::Point &a, const util::Point &b) {

  return util::isLess(a.length(), b.length());
}

double util::methods::add(const std::vector<double> &data) {

  // tf::Taskflow taskflow;
  // tf::Executor executor;
  // auto red = std::numeric_limits<double>::max();
  // taskflow.reduce(data.begin(), data.end(), red, [] (double& a, double& b) {  
  //     return a + b; 
  //   }
  // );
  // executor.run(taskflow).get();

  // return red;

  // return std::reduce(std::execution::par, data.begin(), data.end());
  std::cerr << "util::methods::add() function not implemented" << std::endl;
  exit(EXIT_FAILURE);
}

double util::methods::max(const std::vector<double> &data) {

  // tf::Taskflow taskflow;
  // tf::Executor executor;
  // double red = 0.;
  // taskflow.reduce(data.begin(), data.end(), red, [] (double a, double b) {  
  //     return std::max(a, b);
  //   }
  // );
  // executor.run(taskflow).get();

  // return red;
  // auto red = std::max_element(std::execution::par, data.begin(), data.end());
  // return *red;
  std::cerr << "util::methods::max() function not implemented" << std::endl;
  exit(EXIT_FAILURE);
}

double util::methods::min(const std::vector<double> &data) {

  // tf::Taskflow taskflow;
  // tf::Executor executor;
  // double red = 0.;
  // taskflow.reduce(data.begin(), data.end(), red, [] (double a, double b) {  
  //     return std::min(a, b);
  //   }
  // );
  // executor.run(taskflow).get();

  // return red;
  // auto red = std::min_element(std::execution::par, data.begin(), data.end());
  // return *red;
  std::cerr << "util::methods::min() function not implemented" << std::endl;
  exit(EXIT_FAILURE);
}

float util::methods::add(const std::vector<float> &data) {

  // tf::Taskflow taskflow;
  // tf::Executor executor;
  // float red = 0.;
  // taskflow.reduce(data.begin(), data.end(), red, [] (float a, float b) {  
  //     return a + b; 
  //   }
  // );
  // executor.run(taskflow).get();

  // return red;

  // return std::reduce(std::execution::par, data.begin(), data.end());
  std::cerr << "util::methods::add() function not implemented" << std::endl;
  exit(EXIT_FAILURE);
}

float util::methods::max(const std::vector<float> &data) {

  // tf::Taskflow taskflow;
  // tf::Executor executor;
  // float red = 0.;
  // taskflow.reduce(data.begin(), data.end(), red, [] (float a, float b) {  
  //     return std::max(a, b);
  //   }
  // );
  // executor.run(taskflow).get();

  // return red;
  // auto red = std::max_element(std::execution::par, data.begin(), data.end());
  // return *red;
  std::cerr << "util::methods::max() function not implemented" << std::endl;
  exit(EXIT_FAILURE);
}

float util::methods::min(const std::vector<float> &data) {

  // tf::Taskflow taskflow;
  // tf::Executor executor;
  // float red = 0.;
  // taskflow.reduce(data.begin(), data.end(), red, [] (float a, float b) {  
  //     return std::min(a, b);
  //   }
  // );
  // executor.run(taskflow).get();

  // return red;
  // auto red = std::min_element(std::execution::par, data.begin(), data.end());
  // return *red;
  std::cerr << "util::methods::min() function not implemented" << std::endl;
  exit(EXIT_FAILURE);
}

util::Point util::methods::maxLength(const std::vector<util::Point> &data) {


  // tf::Taskflow taskflow;
  // tf::Executor executor;
  // util::Point red = util::Point();
  // taskflow.reduce(data.begin(), data.end(), red, &compare_point);
  // executor.run(taskflow).get();

  // return red;

  // auto red = std::max_element(std::execution::par, data.begin(), data.end(), &compare_point);
  // return *red;
  std::cerr << "util::methods::maxLength() function not implemented" << std::endl;
  exit(EXIT_FAILURE);
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

float util::methods::timeDiff(std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end, std::string unit) {

  if (unit == "microseconds")
    return std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
  else if (unit == "milliseconds")
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  else if (unit == "seconds")
    return std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
  else {
    std::cerr << "Unit = " << unit << " not valid.\n";
    exit(EXIT_FAILURE);
  }
}