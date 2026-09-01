/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef FE_BMATRIX_H
#define FE_BMATRIX_H

#include <vector>

namespace fe {

/*!
 * Strain-displacement matrix B at a quadrature point, from physical dN/dx.
 * Voigt rows: 2D [εxx, εyy, εxy], 3D [εxx, εyy, εzz, εyz, εxz, εxy]
 * (tensor shear, not engineering γ). Columns are nodal u packed
 * [u1x, u1y, (u1z,) u2x, ...]. Does not take displacements; ε = B u
 * is applied by the caller.
 */
class B {
public:
  B(const std::vector<std::vector<double>> &dNdx, int dim);

  int nStrain() const { return d_nStrain; }
  int nDof() const { return d_nDof; }
  int dim() const { return d_dim; }

  double operator()(int i, int j) const {
    return d_data[i * d_nDof + j];
  }

private:
  int d_dim;
  int d_nStrain;
  int d_nDof;
  std::vector<double> d_data;
};

} // namespace fe

#endif // FE_BMATRIX_H
