/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PD_PD_MPI_H
#define PD_PD_MPI_H

namespace data {
class ModelData;
}

namespace pd {

/*! @brief True when nodal DOF MPI is active (mpiSize > 1, MPI_Strategy=dof). */
bool dofMpiEnabled(const data::ModelData &data);

/*!
 * @brief Metis-partition nodes on the PD neighbor graph and build ghost plans.
 * Call after d_neighPd is ready and before (or while) building d_fPdCompNodes.
 * Works for Single_Particle and Multi_Particle when MPI_Strategy is dof.
 * No-op for single-rank runs or other strategies.
 */
void setupDofPartition(data::ModelData &data);

/*! @brief Halo-exchange nodal displacements (and current x) for PD ghosts. */
void exchangeGhostDisplacement(data::ModelData &data);

/*! @brief Halo-exchange state-based dilatation d_thetaX for PD ghosts. */
void exchangeGhostTheta(data::ModelData &data);

} // namespace pd

#endif // PD_PD_MPI_H
