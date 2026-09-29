/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef FE_HEXELEM_H
#define FE_HEXELEM_H

#include "baseElem.h"

namespace fe {

/*!
 * @brief Trilinear hexahedron (VTK type 12) on reference cube [-1,1]^3.
 *
 * Node ordering matches VTK / createUniformMesh: bottom face 0-1-2-3
 * (ξ,η at ζ=-1), top face 4-5-6-7 (ζ=+1).
 */
class HexElem : public BaseElem {

public:
  explicit HexElem(size_t order);

  double elemSize(const std::vector<util::Point> &nodes) override;

private:
  std::vector<double> getShapes(const util::Point &p) override;

  std::vector<std::vector<double>> getDerShapes(const util::Point &p) override;

  double getJacobian(const util::Point &p,
                     const std::vector<util::Point> &nodes,
                     std::vector<std::vector<double>> *J) override;

  void init() override;
};

} // namespace fe

#endif // FE_HEXELEM_H
