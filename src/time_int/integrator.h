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

namespace model {
class ModelData;
}

namespace time_int {

/*! @brief Central-difference velocity / displacement / position update. */
void updateCentralDifference(model::ModelData &data);

/*! @brief Velocity-Verlet first kick + drift. */
void updateVerletHalfKickAndDrift(model::ModelData &data);

/*! @brief Velocity-Verlet second kick. */
void updateVerletSecondKick(model::ModelData &data);

} // namespace time_int

#endif // TIME_INT_INTEGRATOR_H
