/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "map.h"
#include "util/matrix.h"

void fe::mapToPhysical(std::vector<QuadData> &qds,
                       const std::vector<util::Point> &nodes, bool derivatives) {
  for (auto &qd : qds) {
    const auto n = qd.d_shapes.size();
    if (qd.d_derShapes.size() != n || n == 0 || qd.d_derShapes[0].empty())
      continue;
    const int dim = static_cast<int>(qd.d_derShapes[0].size());

    std::vector<std::vector<double>> J(dim, std::vector<double>(dim, 0.));
    for (int a = 0; a < static_cast<int>(n); a++)
      for (int alpha = 0; alpha < dim; alpha++)
        for (int j = 0; j < dim; j++)
          J[alpha][j] += qd.d_derShapes[a][alpha] * nodes[a][j];

    qd.d_J = J;
    qd.d_detJ = util::det(J);
    qd.d_w *= qd.d_detJ;

    qd.d_p = util::Point();
    for (size_t a = 0; a < n; a++)
      for (int j = 0; j < dim; j++)
        qd.d_p[j] += qd.d_shapes[a] * nodes[a][j];

    if (!derivatives)
      continue;

    const auto Jinv = util::inv(J);
    for (size_t a = 0; a < n; a++)
      qd.d_derShapes[a] = util::dot(Jinv, qd.d_derShapes[a]);
  }
}
