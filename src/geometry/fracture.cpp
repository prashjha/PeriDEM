/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "fracture.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>


geometry::Fracture::Fracture() {}

geometry::Fracture::Fracture(const std::vector<util::Point> *nodes,
    const std::vector<std::vector<std::size_t>> *neighbor_list) {

  std::size_t n = nodes->size();
  d_fracture.resize(n);

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, n, (std::size_t) 1, [this, &nodes, &neighbor_list, n](std::size_t i) {
      // get neighborlist of node i if neighborlist is provided
      std::vector<size_t> neighs;
      if (neighbor_list != nullptr)
        neighs = (*neighbor_list)[i];

      // compute number of neighbors
      size_t ns = n;
      if (!neighs.empty())
        ns = neighs.size();

      size_t s = ns / 8;
      if (s * 8 < ns)
        s++;
      d_fracture[i] = std::vector<uint8_t>(s, uint8_t(0));
    }
  ); // for_each

  executor.run(taskflow).get();
}

void geometry::Fracture::setBondState(const std::size_t &i, const std::size_t &j,
                                      const bool &state) {

  // to set i^th bit as true of integer a,
  // a |= 1UL << (i % 8)

  // to set i^th bit as false of integer a,
  // a &= ~(1UL << (i % 8))

  state ? (d_fracture[i][j / 8] |= 1UL << (j % 8))
        : (d_fracture[i][j / 8] &= ~(1UL << (j % 8)));
}

bool geometry::Fracture::getBondState(const size_t &i, const size_t &j) const {

  auto bond = d_fracture[i][j / 8];
  return bond >> (j % 8) & 1UL;
}

const std::vector<uint8_t> &geometry::Fracture::getBonds(const std::size_t &i)
const {
  return d_fracture[i];
}
std::vector<uint8_t> &geometry::Fracture::getBonds(const std::size_t &i) {
  return d_fracture[i];
}

std::string geometry::Fracture::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "------- Fracture --------" << std::endl << std::endl;
  oss << tabS << "Num of outer fracture data = " << d_fracture.size() <<
  std::endl;
  oss << tabS << std::endl;

  return oss.str();
}
