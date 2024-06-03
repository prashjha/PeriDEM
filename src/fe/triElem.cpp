/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "triElem.h"
#include "util/feElementDefs.h" // global definition of elements
#include "util/function.h"
#include <iostream>

fe::TriElem::TriElem(size_t order)
    : fe::BaseElem(order, util::vtk_type_triangle) {

  // compute quad data
  this->init();
}

double fe::TriElem::elemSize(const std::vector<util::Point> &nodes) {
  return 0.5 * ((nodes[1].d_x - nodes[0].d_x) * (nodes[2].d_y - nodes[0].d_y) -
                (nodes[2].d_x - nodes[0].d_x) * (nodes[1].d_y - nodes[0].d_y));
}

std::vector<double>
fe::TriElem::getShapes(const util::Point &p,
                       const std::vector<util::Point> &nodes) {
  return getShapes(mapPointToRefElem(p, nodes));
}

std::vector<std::vector<double>>
fe::TriElem::getDerShapes(const util::Point &p,
                          const std::vector<util::Point> &nodes) {
  // get derivatives of shape function in reference triangle
  auto ders_ref = getDerShapes(mapPointToRefElem(p, nodes));

  // get Jacobian and its determinant
  std::vector<std::vector<double>> J;
  auto detJ = getJacobian(p, nodes, &J);

  // to hold derivatives
  auto ders = ders_ref;

  for (size_t i=0; i<3; i++) {
    // partial N_i/ partial x
    ders[i][0] = (ders_ref[i][0] * J[1][1] - ders_ref[i][1] * J[0][1]) / detJ;
    // partial N_i/ partial y
    ders[i][1] = (-ders_ref[i][0] * J[1][0] + ders_ref[i][1] * J[0][0]) / detJ;
  }

  return ders;
}

std::vector<fe::QuadData>
fe::TriElem::getQuadDatas(const std::vector<util::Point> &nodes) {

  // copy quad data associated to reference element
  auto qds = d_quads;

  // modify data
  for (auto &qd : qds) {

    // get Jacobian and determinant
    qd.d_detJ = getJacobian(qd.d_p, nodes, &(qd.d_J));

    // transform quad weight
    qd.d_w *= qd.d_detJ;

    // map point to triangle
    qd.d_p.d_x = qd.d_shapes[0] * nodes[0].d_x + qd.d_shapes[1] * nodes[1].d_x +
        qd.d_shapes[2] * nodes[2].d_x;
    qd.d_p.d_y = qd.d_shapes[0] * nodes[0].d_y + qd.d_shapes[1] * nodes[1].d_y +
        qd.d_shapes[2] * nodes[2].d_y;

    // derivatives of shape function
    std::vector<std::vector<double>> ders;

    for (size_t i=0; i<3; i++) {
      // partial N_i/ partial x
      auto d1 = (qd.d_derShapes[i][0] * qd.d_J[1][1] - qd.d_derShapes[i][1] *
          qd.d_J[0][1]) / qd.d_detJ;
      // partial N_i/ partial y
      auto d2 = (-qd.d_derShapes[i][0] * qd.d_J[1][0] + qd.d_derShapes[i][1] *
          qd.d_J[0][0]) / qd.d_detJ;

      ders.push_back(std::vector<double>{d1, d2});
    }
    qd.d_derShapes = ders;
  }

  return qds;
}

std::vector<fe::QuadData>
fe::TriElem::getQuadPoints(const std::vector<util::Point> &nodes) {

  // copy quad data associated to reference element
  auto qds = d_quads;

  // modify data
  for (auto &qd : qds) {

    // transform quad weight
    qd.d_w *= getJacobian(qd.d_p, nodes, nullptr);

    // map point to triangle
    qd.d_p.d_x = qd.d_shapes[0] * nodes[0].d_x + qd.d_shapes[1] * nodes[1].d_x +
        qd.d_shapes[2] * nodes[2].d_x;
    qd.d_p.d_y = qd.d_shapes[0] * nodes[0].d_y + qd.d_shapes[1] * nodes[1].d_y +
        qd.d_shapes[2] * nodes[2].d_y;
  }

  return qds;
}

