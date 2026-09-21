/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#pragma once

#include "util/feElementDefs.h"

#include "util/io.h"
#include <stdexcept>
#include "baseElem.h"
#include "lineElem.h"
#include "quadElem.h"
#include "triElem.h"
#include "tetElem.h"
#include "hexElem.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>

namespace fe {

inline std::unique_ptr<BaseElem> elem(size_t type, size_t order) {
  if (type == util::vtk_type_line)
    return std::make_unique<LineElem>(order);
  if (type == util::vtk_type_triangle)
    return std::make_unique<TriElem>(order);
  if (type == util::vtk_type_quad)
    return std::make_unique<QuadElem>(order);
  if (type == util::vtk_type_tetra)
    return std::make_unique<TetElem>(order);
  if (type == util::vtk_type_hexahedron)
    return std::make_unique<HexElem>(order);
  throw std::runtime_error(
      util::io::Msg()
      << std::format("Error: element type = {} is not supported.\n", type));
}

} // namespace fe
