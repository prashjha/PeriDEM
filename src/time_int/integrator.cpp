/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "integrator.h"

#include "model/modelData.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "util/parallelUtil.h"

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

void time_int::updateCentralDifference(model::ModelData &data) {
  data.d_currentDt = data.d_modelDeck_p->d_dt;
  const auto dim = data.d_modelDeck_p->d_dim;

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1,
      [&data, dim](std::size_t II) {
        auto i = data.d_fPdCompNodes[II];

        const auto rho = data.getDensity(i);
        const auto &fix = data.d_fix[i];

        for (int dof = 0; dof < dim; dof++) {
          if (util::methods::isFree(fix, dof)) {
            data.d_v[i][dof] += (data.d_currentDt / rho) * data.d_f[i][dof];
            data.d_u[i][dof] += data.d_currentDt * data.d_v[i][dof];
            data.d_x[i][dof] += data.d_currentDt * data.d_v[i][dof];
          }
        }

        data.d_vMag[i] = data.d_v[i].length();
      }
  );

  executor.run(taskflow).get();
}

void time_int::updateVerletHalfKickAndDrift(model::ModelData &data) {
  data.d_currentDt = data.d_modelDeck_p->d_dt;
  const auto dim = data.d_modelDeck_p->d_dim;

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1,
      [&data, dim](std::size_t II) {
        auto i = data.d_fPdCompNodes[II];

        const auto rho = data.getDensity(i);
        const auto &fix = data.d_fix[i];

        for (int dof = 0; dof < dim; dof++) {
          if (util::methods::isFree(fix, dof)) {
            data.d_v[i][dof] += 0.5 * (data.d_currentDt / rho) * data.d_f[i][dof];
            data.d_u[i][dof] += data.d_currentDt * data.d_v[i][dof];
            data.d_x[i][dof] += data.d_currentDt * data.d_v[i][dof];
          }

          data.d_vMag[i] = data.d_v[i].length();
        }
      }
  );

  executor.run(taskflow).get();
}

void time_int::updateVerletSecondKick(model::ModelData &data) {
  const auto dim = data.d_modelDeck_p->d_dim;

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.d_fPdCompNodes.size(), (std::size_t) 1,
    [&data, dim](std::size_t II) {
      auto i = data.d_fPdCompNodes[II];

      const auto rho = data.getDensity(i);
      const auto &fix = data.d_fix[i];
      for (int dof = 0; dof < dim; dof++) {
        if (util::methods::isFree(fix, dof)) {
          data.d_v[i][dof] += 0.5 * (data.d_currentDt / rho) * data.d_f[i][dof];
        }

        data.d_vMag[i] = data.d_v[i].length();
      }
    }
  );

  executor.run(taskflow).get();
}
