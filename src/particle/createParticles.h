/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PARTICLE_CREATE_PARTICLES_H
#define PARTICLE_CREATE_PARTICLES_H

namespace model {
class ModelData;
}

namespace particle {

/*! @brief Build reference meshes from the particle deck (no placement). */
void createReferenceParticles(model::ModelData &data);

/*! @brief Place particles from generation-file data; refs must already exist. */
void createParticlesFromFile(model::ModelData &data);

/*! @brief Place one particle per zone geometry; refs must already exist. */
void createParticleUsingParticleZoneGeomObject(model::ModelData &data);

/*! @brief Build refs then place particles according to the generation method. */
void createParticles(model::ModelData &data);

} // namespace particle

#endif // PARTICLE_CREATE_PARTICLES_H
