/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef UTIL_METHODS_H
#define UTIL_METHODS_H

#include "point.h"           // definition of Point
#include <vector>
#include <chrono>
using namespace std::chrono;

namespace util {

/*!
 * @brief Provides fast methods to add/subtract list of data, to find
 * maximum/minimum from list of data
 */
namespace methods {

/*!
 * @brief Returns the sum of data
 * @param data List of real numbers
 * @return sum Sum of the numbers
 */
double add(const std::vector<double> &data);

/*!
 * @brief Returns the maximum from list of data
 * @param data List of real numbers
 * @param i Pointer to store the id where maximum occurs
 * @return max Maximum value
 */
double max(const std::vector<double> &data, size_t *i = nullptr);

/*!
 * @brief Returns the minimum from list of data
 * @param data List of real numbers
 * @param i Pointer to store the id where minimum occurs
 * @return min Minimum value
 */
double min(const std::vector<double> &data, size_t *i = nullptr);

/*!
 * @brief Returns the sum of data
 * @param data List of real numbers
 * @return sum Sum of the numbers
 */
float add(const std::vector<float> &data);

/*!
 * @brief Returns the maximum from list of data
 * @param data List of real numbers
 * @param i Pointer to store the id where maximum occurs
 * @return max Maximum value
 */
float max(const std::vector<float> &data, size_t *i = nullptr);

/*!
 * @brief Returns the minimum from list of data
 * @param data List of real numbers
 * @param i Pointer to store the id where minimum occurs
 * @return min Minimum value
 */
float min(const std::vector<float> &data, size_t *i = nullptr);

/*!
 * @brief Returns the maximum length of point from list of points
 * @param data List of points
 * @return max Maximum length of point
 */
util::Point maxLength(const std::vector<util::Point> &data);

/*!
 * @brief Returns true if degree of freedom is free
 * @param i Fixity value
 * @param dof Degree of freedom to probe
 * @return True True if dof is free
 */
bool isFree(const int &i, const unsigned int &dof);
bool isFree(const uint8_t &i, const unsigned int &dof);

/*!
 * @brief Returns true if tag is found in the list of tags
 * @param tag Tag to search
 * @param tags List of tags
 * @return True True if tag exists
 */
bool isTagInList(const std::string &tag, const std::vector<std::string> &tags);

/*!
 * @brief Returns difference between two times
 * @param begin Beginning time
 * @param end Ending time
 * @return time Time difference
 */
float timeDiff(std::chrono::steady_clock::time_point begin,
                std::chrono::steady_clock::time_point end);

} // namespace methods

} // namespace util

#endif // UTIL_FAST_METHODS_H
