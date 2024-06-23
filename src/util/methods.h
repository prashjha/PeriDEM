/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#pragma once

#include "point.h"           // definition of Point
#include <cstdint> // uint8_t type
#include <cstring> // string and size_t type
#include <vector>
#include <chrono>
#include <numeric>      // std::iota
#include <algorithm>    // std::sort, std::stable_sort

using namespace std::chrono;

namespace util {

/*!
 * @brief Provides fast methods to add/subtract list of data, to find
 * maximum/minimum from list of data
 */
namespace methods {

/*!
 * @brief Returns the index corresponding to maximum from list of data
 * @param data List of real numbers
 * @return i Index with a maximum value
 */
template <typename T>
inline size_t maxIndex(const std::vector<T> &data) {

      // initialize original index locations
      std::vector<size_t> idx(data.size());
      std::iota(idx.begin(), idx.end(), 0);

      std::stable_sort(idx.begin(), idx.end(),
                       [&data](size_t i1, size_t i2) {return data[i1] > data[i2];});

      return idx[0];
};

/*!
 * @brief Returns the index corresponding to minimum from list of data
 * @param data List of real numbers
 * @return i Index with a minimum value
 */
template <typename T>
inline size_t minIndex(const std::vector<T> &data) {

      // initialize original index locations
      std::vector<size_t> idx(data.size());
      std::iota(idx.begin(), idx.end(), 0);

      std::stable_sort(idx.begin(), idx.end(),
                       [&data](size_t i1, size_t i2) {return data[i1] < data[i2];});

      return idx[0];
};

/*!
 * @brief Returns the maximum from list of data
 * @param data List of real numbers
 * @return value Maximum value
 */
template <typename T>
inline T max(const std::vector<T> &data) {
      return data[util::methods::maxIndex(data)];
};

/*!
 * @brief Returns the minimim from list of data
 * @param data List of real numbers
 * @return value Minimum value
 */
template <typename T>
inline T min(const std::vector<T> &data) {
      return data[util::methods::minIndex(data)];
};

/*!
 * @brief Returns the maximum and index of maximum from list of data
 * @param data List of real numbers
 * @return pair Maximum value and index
 */
template <typename T>
inline std::pair<size_t, T> maxAndMaxIndex(const std::vector<T> &data) {
      auto i = util::methods::maxIndex(data);
      return {data[i], i};
};

/*!
 * @brief Returns the minimum and index of minimum from list of data
 * @param data List of real numbers
 * @return pair Minimum value and index
 */
template <typename T>
inline std::pair<size_t, T> minAndMinIndex(const std::vector<T> &data) {
      auto i = util::methods::minIndex(data);
      return {data[i], i};
};


/*!
 * @brief Returns the index corresponding to maximum from list of data
 * @param data List of real numbers
 * @return i Index with a maximum value
 */
template <typename T>
inline size_t maxIndex(const std::vector<T> &data,
                       size_t data_start, size_t data_end) {

  if (data.size() == 0) {
    std::cerr << "Error: maxIndex() is called with data of size " << data.size()
              << ".\n";
    exit(EXIT_FAILURE);
  }

  if (data_end == 0 or data_end > data.size()) {
    std::cerr << "Error: maxIndex() data_end = " << data_end
              << " is not valid for the data of size " << data.size()
              << ".\n";
    exit(EXIT_FAILURE);
  }

  if (data_start > data.size() - 1) {
    std::cerr << "Error: maxIndex() data_start = " << data_start
              << " is not valid for the data of size " << data.size()
              << ".\n";
    exit(EXIT_FAILURE);
  }

  // initialize original index locations
  std::vector<size_t> idx(data_end - data_start);
  std::iota(idx.begin(), idx.end() , 0);

  std::stable_sort(idx.begin(), idx.end(),
                   [&data, &data_start](size_t i1, size_t i2)
                   {return data[i1 + data_start] > data[i2 + data_start];});

  return idx[0] + data_start;
};

/*!
 * @brief Returns the index that has maximum length of point from list of points
 * @param data List of points
 * @return i Index with maximum length of point
 */
inline size_t maxLengthIndex(const std::vector<util::Point> &data) {
      std::vector<double> length_data(data.size());
      for (size_t i = 0; i < data.size(); i++)
        length_data[i] = data[i].length();

      return util::methods::maxIndex(length_data);
};

/*!
 * @brief Returns the index that has minimum length of point from list of points
 * @param data List of points
 * @return i Index with minimum length of point
 */
inline size_t minLengthIndex(const std::vector<util::Point> &data) {
  std::vector<double> length_data(data.size());
  for (size_t i = 0; i < data.size(); i++)
    length_data[i] = data[i].length();

  return util::methods::minIndex(length_data);
};

/*!
 * @brief Returns the maximum length of point from list of points
 * @param data List of points
 * @return value Maximum length of point
 */
inline double maxLength(const std::vector<util::Point> &data) {
      std::vector<double> length_data(data.size());
      for (size_t i = 0; i < data.size(); i++)
        length_data[i] = data[i].length();

      return length_data[util::methods::maxIndex(length_data)];
};

/*!
 * @brief Returns the minimum length of point from list of points
 * @param data List of points
 * @return value Minimum length of point
 */
inline double minLength(const std::vector<util::Point> &data) {
      std::vector<double> length_data(data.size());
      for (size_t i = 0; i < data.size(); i++)
        length_data[i] = data[i].length();

      return length_data[util::methods::minIndex(length_data)];
};

/*!
 * @brief Returns the maximum length of point and index from list of points
 * @param data List of points
 * @return pair Maximum length of point and index
 */
inline std::pair<double, size_t> maxLengthAndMaxLengthIndex(const std::vector<util::Point> &data) {
      std::vector<double> length_data(data.size());
      for (size_t i = 0; i < data.size(); i++)
        length_data[i] = data[i].length();

      auto i = util::methods::maxIndex(length_data);
      return {length_data[i], i};
};

/*!
 * @brief Returns the minimum length of point and index from list of points
 * @param data List of points
 * @return pair Minimum length of point and index
 */
inline std::pair<double, size_t> minLengthAndMinLengthIndex(const std::vector<util::Point> &data) {
      std::vector<double> length_data(data.size());
      for (size_t i = 0; i < data.size(); i++)
        length_data[i] = data[i].length();

      auto i = util::methods::minIndex(length_data);
      return {length_data[i], i};
};


/*!
 * @brief Returns the sum of data
 * @param data List of real numbers
 * @return value Sum of the numbers
 */
template <typename T>
inline T add(const std::vector<T> &data) {
  return std::reduce(data.begin(), data.end());
};


/*!
 * @brief Returns true if degree of freedom is free
 * @param i Fixity value
 * @param dof Degree of freedom to probe (dof = 0 for x, dof = 1 for y, and dof = 2 for z)
 * @return True True if dof is free
 */
inline bool isFree(const int &i, const unsigned int &dof) {
      return !(i >> dof & 1UL);
};

/*! @copydoc isFree(const int &i, const unsigned int &dof) */
inline bool isFree(const uint8_t &i, const unsigned int &dof) {
      return !(i >> dof & 1UL);
};

/*!
 * @brief Find if data is in the list
 * @param tag Tag to search
 * @param tags List of tags
 * @return True True if tag exists
 */
template <typename T>
inline bool isInList(const T &i, const std::vector<T> &list) {
      for (const auto &j : list)
        if (j == i)
          return true;

      return false;
};

/*!
 * @brief Returns true if tag is found in the list of tags
 * @param tag Tag to search
 * @param tags List of tags
 * @return True True if tag exists
 */
inline bool isTagInList(const std::string &tag, const std::vector<std::string> &tags) {
  return isInList(tag, tags);
};

/*!
 * @brief Returns difference between two times
 * @param begin Beginning time
 * @param end Ending time
 * @param unit Unit in which time difference is to be returned
 * @return time Time difference
 */
inline float timeDiff(std::chrono::steady_clock::time_point begin,
                std::chrono::steady_clock::time_point end, std::string unit = "microseconds") {
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
};

} // namespace methods

} // namespace util
