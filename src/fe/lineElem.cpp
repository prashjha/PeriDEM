/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "lineElem.h"
#include "util/feElementDefs.h" // global definition of elements
#include "util/function.h"
#include <iostream>

fe::LineElem::LineElem(size_t order)
    : fe::BaseElem(order, util::vtk_type_line) {

  // compute quad data
  this->init();
}

double fe::LineElem::elemSize(const std::vector<util::Point> &nodes) {
  return (nodes[1].d_x - nodes[0].d_x);
}

std::vector<double>
fe::LineElem::getShapes(const util::Point &p,
                       const std::vector<util::Point> &nodes) {
  return getShapes(mapPointToRefElem(p, nodes));
}

std::vector<std::vector<double>>
fe::LineElem::getDerShapes(const util::Point &p,
                          const std::vector<util::Point> &nodes) {
  // get derivatives of shape function in reference triangle
  auto ders_ref = getDerShapes(mapPointToRefElem(p, nodes));

  // get Jacobian
  auto detJ = getJacobian(p, nodes, nullptr);

  // modify derivatives of shape function
  ders_ref[0][0] = ders_ref[0][0] / detJ;
  ders_ref[1][0] = ders_ref[1][0] / detJ;

  return ders_ref;
}

std::vector<fe::QuadData>
fe::LineElem::getQuadDatas(const std::vector<util::Point> &nodes) {

  // copy quad data associated to reference element
  auto qds = d_quads;

  // modify data
  for (auto &qd : qds) {

    // get Jacobian and determinant
    qd.d_detJ = getJacobian(qd.d_p, nodes, &(qd.d_J));

    // transform quad weight
    qd.d_w *= qd.d_detJ;

    // map point to line
    qd.d_p.d_x = qd.d_shapes[0] * nodes[0].d_x + qd.d_shapes[1] * nodes[1]
        .d_x;

    // modify derivative of shape function
    for (size_t i=0; i<2; i++)
      qd.d_derShapes[i][0] = qd.d_derShapes[i][0] / qd.d_detJ;
  }

  return qds;
}

std::vector<fe::QuadData>
fe::LineElem::getQuadPoints(const std::vector<util::Point> &nodes) {

  // copy quad data associated to reference element
  auto qds = d_quads;

  // modify data
  for (auto &qd : qds) {

    // transform quad weight
    qd.d_w *= getJacobian(qd.d_p, nodes, nullptr);

    // map point to line
    qd.d_p.d_x = qd.d_shapes[0] * nodes[0].d_x + qd.d_shapes[1] * nodes[1]
        .d_x;
  }

  return qds;
}

std::vector<double> fe::LineElem::getShapes(const util::Point &p) {

  // N1 = (1 - xi)/2
  // N2 = (1 + xi)/2
  return std::vector<double>{0.5 * (1. - p.d_x), 0.5 * (1. + p.d_x)};
}

std::vector<std::vector<double>>
fe::LineElem::getDerShapes(const util::Point &p) {

  // N1 = (1 - xi)/2
  // --> d N1/d xi = -1/2
  //
  // N2 = (1 + xi)/2
  // --> d N2/d xi = 1/2
  std::vector<std::vector<double>> r;
  r.push_back(std::vector<double>{-0.5});
  r.push_back(std::vector<double>{0.5});
  return r;
}

util::Point
fe::LineElem::mapPointToRefElem(const util::Point &p,
                               const std::vector<util::Point> &nodes) {
  auto xi = (2. * p.d_x - nodes[0].d_x - nodes[1].d_x) /
            (nodes[0].d_x - nodes[1].d_x);

  if (util::isLess(xi, -1. - 1.0E-8) ||
      util::isGreater(xi, 1. + 1.0E-8) ) {
    std::cerr << "Error: Trying to map point p = " << p.d_x
              << " in given line to reference line.\n"
              << "But the point p does not belong to line = {"
              << nodes[0].d_x << ", " << nodes[1].d_x << "}.\n";
    exit(1);
  }

  if (util::isLess(xi, -1.))
    xi = -1.;
  if (util::isGreater(xi, 1.))
    xi = 1.;

  return {xi, 0., 0.};
}

double fe::LineElem::getJacobian(const util::Point &p,
                                const std::vector<util::Point> &nodes,
                                std::vector<std::vector<double>> *J) {

  if (J != nullptr) {
    J->resize(1);
    (*J)[0] = std::vector<double>{nodes[1].d_x - nodes[0].d_x};

    return (*J)[0][0];
  }

  return (nodes[1].d_x - nodes[0].d_x);
}

void fe::LineElem::init() {

  //
  // compute quad data for reference line element with vertex at p1 = -1 and
  // p2 = 1
  //
  // Shape functions are
  // N1 = (1 - xi)/2
  // N2 = (1 + xi)/2

  if (!d_quads.empty())
    return;

  // no point in zeroth order
  if (d_quadOrder == 0)
    d_quads.resize(0);

  // 1x1 identity matrix
  std::vector<std::vector<double>> ident_mat;
  ident_mat.push_back(std::vector<double>{1.});

  //
  // first order quad points
  //
  if (d_quadOrder == 1) {
    d_quads.clear();
    // 1-d points are: {0} and weights are: {2}
    fe::QuadData qd;
    qd.d_w = 2.;
    qd.d_p = util::Point();
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }

  //
  // second order quad points
  //
  if (d_quadOrder == 2) {
    d_quads.clear();
    // 1-d points are: {-1/sqrt{3], 1/sqrt{3}} and weights are: {1,1}
    fe::QuadData qd;
    qd.d_w = 1.;
    qd.d_p = util::Point(-1. / std::sqrt(3.), 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 1.;
    qd.d_p = util::Point(1. / std::sqrt(3.), 0., 0.);
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
    // 1-d points are: {-sqrt{3}/sqrt{5}, 0, sqrt{3}/sqrt{5}}
    // weights are: {5/9, 8/9, 5/9}
    fe::QuadData qd;
    qd.d_w = 5. / 9.;
    qd.d_p = util::Point(-std::sqrt(3.) / std::sqrt(5.), 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 8. / 9.;
    qd.d_p = util::Point();
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 5. / 9.;
    qd.d_p = util::Point(std::sqrt(3.) / std::sqrt(5.), 0., 0.);
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
    qd.d_w = 0.6521451548625461;
    qd.d_p = util::Point(-0.3399810435848563, 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 0.6521451548625461;
    qd.d_p = util::Point(0.3399810435848563, 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 0.3478548451374538;
    qd.d_p = util::Point(-0.8611363115940526, 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 0.3478548451374538;
    qd.d_p = util::Point(0.8611363115940526, 0., 0.);
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
    qd.d_w = 0.5688888888888889;
    qd.d_p = util::Point();
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 0.4786286704993665;
    qd.d_p = util::Point(-0.5384693101056831, 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 0.4786286704993665;
    qd.d_p = util::Point(0.5384693101056831, 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 0.2369268850561891;
    qd.d_p = util::Point(-0.9061798459386640, 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);

    qd.d_w = 0.2369268850561891;
    qd.d_p = util::Point(0.9061798459386640, 0., 0.);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }
}
