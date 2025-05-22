/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "particleIC.h"
#include "particleLoadingUtil.h"
#include "particle/baseParticle.h"
#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

void loading::applyIC(particle::BaseParticle *particle, const std::vector<inp::BCBaseDeck> &icVec) {
  for (size_t s=0; s<icVec.size(); s++) {

    // get alias for bc data
    const auto &bc = icVec[s];

    if (bc.d_icType != "Constant_Velocity")
      continue;

    // check if we need to process this particle
    if (!needToProcessParticle(particle->getId(), bc))
      continue;

    // get bounding box (quite possibly be generic)
    auto reg_box = bc.d_regionGeomData.d_geom_p->box();

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
        (std::size_t) 0, particle->getNumNodes(), (std::size_t) 1,
        [&particle, bc, reg_box] (std::size_t i) {

          // currently, IC is implemented using particle list only
          // apply velocity condition
          particle->setVLocal(i, bc.d_icVec);
        }
    ); // for_each

    executor.run(taskflow).get();
  }
}