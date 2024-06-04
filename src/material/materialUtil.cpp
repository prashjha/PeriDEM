/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "materialUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/parallelUtil.h"
#include <iostream>
#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

namespace {

double computeStateMxI(size_t i, const std::vector<util::Point> &nodes,
                       const std::vector<double> &nodal_vol,
                       const std::vector<std::vector<size_t>> &neighbors,
                       const std::vector<std::vector<float>> &neighbors_sq_dist,
                       const double &mesh_size,
                       const material::Material *material) {

  double horizon = material->getHorizon();
  const auto &xi = nodes[i];
  double m = 0.;

  // upper and lower bound for volume correction
  auto check_up = horizon + 0.5 * mesh_size;
  auto check_low = horizon - 0.5 * mesh_size;

  size_t k = 0;
  for (size_t j : neighbors[i]) {

    const auto &xj = nodes[j];
    double rji = (xj - xi).length();
    //double rji = std::sqrt(neighbors_sq_dist[i][k]);

    if (util::isGreater(rji, horizon) or j == i)
      continue;

    // get corrected volume of node j
    auto volj = nodal_vol[j];

    if (util::isGreater(rji, check_low))
      volj *= (check_up - rji) / mesh_size;

    m += std::pow(rji, 2) * material->getInfFn(rji) * volj;

    k++;
  }

  if (util::isLess(m, 1.0E-18)) {
    std::ostringstream oss;
    oss << "Error: Weighted nodal volume = " << m
              << " should not be too close to zero.\n";

    oss << "Mesh size = " << mesh_size << "\n";
    oss << "J = " << material->getInfFn(0.5 * horizon) << "\n";
    oss << material->printStr(0, 0);

    std::cout << oss.str();

    exit(1);
  }
  return m;
}

double computeStateThetaxI(size_t i, const std::vector<util::Point> &nodes,
                           const std::vector<util::Point> &nodes_disp,
                           const std::vector<double> &nodal_vol,
                           const std::vector<std::vector<size_t>> &neighbors,
                           const std::vector<std::vector<float>> &neighbors_sq_dist,
                           const double &mesh_size,
                           const material::Material *material,
                           const geometry::Fracture *fracture,
                           const std::vector<double> &mx) {

  double horizon = material->getHorizon();
  const auto &xi = nodes[i];
  const auto &ui = nodes_disp[i];
  double m = mx[i];
  double theta = 0.;

  // upper and lower bound for volume correction
  auto check_up = horizon + 0.5 * mesh_size;
  auto check_low = horizon - 0.5 * mesh_size;

  size_t k = 0;
  for (size_t j : neighbors[i]) {

    const auto &xj = nodes[j];
    const auto &uj = nodes_disp[j];
    double rji = (xj - xi).length();
    //double rji = std::sqrt(neighbors_sq_dist[i][k]); // distance in
                                                      // reference configuration

    if (util::isGreater(rji, horizon) or j == i)
      continue;

    // get corrected volume of node j
    auto volj = nodal_vol[j];

    if (util::isGreater(rji, check_low))
      volj *= (check_up - rji) / mesh_size;

    // get bond state
    double bond_state = fracture->getBondState(i, k) ? 0. : 1.;

    // get change in bond length
    auto yi = xi + ui;
    auto yj = xj + uj;
    double change_length = (yj - yi).length() - rji;

    theta += bond_state * rji * change_length *
             material->getInfFn(rji) * volj;

    k += 1;
  }

  return 3. * theta / m;
}

double computeHydrostaticStrainI(size_t i, const std::vector<util::Point> &nodes,
                           const std::vector<util::Point> &nodes_disp,
                           const std::vector<double> &nodal_vol,
                                 const std::vector<std::vector<size_t>> &neighbors,
                                 const std::vector<std::vector<float>> &neighbors_sq_dist,
                           const double &mesh_size,
                           const material::Material *material,
                           const geometry::Fracture *fracture,
                           size_t dim) {

  double horizon = material->getHorizon();
  const auto &xi = nodes[i];
  const auto &ui = nodes_disp[i];
  double theta = 0.;

  // upper and lower bound for volume correction
  auto check_up = horizon + 0.5 * mesh_size;
  auto check_low = horizon - 0.5 * mesh_size;

  // get volume of ball
  double vol_ball = std::pow(horizon, 2) * M_PI;
  if (dim == 3)
    vol_ball *= horizon * 4. / 3.;

  size_t k = 0;
  for (size_t j : neighbors[i]) {

    const auto &xj = nodes[j];
    const auto &uj = nodes_disp[j];
    double rji = (xj - xi).length();
    //double rji = std::sqrt(neighbors_sq_dist[i][k]);

    if (util::isGreater(rji, horizon) or j == i)
      continue;

    // get corrected volume of node j
    auto volj = nodal_vol[j];

    if (util::isGreater(rji, check_low))
      volj *= (check_up - rji) / mesh_size;

    // get bond state
    double bond_state = fracture->getBondState(i, k) ? 0. : 1.;

    // get bond strain
    double Sji = material->getS(xj - xi, uj - ui);

    theta += bond_state * rji * Sji *
             material->getInfFn(rji) * volj / vol_ball;

    k += 1;
  }

  return theta;
}

void updateBondFractureDataI(size_t i, const std::vector<util::Point> &nodes,
                             const std::vector<std::vector<size_t>> &neighbors,
                                 const std::vector<util::Point> &nodes_disp,
                                 const material::Material *material,
                                 geometry::Fracture *fracture) {

  size_t k = 0;
  for (size_t j : neighbors[i]) {

    double s = material->getS(nodes[j] - nodes[i], nodes_disp[j] -
    nodes_disp[i]);
    double sc = material->getSc((nodes[j] - nodes[i]).length());

    // get fracture state, modify, and set
    auto fs = fracture->getBondState(i, k);
    if (!fs && util::isGreater(std::abs(s), sc + 1.0e-10))
      fs = true;
    fracture->setBondState(i, k, fs);

    k += 1;
  }
}

} // anonymous namespace

