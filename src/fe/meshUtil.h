/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#pragma once

#include <string>
#include <vector>

namespace fe {

// forward declare Mesh
class Mesh;

/*! @brief Creates uniform mesh for rectangle/cuboid domain
 *
 * @param mesh_p Pointer to already created possibly empty mesh object
 * @param dim Dimension of the domain
 * @param box Specifies domain (e.g., rectangle/cuboid)
 * @param nGrid Grid sizes in dim directions
 */
void createUniformMesh(fe::Mesh *mesh_p, size_t dim, std::pair<std::vector<double>, std::vector<double>> box, std::vector<size_t> nGrid);

} // namespace fe