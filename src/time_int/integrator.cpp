/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "integrator.h"

#include "data/modelData.h"
#include "util/parallelUtil.h"

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

void time_int::applyRigidBodyConstraint(data::ModelData &data) {
  const auto &rigid = data.d_modelDeck_p->d_rigidParticles;
  if (rigid.empty())
    return;

  // Nodal force in the integrators is a force density (acceleration = f/rho).
  // A rigid translating body of mass M has acceleration a = sum(f_i V_i) / M,
  // so replacing each nodal force density by rho * a gives that motion
  // without touching the integrators themselves.
  for (const auto &[pid, mass] : rigid) {
    util::Point net(0., 0., 0.);
    for (std::size_t II = 0; II < data.numPdForceNodes(); II++) {
      const auto i = data.pdForceNode(II);
      if (data.getPtId(i) != pid)
        continue;
      net += data.getVol(i) * data.getF(i);
    }

    const auto acc = net / mass;
    for (std::size_t II = 0; II < data.numPdForceNodes(); II++) {
      const auto i = data.pdForceNode(II);
      if (data.getPtId(i) != pid)
        continue;
      data.getF(i) = data.getDensity(i) * acc;
    }
  }
}

void time_int::updateCentralDifference(data::ModelData &data) {
  const auto dim = data.dimension();
  const auto dt = data.currentDt();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.numPdForceNodes(), (std::size_t) 1,
      [&data, dim, dt](std::size_t II) {
        const auto i = data.pdForceNode(II);
        const auto rho = data.getDensity(i);
        auto &v = data.getV(i);
        auto &u = data.getU(i);
        auto &x = data.getX(i);
        const auto &f = data.getF(i);

        for (int dof = 0; dof < dim; dof++) {
          if (data.isDofFree(i, dof)) {
            v[dof] += (dt / rho) * f[dof];
            u[dof] += dt * v[dof];
            x[dof] += dt * v[dof];
          }
        }

        data.setVMag(i, v.length());
      }
  );

  executor.run(taskflow).get();
}

void time_int::updateVerletHalfKickAndDrift(data::ModelData &data) {
  const auto dim = data.dimension();
  const auto dt = data.currentDt();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.numPdForceNodes(), (std::size_t) 1,
      [&data, dim, dt](std::size_t II) {
        const auto i = data.pdForceNode(II);
        const auto rho = data.getDensity(i);
        auto &v = data.getV(i);
        auto &u = data.getU(i);
        auto &x = data.getX(i);
        const auto &f = data.getF(i);

        for (int dof = 0; dof < dim; dof++) {
          if (data.isDofFree(i, dof)) {
            v[dof] += 0.5 * (dt / rho) * f[dof];
            u[dof] += dt * v[dof];
            x[dof] += dt * v[dof];
          }
        }

        data.setVMag(i, v.length());
      }
  );

  executor.run(taskflow).get();
}

void time_int::updateVerletSecondKick(data::ModelData &data) {
  const auto dim = data.dimension();
  const auto dt = data.currentDt();

  tf::Executor executor(util::parallel::getNThreads());
  tf::Taskflow taskflow;

  taskflow.for_each_index(
    (std::size_t) 0, data.numPdForceNodes(), (std::size_t) 1,
    [&data, dim, dt](std::size_t II) {
      const auto i = data.pdForceNode(II);
      const auto rho = data.getDensity(i);
      auto &v = data.getV(i);
      const auto &f = data.getF(i);

      for (int dof = 0; dof < dim; dof++) {
        if (data.isDofFree(i, dof)) {
          v[dof] += 0.5 * (dt / rho) * f[dof];
        }
      }

      data.setVMag(i, v.length());
    }
  );

  executor.run(taskflow).get();
}
