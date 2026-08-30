/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef TIME_INT_INTEGRATOR_H
#define TIME_INT_INTEGRATOR_H

#include "data/modelData.h"
#include "util/vecMethods.h"

#include <chrono>
#include <format>
#include <stdexcept>
#include <string>

namespace time_int {

void updateCentralDifference(data::ModelData &data);
void updateVerletHalfKickAndDrift(data::ModelData &data);
void updateVerletSecondKick(data::ModelData &data);

/*!
 * Explicit integrator. The model is passed in; this class runs CD or Verlet
 * using the model's named methods (IC, BCs, forces, output, checkStop) and
 * ModelData kinematics accessors. It does not include PeriDEMModel.
 */
class Integrator {
public:
  template <typename Model>
  void integrate(Model &model) {
    if (model.currentStep() == 0) {
      model.applyInitialCondition();
      if (model.performOutput())
        model.output();
    }

    model.applyDisplacementBC();
    model.computeForces();

    while (model.currentStep() < model.numTimeSteps()) {
      model.log(std::format("{}: Time step: {}, time: {:8.6f}, steps completed = {}%\n",
                            model.d_name, model.currentStep(), model.d_time,
                            float(model.currentStep()) * 100. / model.numTimeSteps()),
                2, model.d_n % model.d_infoN == 0, 3);

      const auto t0 = std::chrono::steady_clock::now();
      step(model);
      const auto dt_ms = util::methods::timeDiff(t0, std::chrono::steady_clock::now());
      model.appendKeyData("integrate_compute_time", dt_ms, true);
      model.log(std::format("  Integration time (ms) = {}\n", dt_ms),
                2, model.d_n % model.d_infoN == 0, 3);

      if (model.shouldOutput())
        model.output();
      model.checkStop();
    }

    model.log(std::format(
        "{}: Total compute time information (s) \n"
        "  {:22s} = {:8.2f} \n"
        "  {:22s} = {:8.2f} \n"
        "  {:22s} = {:8.2f} \n"
        "  {:22s} = {:8.2f} \n"
        "  {:22s} = {:8.2f} \n",
        model.d_name,
        "Time integration", model.getKeyData("integrate_compute_time") * 1.e-6,
        "Peridynamics force", model.getKeyData("pd_compute_time") * 1.e-6,
        "Contact force", model.getKeyData("contact_compute_time") * 1.e-6,
        "Search tree update", model.getKeyData("tree_compute_time") * 1.e-6,
        "External force", model.getKeyData("extf_compute_time") * 1.e-6));
  }

  template <typename Model>
  void step(Model &model) {
    const auto &scheme = model.timeDiscretization();
    model.setCurrentDt(model.timeStep());
    if (scheme == "central_difference") {
      updateCentralDifference(model);
      model.advanceTime();
      model.applyDisplacementBC();
      model.computeForces();
    } else if (scheme == "velocity_verlet") {
      updateVerletHalfKickAndDrift(model);
      model.advanceTime();
      model.applyDisplacementBC();
      model.computeForces();
      updateVerletSecondKick(model);
    } else {
      throw std::runtime_error("Unknown time discretization: " + scheme);
    }
  }
};

} // namespace time_int

#endif // TIME_INT_INTEGRATOR_H
