/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "particleFLoading.h"
#include "particleLoadingUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/parallelUtil.h"
#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

namespace {

bool isInList(const size_t &i, const std::vector<size_t> &list) {
  for (const auto &l : list)
    if (l == i)
      return true;

  return false;
}

} // namespace

loading::ParticleFLoading::ParticleFLoading(
    std::vector<inp::BCBaseDeck> &bc_data) {

  d_bcData = bc_data;
}

void loading::ParticleFLoading::apply(const double &time,
                                      particle::BaseParticle *particle) {

  for (size_t s = 0; s < d_bcData.size(); s++) {

    // get alias for bc data
    const auto &bc = d_bcData[s];

    // check if we need to process this particle
    if (!needToProcessParticle(particle->getId(), bc))
      continue;

    // get bounding box (quite possibly be generic)
    auto reg_box = bc.d_regionGeomData.d_geom_p == nullptr ? std::pair<util::Point, util::Point>(util::Point(), util::Point()) : bc.d_regionGeomData.d_geom_p->box();

    // for (size_t i = 0; i < particle->getNumNodes(); i++) {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
            (std::size_t) 0, particle->getNumNodes(), (std::size_t) 1,
            [time, &particle, bc, reg_box, this](std::size_t i) {

                const auto x = particle->getXRefLocal(i);
                double fmax = 1.0;

                auto box = reg_box;
                if (!bc.d_isRegionActive) {
                  // get box from particle
                  box = particle->d_geom_p->box();
                }

                if (needToComputeDof(x, particle->getId(), bc)) {

                  // apply spatial function
                  if (bc.d_spatialFnType == "hat_x") {
                    fmax = bc.d_spatialFnParams[0] *
                           util::hatFunction(x.d_x, box.first.d_x,
                                             box.second.d_x);
                  } else if (bc.d_spatialFnType == "hat_y") {
                    fmax = bc.d_spatialFnParams[0] *
                           util::hatFunction(x.d_y, box.first.d_y,
                                             box.second.d_y);
                  } else if (bc.d_spatialFnType == "sin_x") {
                    double a = M_PI * bc.d_spatialFnParams[0];
                    fmax = bc.d_spatialFnParams[0] * std::sin(a * x.d_x);
                  } else if (bc.d_spatialFnType == "sin_y") {
                    double a = M_PI * bc.d_spatialFnParams[0];
                    fmax = bc.d_spatialFnParams[0] * std::sin(a * x.d_y);
                  } else if (bc.d_spatialFnType == "linear_x") {
                    double a = bc.d_spatialFnParams[0];
                    fmax = bc.d_spatialFnParams[0] * a * x.d_x;
                  } else if (bc.d_spatialFnType == "linear_y") {
                    double a = bc.d_spatialFnParams[0];
                    fmax = bc.d_spatialFnParams[0] * a * x.d_y;
                  }

                  // apply time function
                  if (bc.d_timeFnType == "linear")
                    fmax *= time;
                  else if (bc.d_timeFnType == "linear_step")
                    fmax *= util::linearStepFunc(time, bc.d_timeFnParams[1],
                                                 bc.d_timeFnParams[2]);
                  else if (bc.d_timeFnType == "linear_slow_fast") {
                    if (util::isGreater(time, bc.d_timeFnParams[1]))
                      fmax *= bc.d_timeFnParams[3] * time;
                    else
                      fmax *= bc.d_timeFnParams[2] * time;
                  } else if (bc.d_timeFnType == "sin") {
                    double a = M_PI * bc.d_timeFnParams[1];
                    fmax *= std::sin(a * time);
                  }

                  // multiply by the slope
                  fmax *= bc.d_timeFnParams[0];

                  auto force_i = util::Point();
                  for (auto d : bc.d_direction) {
                    force_i[d-1] = fmax;
                  }

                  // add force
                  particle->addFLocal(i, force_i);
                } // if compute force
            }
    ); // for_each

    executor.run(taskflow).get();
  } // loop over bc sets
}