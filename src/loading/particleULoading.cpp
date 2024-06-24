/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "particleULoading.h"
#include "fe/mesh.h"
#include "inp/pdecks/particleDeck.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/geom.h"
#include "util/transformation.h"
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

loading::ParticleULoading::ParticleULoading(
    std::vector<inp::PBCData> &bc_data) {

  d_bcData = bc_data;

  d_pZeroDisplacementApplied = std::vector<bool>(d_bcData.size(), false);
}

bool loading::ParticleULoading::needToProcessParticle(size_t id, const inp::PBCData &bc) {
  // if there is a list, and if particle is not in the list, skip
  bool skip_condition1 = (bc.d_selectionType == "particle"
                          || bc.d_selectionType == "region_with_include_list")
                         && !isInList(id, bc.d_pList);
  // if there is an exclusion list, and if particle is in the list, skip
  bool skip_condition2 = (bc.d_selectionType == "region_with_exclude_list")
                         && isInList(id, bc.d_pNotList);
  // if there is a inclusion and an exclusion list,
  // and if particle is either in the exclusion list or not in the inclusion list, skip
  bool skip_condition3 = (bc.d_selectionType == "region_with_include_list_with_exclude_list")
                         && (isInList(id, bc.d_pNotList) ||
                             !isInList(id, bc.d_pList));

  bool skip = skip_condition1 or skip_condition2 or skip_condition3;
  return !skip;
}

bool loading::ParticleULoading::needToComputeDof(const util::Point &x,
                                                 size_t id,
                                                 const inp::PBCData &bc) {
  if (bc.d_selectionType == "region" && bc.d_regionGeom_p->isInside(x))
    return true;
  else if (bc.d_selectionType == "region_with_include_list" &&
           bc.d_regionGeom_p->isInside(x) &&
           isInList(id, bc.d_pList))
    return true;
  else if (bc.d_selectionType == "region_with_exclude_list" &&
           bc.d_regionGeom_p->isInside(x) &&
           !isInList(id, bc.d_pNotList))
    return true;
  else if (bc.d_selectionType == "region_with_include_list_with_exclude_list" &&
           bc.d_regionGeom_p->isInside(x) &&
           isInList(id, bc.d_pList) &&
           !isInList(id, bc.d_pNotList))
    return true;
  else if (bc.d_selectionType == "particle" &&
           isInList(id, bc.d_pList))
    return true;

  return false;
}

void loading::ParticleULoading::setFixity(particle::BaseParticle *particle) {

  for (size_t s = 0; s < d_bcData.size(); s++) {

    // get alias for bc data
    const auto &bc = d_bcData[s];

    // check if we need to process this particle
    if (!needToProcessParticle(particle->getId(), bc))
      continue;

    // get bounding box
    auto box = bc.d_regionGeom_p->box();

    for (size_t i = 0; i < particle->getNumNodes(); i++) {

      const auto x = particle->getXRefLocal(i);

      if (!needToComputeDof(x, particle->getId(), bc))
        continue;

      // set fixity to true
      for (auto d : bc.d_direction)
        particle->setFixLocal(i, d - 1, true);
    } // loop over nodes
  }   // loop over bc sets
}

void loading::ParticleULoading::apply(const double &time,
                                      particle::BaseParticle *particle) {

  for (size_t s = 0; s < d_bcData.size(); s++) {

    // get alias for bc data
    const auto &bc = d_bcData[s];

    // if this is zero displacement condition, and if we have already applied
    // displacement then do not need to reapply this
    if (bc.d_isDisplacementZero && d_pZeroDisplacementApplied[s])
      continue;

    // set to true so that next time this is not called
    if (bc.d_isDisplacementZero)
      d_pZeroDisplacementApplied[s] = true;

    // check if we need to process this particle
    if (!needToProcessParticle(particle->getId(), bc))
      continue;

    // get bounding box
    auto box = bc.d_regionGeom_p->box();

    // for (size_t i = 0; i < particle->getNumNodes(); i++) {
    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
            (std::size_t) 0, particle->getNumNodes(), (std::size_t) 1,
            [time, &particle, bc, box, this] (std::size_t i) {

                const auto x = particle->getXRefLocal(i);

                double umax = bc.d_timeFnParams[0];
                double du = 0.;
                double dv = 0.;

                if (needToComputeDof(x, particle->getId(), bc)) {

                  // apply spatial function
                  if (bc.d_spatialFnType == "sin_x") {
                    double a = M_PI * bc.d_spatialFnParams[0];
                    umax = umax * std::sin(a * x.d_x);
                  } else if (bc.d_spatialFnType == "sin_y") {
                    double a = M_PI * bc.d_spatialFnParams[0];
                    umax = umax * std::sin(a * x.d_y);
                  } else if (bc.d_spatialFnType == "linear_x") {
                    double a = bc.d_spatialFnParams[0];
                    umax = umax * a * x.d_x;
                  } else if (bc.d_spatialFnType == "linear_y") {
                    double a = bc.d_spatialFnParams[0];
                    umax = umax * a * x.d_y;
                  }

                  // apply time function
                  if (bc.d_timeFnType == "constant")
                    du = umax;
                  else if (bc.d_timeFnType == "linear") {
                    du = umax * time;
                    dv = umax;
                  } else if (bc.d_timeFnType == "quadratic") {
                    du = umax * time + bc.d_timeFnParams[1] * time * time;
                    dv = umax + bc.d_timeFnParams[1] * time;
                  } else if (bc.d_timeFnType == "sin") {
                    double a = M_PI * bc.d_timeFnParams[1];
                    du = umax * std::sin(a * time);
                    dv = umax * a * std::cos(a * time);
                  }

                  auto u_i = util::Point();
                  auto v_i = util::Point();
                  for (auto d : bc.d_direction) {
                    u_i[d-1] = du;
                    v_i[d-1] = dv;
                  }

                  if (bc.d_timeFnType == "rotation") {
                    auto x0 = util::Point(bc.d_timeFnParams[1], bc.d_timeFnParams[2],
                                          bc.d_timeFnParams[3]);
                    auto dx = x - x0;
                    auto r_x = util::rotate2D(
                            dx, bc.d_timeFnParams[0] * time);
                    auto dr_x = util::derRotate2D(
                            dx, bc.d_timeFnParams[0] * time);

                    u_i += r_x - dx;
                    v_i += bc.d_timeFnParams[0] * dr_x;
                  }

                  for (auto d : bc.d_direction) {
                    particle->setULocal(i, d-1, u_i[d-1]);
                    particle->setVLocal(i, d-1, v_i[d-1]);
                    auto xref = particle->getXRefLocal(i)[d-1];
                    particle->setXLocal(i, d-1, u_i[d-1] + xref);
                  }
                } // if compute displacement
            }
    ); // for_each

    executor.run(taskflow).get();
  } // loop over bc sets
}