std::vector<double> fe::TriElem::getShapes(const util::Point &p) {

  // N1 = 1 - xi - eta, N2 = xi, N3 = eta
  return std::vector<double>{1. - p.d_x - p.d_y, p.d_x, p.d_y};
}

std::vector<std::vector<double>>
fe::TriElem::getDerShapes(const util::Point &p) {

  // d N1/d xi = -1, d N1/d eta = -1, d N2/ d xi = 1, d N2/d eta = 0,
  // d N3/ d xi = 0, d N3/d eta = 1
  std::vector<std::vector<double>> r;
  r.push_back(std::vector<double>{-1., -1.});
  r.push_back(std::vector<double>{1., 0.});
  r.push_back(std::vector<double>{0., 1.});

  return r;
}

util::Point
fe::TriElem::mapPointToRefElem(const util::Point &p,
                               const std::vector<util::Point> &nodes) {
  auto detB = 2. * elemSize(nodes);
  auto xi = ((nodes[2].d_y - nodes[0].d_y) * (p.d_x - nodes[0].d_x) -
             (nodes[2].d_x - nodes[0].d_x) * (p.d_y - nodes[0].d_y)) /
      detB;
  auto eta = (-(nodes[1].d_y - nodes[0].d_y) * (p.d_x - nodes[0].d_x) +
              (nodes[1].d_x - nodes[0].d_x) * (p.d_y - nodes[0].d_y)) /
      detB;

  if (util::isLess(xi, -1.0E-5) || util::isLess(eta, -1.0E-5) ||
      util::isGreater(xi, 1. + 1.0E-5 - eta)) {
    std::cerr << "Error: Trying to map point p = (" << p.d_x << ", " << p.d_y
              << ") in triangle to reference triangle.\n"
              << "But the point p does not belong to triangle = {("
              << nodes[0].d_x << ", " << nodes[0].d_y << "), (" << nodes[1].d_x
              << "," << nodes[1].d_y << "), (" << nodes[2].d_x << ","
              << nodes[2].d_y << ")}.\n"
              << "Coordinates in reference triangle are: xi = " << xi
              << ", eta = " << eta << "\n";
    exit(1);
  }

  if (util::isLess(xi, 0.))
    xi = 0.;
  if (util::isLess(eta, 0.))
    eta = 0.;

  return {xi, eta, 0.};
}

double fe::TriElem::getJacobian(const util::Point &p,
            const std::vector<util::Point> &nodes,
            std::vector<std::vector<double>> *J) {

  if (J != nullptr) {
    J->resize(2);
    (*J)[0] = std::vector<double>{nodes[1].d_x - nodes[0].d_x,
                                  nodes[1].d_y - nodes[0].d_y};
    (*J)[1] = std::vector<double>{nodes[2].d_x - nodes[0].d_x,
                                  nodes[2].d_y - nodes[0].d_y};

    return (*J)[0][0] * (*J)[1][1] - (*J)[0][1] * (*J)[1][0];
  }

  return (nodes[1].d_x - nodes[0].d_x) * (nodes[2].d_y - nodes[0].d_y)
        - (nodes[1].d_y - nodes[0].d_y) * (nodes[2].d_x - nodes[0].d_x);
}

