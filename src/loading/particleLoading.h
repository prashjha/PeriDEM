/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef LOADING_PARTILCELOADING_H
#define LOADING_PARTILCELOADING_H

#include <string>
#include <vector>
#include "inp/pdecks/particleDeck.h"

/*!
 * @brief Collection of methods and database related to loading
 *
 * This namespace provides methods and data members specific to application
 * of displacement and force boundary condition and also initial condition.
 */
namespace loading {

/*!
 * @brief A base class to apply displacement and force boundary condition
 *
 * Base class which provides method and database for application of boundary
 * conditions in the form of displacement or force. Later temperature
 * boundary condition or other type of boundary condition can also be
 * implemented using this base class.
 */
class ParticleLoading {

public:
  /*! @brief Constructor */
  ParticleLoading() = default;

protected:
  /*! @brief List of displacement bcs */
  std::vector<inp::PBCData> d_bcData;
};

} // namespace loading

#endif // LOADING_LOADING_H
