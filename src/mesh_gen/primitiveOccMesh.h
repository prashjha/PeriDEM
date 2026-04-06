/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MESH_GEN_PRIMITIVE_OCC_MESH_H
#define MESH_GEN_PRIMITIVE_OCC_MESH_H

namespace geom {
class Cylinder;
}

namespace mesh_gen {

/** OCC cylinder matching `geom::Cylinder` (base center, axis × length, radius). */
void buildCylinderOcc(const geom::Cylinder &c);

} // namespace mesh_gen

#endif
