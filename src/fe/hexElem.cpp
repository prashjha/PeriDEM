/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "hexElem.h"
#include "util/feElementDefs.h"
#include "util/matrix.h"

#include <cmath>
#include <vector>

namespace {

// VTK / Gmsh hex corner signs on [-1,1]^3
constexpr double SX[8] = {-1., 1., 1., -1., -1., 1., 1., -1.};
constexpr double SY[8] = {-1., -1., 1., 1., -1., -1., 1., 1.};
constexpr double SZ[8] = {-1., -1., -1., -1., 1., 1., 1., 1.};

} // namespace

fe::HexElem::HexElem(size_t order)
    : fe::BaseElem(order, util::vtk_type_hexahedron) {
  this->init();
}

double fe::HexElem::elemSize(const std::vector<util::Point> &nodes) {
  // Reference cube volume is 8; for a parallelepiped V = 8 * det(J(0,0,0)).
  // General hex: integrate |det J| with the stored reference quadrature.
  if (d_quads.empty())
    return 8. * getJacobian(util::Point(0., 0., 0.), nodes, nullptr);
  double vol = 0.;
  for (const auto &qd : d_quads)
    vol += qd.d_w * getJacobian(qd.d_p, nodes, nullptr);
  return vol;
}

std::vector<double> fe::HexElem::getShapes(const util::Point &p) {
  std::vector<double> N(8);
  for (size_t i = 0; i < 8; ++i)
    N[i] = 0.125 * (1. + SX[i] * p.d_x) * (1. + SY[i] * p.d_y) *
           (1. + SZ[i] * p.d_z);
  return N;
}

std::vector<std::vector<double>>
fe::HexElem::getDerShapes(const util::Point &p) {
  std::vector<std::vector<double>> r(8, std::vector<double>(3, 0.));
  for (size_t i = 0; i < 8; ++i) {
    r[i][0] = 0.125 * SX[i] * (1. + SY[i] * p.d_y) * (1. + SZ[i] * p.d_z);
    r[i][1] = 0.125 * SY[i] * (1. + SX[i] * p.d_x) * (1. + SZ[i] * p.d_z);
    r[i][2] = 0.125 * SZ[i] * (1. + SX[i] * p.d_x) * (1. + SY[i] * p.d_y);
  }
  return r;
}

double fe::HexElem::getJacobian(const util::Point &p,
                                const std::vector<util::Point> &nodes,
                                std::vector<std::vector<double>> *J) {
  auto der = getDerShapes(p);
  std::vector<std::vector<double>> Jloc(3, std::vector<double>(3, 0.));
  for (size_t a = 0; a < 8; ++a) {
    Jloc[0][0] += der[a][0] * nodes[a].d_x;
    Jloc[0][1] += der[a][0] * nodes[a].d_y;
    Jloc[0][2] += der[a][0] * nodes[a].d_z;
    Jloc[1][0] += der[a][1] * nodes[a].d_x;
    Jloc[1][1] += der[a][1] * nodes[a].d_y;
    Jloc[1][2] += der[a][1] * nodes[a].d_z;
    Jloc[2][0] += der[a][2] * nodes[a].d_x;
    Jloc[2][1] += der[a][2] * nodes[a].d_y;
    Jloc[2][2] += der[a][2] * nodes[a].d_z;
  }
  if (J != nullptr)
    *J = Jloc;
  return util::det(Jloc);
}

void fe::HexElem::init() {
  if (!d_quads.empty())
    return;

  if (d_quadOrder == 0) {
    d_quads.resize(0);
    return;
  }

  std::vector<std::vector<double>> ident(3, std::vector<double>(3, 0.));
  ident[0][0] = ident[1][1] = ident[2][2] = 1.;

  std::vector<double> x;
  std::vector<double> w;
  if (d_quadOrder == 1) {
    x = {0.};
    w = {2.};
  } else if (d_quadOrder == 2) {
    x = {-1. / std::sqrt(3.), 1. / std::sqrt(3.)};
    w = {1., 1.};
  } else {
    // order >= 3: 3-point Gauss on [-1,1]
    x = {-std::sqrt(3.) / std::sqrt(5.), 0., std::sqrt(3.) / std::sqrt(5.)};
    w = {5. / 9., 8. / 9., 5. / 9.};
  }

  const size_t npts = x.size();
  d_quads.clear();
  d_quads.reserve(npts * npts * npts);
  for (size_t i = 0; i < npts; ++i)
    for (size_t j = 0; j < npts; ++j)
      for (size_t k = 0; k < npts; ++k) {
        fe::QuadData qd;
        qd.d_w = w[i] * w[j] * w[k];
        qd.d_p = util::Point(x[i], x[j], x[k]);
        qd.d_shapes = getShapes(qd.d_p);
        qd.d_derShapes = getDerShapes(qd.d_p);
        qd.d_J = ident;
        qd.d_detJ = 1.;
        d_quads.push_back(qd);
      }
}
