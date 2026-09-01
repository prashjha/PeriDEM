/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "bMatrix.h"

fe::B::B(const std::vector<std::vector<double>> &dNdx, int dim)
    : d_dim(dim), d_nStrain(dim == 2 ? 3 : (dim == 3 ? 6 : 1)),
      d_nDof(dim * static_cast<int>(dNdx.size())),
      d_data(static_cast<size_t>(d_nStrain * d_nDof), 0.) {
  const int n = static_cast<int>(dNdx.size());
  for (int a = 0; a < n; a++) {
    const auto &dN = dNdx[a];
    const int c = dim * a;
    auto set = [&](int i, int j, double v) { d_data[i * d_nDof + j] = v; };

    set(0, c, dN[0]);
    if (dim == 1)
      continue;
    set(1, c + 1, dN[1]);
    if (dim == 2) {
      set(2, c, 0.5 * dN[1]);
      set(2, c + 1, 0.5 * dN[0]);
      continue;
    }
    set(2, c + 2, dN[2]);
    set(3, c + 1, 0.5 * dN[2]);
    set(3, c + 2, 0.5 * dN[1]);
    set(4, c, 0.5 * dN[2]);
    set(4, c + 2, 0.5 * dN[0]);
    set(5, c, 0.5 * dN[1]);
    set(5, c + 1, 0.5 * dN[0]);
  }
}
