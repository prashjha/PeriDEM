/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PARTICLE_PARTICLE_MPI_H
#define PARTICLE_PARTICLE_MPI_H

#include <string>

namespace data {
class ModelData;
}

namespace particle {

class BaseParticle;

/*! @brief True if this rank updates / assembles forces for the particle.
 * Walls are replicated on every rank. With one MPI rank, everything is local. */
bool isLocallyOwned(const BaseParticle &p);

/*! @brief Resolved MPI strategy: none|particle|dof (auto expanded). */
std::string resolvedMpiStrategy(const data::ModelData &data);

/*!
 * @brief Assign grain owners by spatial 2D brick decomposition of centers.
 * Walls get owner -1 (all ranks). Honors Model.MPI_Strategy.
 */
void assignMpiOwners(data::ModelData &data);

/*!
 * @brief Distance-limited ghosts + kinematics exchange.
 * Rebuilds the ghost plan on a Verlet-skin cadence (tied to contact neigh
 * interval when available); between rebuilds only exchanges kinematics for
 * the cached ghost set via Alltoallv.
 */
void exchangeGhostKinematics(data::ModelData &data);

} // namespace particle

#endif // PARTICLE_PARTICLE_MPI_H