void fe::TriElem::init() {

  //
  // compute quad data for reference triangle with vertex at
  // (0,0), (1,0), (0,1)
  //

  if (!d_quads.empty())
    return;

  // no point in zeroth order
  if (d_quadOrder == 0) {
    d_quads.resize(0);
  }

  // 2x2 identity matrix
  std::vector<std::vector<double>> ident_mat;
  ident_mat.push_back(std::vector<double>{1., 0.});
  ident_mat.push_back(std::vector<double>{0., 1.});

  //
  // first order quad points for triangle
  //
  if (d_quadOrder == 1) {
    d_quads.clear();
    fe::QuadData qd;
    qd.d_w = 0.5;
    qd.d_p = util::Point(1. / 3., 1. / 3., 0.);
    // N1 = 1 - xi - eta, N2 = xi, N3 = eta
    qd.d_shapes = getShapes(qd.d_p);
    // d N1/d xi = -1, d N1/d eta = -1, d N2/ d xi = 1, d N2/d eta = 0,
    // d N3/ d xi = 0, d N3/d eta = 1
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }

  //
  // second order quad points for triangle
  //
  if (d_quadOrder == 2) {
    d_quads.clear();
    fe::QuadData qd;
    // point 1
    qd.d_w = 1. / 6.;
    qd.d_p = util::Point(1. / 6., 1. / 6., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 2
    qd.d_w = 1. / 6.;
    qd.d_p = util::Point(2. / 3., 1. / 6., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 3
    qd.d_w = 1. / 6.;
    qd.d_p = util::Point(1. / 6., 2. / 3., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }

  //
  // third order quad points for triangle
  //
  if (d_quadOrder == 3) {
    d_quads.clear();
    fe::QuadData qd;
    // point 1
    qd.d_w = -27. / 96.;
    qd.d_p = util::Point(1. / 3., 1. / 3., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 2
    qd.d_w = 25. / 96.;
    qd.d_p = util::Point(1. / 5., 3. / 5., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 3
    qd.d_w = 25. / 96.;
    qd.d_p = util::Point(1. / 5., 1. / 5., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 4
    qd.d_w = 25. / 96.;
    qd.d_p = util::Point(3. / 5., 1. / 5., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }

  //
  // fourth order quad points for triangle
  //
  if (d_quadOrder == 4) {
    d_quads.clear();
    fe::QuadData qd;
    // point 1
    qd.d_w = 0.5 * 0.22338158967801;
    qd.d_p = util::Point(0.44594849091597, 0.44594849091597, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 2
    qd.d_w = 0.5 * 0.22338158967801;
    qd.d_p = util::Point(0.44594849091597, 0.10810301816807, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 3
    qd.d_w = 0.5 * 0.22338158967801;
    qd.d_p = util::Point(0.10810301816807, 0.44594849091597, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 4
    qd.d_w = 0.5 * 0.10995174365532;
    qd.d_p = util::Point(0.09157621350977, 0.09157621350977, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 5
    qd.d_w = 0.5 * 0.10995174365532;
    qd.d_p = util::Point(0.09157621350977, 0.81684757298046, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 6
    qd.d_w = 0.5 * 0.10995174365532;
    qd.d_p = util::Point(0.81684757298046, 0.09157621350977, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }

  //
  // fifth order quad points for triangle
  //
  if (d_quadOrder == 5) {
    d_quads.clear();
    fe::QuadData qd;
    // point 1
    qd.d_w = 0.5 * 0.22500000000000;
    qd.d_p = util::Point(0.33333333333333, 0.33333333333333, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 2
    qd.d_w = 0.5 * 0.13239415278851;
    qd.d_p = util::Point(0.47014206410511, 0.47014206410511, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 3
    qd.d_w = 0.5 * 0.13239415278851;
    qd.d_p = util::Point(0.47014206410511, 0.05971587178977, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 4
    qd.d_w = 0.5 * 0.13239415278851;
    qd.d_p = util::Point(0.05971587178977, 0.47014206410511, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 5
    qd.d_w = 0.5 * 0.12593918054483;
    qd.d_p = util::Point(0.10128650732346, 0.10128650732346, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 6
    qd.d_w = 0.5 * 0.12593918054483;
    qd.d_p = util::Point(0.10128650732346, 0.79742698535309, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 7
    qd.d_w = 0.5 * 0.12593918054483;
    qd.d_p = util::Point(0.79742698535309, 0.10128650732346, 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }
}
