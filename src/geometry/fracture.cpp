/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "fracture.h"
#include "util/geom.h"
#include "util/io.h"
#include <hpx/include/parallel_algorithm.hpp>


geometry::Fracture::Fracture() {}

geometry::Fracture::Fracture(const std::vector<util::Point> *nodes,
    const std::vector<std::vector<size_t>> *neighbor_list) {

  auto n = nodes->size();
  d_fracture.resize(n);

  auto f = hpx::parallel::for_loop(
      hpx::parallel::execution::par(hpx::parallel::execution::task), 0,
      n, [this, &nodes, &neighbor_list, n](boost::uint64_t i) {

        // get neighborlist of node i if neighborlist is provided
        std::vector<size_t> neighs;
        if (neighbor_list != nullptr)
          neighs = (*neighbor_list)[i];

        // compute number of neighbors
        auto ns = n;
        if (!neighs.empty())
          ns = neighs.size();

        size_t s = ns / 8;
        if (s * 8 < ns)
          s++;
        d_fracture[i] = std::vector<uint8_t>(s, uint8_t(0));
      }); // end of parallel for loop

  f.get();
}

void geometry::Fracture::setBondState(const size_t &i, const size_t &j,
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

const std::vector<uint8_t> &geometry::Fracture::getBonds(const size_t &i)
const {
  return d_fracture[i];
}
std::vector<uint8_t> &geometry::Fracture::getBonds(const size_t &i) {
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