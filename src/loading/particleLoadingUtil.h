/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef LOADING_PARTILCELOADINGUTIL_H
#define LOADING_PARTILCELOADINGUTIL_H

#include <string>
#include <vector>
#include "util/point.h"
#include "inp/bcBaseDeck.h"

namespace loading {
  /*!
   * @brief Function that checks if given particle with id = id needs to be processed
   * within boundary condition data bc
   * @param id Id of particle in all particle list
   * @param bc Boundary condition data
   * @return bool True if particle should be processed further
   */
  bool needToProcessParticle(size_t id,
                             const inp::BCBaseDeck &bc);

  /*!
   * @brief Function that checks if we need to do computation at a given point x within a particle with id = id
   * @param x Coordinates of a point within particle (reference coordinate)
   * @param id Id of particle in all particle list
   * @param bc Boundary condition data
   * @return bool True if we compute at x
   */
  bool needToComputeDof(const util::Point &x,
                        size_t id,
                        const inp::BCBaseDeck &bc);

} // namespace loading

#endif // LOADING_PARTILCELOADINGUTIL_H
