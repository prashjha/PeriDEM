/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "baseElem.h"
#include "util/io.h"
#include <stdexcept>
#include "map.h"
#include <cstdlib>
#include <iostream>

fe::BaseElem::BaseElem(size_t order, size_t element_type)
    : d_quadOrder(order), d_elemType(element_type){};

std::vector<double>
fe::BaseElem::getShapes(const util::Point &p,
                        const std::vector<util::Point> &nodes) {
  throw std::runtime_error(
      util::io::Msg()
      << "Error: For element type = " << d_elemType << " the map from "
      << "element to reference element is not available.\n"
      << "Therefore, shape function evaluation at any arbitrary point "
      "in the element is not possible.\n");
}

std::vector<std::vector<double>>
fe::BaseElem::getDerShapes(const util::Point &p,
                           const std::vector<util::Point> &nodes) {
  throw std::runtime_error(
      util::io::Msg()
      << "Error: For element type = " << d_elemType << " the map from "
      << "element to reference element is not available.\n"
      << "Therefore, derivatives of shape function at any "
      "arbitrary point in the element can not be computed.\n");
}

util::Point
fe::BaseElem::mapPointToRefElem(const util::Point &p,
                                const std::vector<util::Point> &nodes) {
  throw std::runtime_error(
      util::io::Msg()
      << "Error: For element type = " << d_elemType << " the map from "
      << "element to reference element is not available.\n");
}

void fe::BaseElem::init() {

  throw std::runtime_error(
      util::io::Msg()
      << "Error: init() of BaseElem must be implemented in inheriting "
      "class.\n");
}

std::vector<fe::QuadData>
fe::BaseElem::getQuadDatas(const std::vector<util::Point> &nodes) {
  auto qds = d_quads;
  mapToPhysical(qds, nodes, true);
  return qds;
}

std::vector<fe::QuadData>
fe::BaseElem::getQuadPoints(const std::vector<util::Point> &nodes) {
  auto qds = d_quads;
  mapToPhysical(qds, nodes, false);
  return qds;
}
