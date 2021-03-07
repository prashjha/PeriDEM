/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "baseElem.h"
#include "util/feElementDefs.h"    // global definition of elements
#include <iostream>                // for std::cerr

fe::BaseElem::BaseElem(size_t order, size_t element_type)
    : d_quadOrder(order), d_elemType(element_type),
      d_numQuadPts(util::vtk_map_element_to_num_nodes[element_type]){};

std::vector<double>
fe::BaseElem::getShapes(const util::Point &p,
                        const std::vector<util::Point> &nodes) {
  std::cerr << "Error: For element type = " << d_elemType << " the map from "
            << "element to reference element is not available.\n"
            << "Therefore, shape function evaluation at any arbitrary point "
               "in the element is not possible.\n";
  exit(1);
}

std::vector<std::vector<double>>
fe::BaseElem::getDerShapes(const util::Point &p,
                           const std::vector<util::Point> &nodes) {
  std::cerr << "Error: For element type = " << d_elemType << " the map from "
            << "element to reference element is not available.\n"
            << "Therefore, derivatives of shape function at any "
               "arbitrary point in the element can not be computed.\n";
  exit(1);
}

util::Point
fe::BaseElem::mapPointToRefElem(const util::Point &p,
                                const std::vector<util::Point> &nodes) {
  std::cerr << "Error: For element type = " << d_elemType << " the map from "
            << "element to reference element is not available.\n";
  exit(1);
}

void fe::BaseElem::init() {

  std::cerr << "Error: init() of BaseElem must be implemented in inheriting "
               "class.\n";
  exit(1);
}
