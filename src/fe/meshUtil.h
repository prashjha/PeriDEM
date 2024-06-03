/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
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