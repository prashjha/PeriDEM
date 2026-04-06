/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef MESH_GEN_OPEN_BOUNDARY_WALLS_3D_H
#define MESH_GEN_OPEN_BOUNDARY_WALLS_3D_H

#include <string>

namespace mesh_gen {

/*!
 * Placeholder for Phase 4B / Phase 6: partition boundary surfaces of a volume into
 * physical groups (wall vs open face). Not used by the 2D open-channel path.
 */
void physicalGroupsWallOpenPlaceholder3D(int volumeTag);

} // namespace mesh_gen

#endif
