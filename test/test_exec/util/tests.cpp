/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <string>
#include <vector>

void removeEntry(size_t i, std::vector<std::vector<int>> &data) {

  data.erase(data.begin() + i - 1);
}

int main()
{
  std::vector<std::vector<int>> data;
  data.resize(10);
  size_t c=0;
  for (auto &d: data)
    d = std::vector<int>(4, c++);

  std::cout << "Before erase\n";
  for (const auto &d: data) {
    for (const auto &i: d)
      std::cout << i << "; ";

    std::cout << "\n";
  }

  removeEntry(4, data);

  std::cout << "After erase\n";
  for (const auto &d: data) {
    for (const auto &i: d)
      std::cout << i << "; ";

    std::cout << "\n";
  }

}