void material::computeStateMx(model::ModelData *model, bool compute_in_parallel) {

  model->d_mX.resize(model->d_x.size());
  if (!compute_in_parallel) {
    for (size_t i = 0; i < model->d_x.size(); i++) {
      const auto &pti = model->getPtId(i);
      const auto &particle = model->getBaseParticle(pti);
      auto mx = computeStateMxI(i, model->d_xRef, model->d_vol,
                                model->d_neighPd, model->d_neighPdSqdDist,
                                particle->getMeshSize(),
                                particle->getMaterial());

      model->setMx(i, mx);
    }
  } else {

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, model->d_x.size(), (std::size_t) 1, [model](std::size_t i) {
        const auto &pti = model->getPtId(i);
        const auto &particle = model->getBaseParticle(pti);
        auto mx = computeStateMxI(i, model->d_xRef, model->d_vol,
                                  model->d_neighPd, model->d_neighPdSqdDist,
                                  particle->getMeshSize(),
                                  particle->getMaterial());

        model->setMx(i, mx);
      }
    ); // for_each

    executor.run(taskflow).get();
  }
}

void material::computeStateThetax(model::ModelData *model, bool compute_in_parallel) {

  model->d_thetaX.resize(model->d_x.size());
  if (!compute_in_parallel) {
    for (size_t i = 0; i < model->d_x.size(); i++) {
      const auto &pti = model->getPtId(i);
      const auto &particle = model->getBaseParticle(pti);

      auto thetax = computeStateThetaxI(i, model->d_xRef, model->d_u,
                                        model->d_vol,
                                        model->d_neighPd, model->d_neighPdSqdDist,
                                        particle->getMeshSize(),
                                      particle->getMaterial(),
                                        model->d_fracture_p.get(),
                                        model->d_mX);

      model->setThetax(i, thetax);

    }

  } else {

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, model->d_x.size(), (std::size_t) 1, [model](std::size_t i) {
        const auto &pti = model->getPtId(i);
        const auto &particle = model->getBaseParticle(pti);

        auto thetax = computeStateThetaxI(i, model->d_xRef, model->d_u,
                                          model->d_vol,
                                          model->d_neighPd, model->d_neighPdSqdDist,
                                          particle->getMeshSize(),
                                          particle->getMaterial(),
                                          model->d_fracture_p.get(),
                                          model->d_mX);

        model->setThetax(i, thetax);
      }
    ); // for_each

    executor.run(taskflow).get();
  }
}

void material::computeHydrostaticStrain(model::ModelData *model, bool compute_in_parallel) {

  model->d_thetaX.resize(model->d_x.size());
  if (!compute_in_parallel) {
    for (size_t i = 0; i < model->d_x.size(); i++) {
      const auto &pti = model->getPtId(i);
      const auto &particle = model->getBaseParticle(pti);

      auto thetax = computeHydrostaticStrainI(i, model->d_xRef, model->d_u,
                                        model->d_vol,
                                        model->d_neighPd, model->d_neighPdSqdDist,
                                        particle->getMeshSize(),
                                        particle->getMaterial(),
                                        model->d_fracture_p.get(),
                                        particle->getDimension());

      model->setThetax(i, thetax);
    }
  } else {

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, model->d_x.size(), (std::size_t) 1, [model](std::size_t i) {
        const auto &pti = model->getPtId(i);
        const auto &particle = model->getBaseParticle(pti);

        auto thetax = computeHydrostaticStrainI(i, 
          model->d_xRef, 
          model->d_u,
          model->d_vol,
          model->d_neighPd, model->d_neighPdSqdDist,
          particle->getMeshSize(),
          particle->getMaterial(),
          model->d_fracture_p.get(),
          particle->getDimension());

        model->setThetax(i, thetax);
      }
    ); // for_each

    executor.run(taskflow).get();
  }

}

void material::updateBondFractureData(model::ModelData *model, bool compute_in_parallel) {

  if (!compute_in_parallel) {
    for (size_t i = 0; i < model->d_x.size(); i++) {
      const auto &pti = model->getPtId(i);
      const auto &particle = model->getBaseParticle(pti);

      updateBondFractureDataI(i, model->d_xRef, model->d_neighPd, model->d_u,
                              particle->getMaterial(),
                                              model->d_fracture_p.get());
    }
  } else {

    tf::Executor executor(util::parallel::getNThreads());
    tf::Taskflow taskflow;

    taskflow.for_each_index(
      (std::size_t) 0, model->d_x.size(), (std::size_t) 1, [model](std::size_t i) {
        const auto &pti = model->getPtId(i);
        const auto &particle = model->getBaseParticle(pti);

        updateBondFractureDataI(i, 
          model->d_xRef, 
          model->d_neighPd, 
          model->d_u,
          particle->getMaterial(),
          model->d_fracture_p.get());
      }
    ); // for_each

    executor.run(taskflow).get();
  }
}
