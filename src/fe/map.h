/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef FE_MAP_H
#define FE_MAP_H

#include "quadData.h"

#include <vector>

namespace fe {

/*!
 * Isoparametric map for quadrature data already filled on the reference
 * element (N, dN/dξ, w_ref, ξ). Writes J, det(J), physical x, w = w_ref det(J),
 * and (if derivatives) dN/dx = J^{-1} dN/dξ.
 */
void mapToPhysical(std::vector<QuadData> &qds,
                   const std::vector<util::Point> &nodes, bool derivatives);

} // namespace fe

#endif // FE_MAP_H
