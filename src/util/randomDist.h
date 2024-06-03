/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef UTILS_RANDOM_H
#define UTILS_RANDOM_H

#include <random>

typedef std::mt19937 RandGenerator;
typedef std::lognormal_distribution<> LogNormalDistribution;
typedef std::uniform_real_distribution<> UniformDistribution;
typedef std::normal_distribution<> NormalDistribution;

namespace util {

/*!
 * @brief Return random number generator
 *
 * @param seed Seed
 * @return Random number generator
 */
inline RandGenerator get_rd_gen(int seed = -1) {

  //return RandGenerator();

  if (seed < 0) {
    std::random_device rd;
    seed = rd();
  }

  return RandGenerator(seed);
}

/*!
 * @brief Return random number generator
 *
 * @param seed Seed
 * @return Random number generator
 */
inline std::default_random_engine get_rd_engine(int &seed) {

  //return std::default_random_engine();

  if (seed < 0) {
    std::random_device rd;
    seed = rd();
  }

  return std::default_random_engine(seed);
}

/*!
 * @brief Transform sample from N(0,1) to N(mean, std^2)
 *
 * @param mean Mean of normal distribution
 * @param std Std of normal distribution
 * @param sample Sample from N(0,1)
 * @return sample Transformed sample
 */
inline double transform_to_normal_dist(double mean, double std, double sample) {
  return std * sample + mean;
}

/*!
 * @brief Transform sample from U(0,1) to U(a,b)
 *
 * @param min Min of uniform distribution
 * @param max Max of uniform distribution
 * @param sample Sample from U(0,1)
 * @return sample Transformed sample
 */
inline double transform_to_uniform_dist(double min, double max, double sample) {
  return min + sample * (max - min);
}

/*!
 * @brief Templated probability distribution
 *
 * Template could be log-normal, normal, or uniform distribution
 */
template<class T>
class DistributionSample {
public:
  /*!
   * @brief Constructor
   *
   * @param arg1 Argument 1 (in case of normal distribution this is mean)
   * @param arg2 Argument 2 (in case of normal distribution this is std)
   * @param seed Seed
   */
  DistributionSample(double arg1, double arg2, int seed = -1)
      : d_seed(seed), d_gen(get_rd_gen(seed)), d_dist(arg1, arg2){};

  /*!
   * @brief Constructor
   */
  DistributionSample() : d_seed(-1){};

  /*!
   * @brief Initialize the distribution
   *
   * @param arg1 Argument 1 (in case of normal distribution this is mean)
   * @param arg2 Argument 2 (in case of normal distribution this is std)
   * @param seed Seed
   */
  void init(double arg1, double arg2, int seed = -1) {

    d_seed = seed;
    d_gen = RandGenerator(get_rd_gen(seed));
    d_dist = T(arg1, arg2);
  }

  /*!
   * @brief Sample from the distribution
   * @return Sample Sample from the distribution
   */
  double operator()() {
    return d_dist(d_gen);
  }

  /*! @brief Seed */
  int d_seed;

  /*! @brief Random number generator */
  RandGenerator d_gen;

  /*! @brief Templated distribution */
  T d_dist;
};

} // namespace util

#endif // UTILS_RANDOM_H
