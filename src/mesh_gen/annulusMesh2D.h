/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef MESH_GEN_ANNULUS_MESH_2D_H
#define MESH_GEN_ANNULUS_MESH_2D_H

namespace geom {
class AnnulusGeomObject;
}

namespace mesh_gen {

/** OCC: outer rectangle minus inner rectangle (coplanar XY), then mesh-size point embed. */
void buildAnnulus2DInCurrentModel(const geom::AnnulusGeomObject &a, double h);

} // namespace mesh_gen

#endif
