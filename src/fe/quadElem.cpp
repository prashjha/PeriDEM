/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "quadElem.h"
#include "util/feElementDefs.h"     // global definition of elements

fe::QuadElem::QuadElem(size_t order)
    : fe::BaseElem(order, util::vtk_type_quad) {

  // compute quad data
  this->init();
}

double fe::QuadElem::elemSize(const std::vector<util::Point> &nodes) {
  return 0.25 *
         ((-nodes[0].d_x + nodes[1].d_x + nodes[2].d_x - nodes[3].d_x) *
              (-nodes[0].d_y - nodes[1].d_y + nodes[2].d_y + nodes[3].d_y) -
          (-nodes[0].d_x - nodes[1].d_x + nodes[2].d_x + nodes[3].d_x) *
              (-nodes[0].d_y + nodes[1].d_y + nodes[2].d_y - nodes[3].d_y));
}



std::vector<fe::QuadData>
fe::QuadElem::getQuadDatas(const std::vector<util::Point> &nodes) {

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
        qd.d_shapes[2] * nodes[2].d_x + qd.d_shapes[3] * nodes[3].d_x;
    qd.d_p.d_y = qd.d_shapes[0] * nodes[0].d_y + qd.d_shapes[1] * nodes[1].d_y +
        qd.d_shapes[2] * nodes[2].d_y + qd.d_shapes[3] * nodes[3].d_y;

    // derivatives of shape function
    std::vector<std::vector<double>> ders;

    for (size_t i=3; i<4; i++) {
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
fe::QuadElem::getQuadPoints(const std::vector<util::Point> &nodes) {

  // copy quad data associated to reference element
  auto qds = d_quads;

  // modify data
  for (auto &qd : qds) {

    // transform quad weight
    qd.d_w *= getJacobian(qd.d_p, nodes, nullptr);

    // map point to triangle
    qd.d_p.d_x = qd.d_shapes[0] * nodes[0].d_x + qd.d_shapes[1] * nodes[1].d_x +
        qd.d_shapes[2] * nodes[2].d_x + qd.d_shapes[3] * nodes[3].d_x;
    qd.d_p.d_y = qd.d_shapes[0] * nodes[0].d_y + qd.d_shapes[1] * nodes[1].d_y +
        qd.d_shapes[2] * nodes[2].d_y + qd.d_shapes[3] * nodes[3].d_y;
  }

  return qds;
}

std::vector<double> fe::QuadElem::getShapes(const util::Point &p) {

  // N1 = (1 - xi)(1 - eta)/4
  // N2 = (1 + xi)(1 - eta)/4
  // N3 = (1 + xi)(1 + eta)/4
  // N4 = (1 - xi)(1 + eta)/4
  return std::vector<double>{
      0.25 * (1. - p.d_x) * (1. - p.d_y), 0.25 * (1. + p.d_x) * (1. - p.d_y),
      0.25 * (1. + p.d_x) * (1. + p.d_y), 0.25 * (1. - p.d_x) * (1. + p.d_y)};
}

std::vector<std::vector<double>>
fe::QuadElem::getDerShapes(const util::Point &p) {

  // N1 = (1 - xi)(1 - eta)/4
  // --> d N1/d xi = -(1 - eta)/4, d N1/d eta = -(1 - xi)/4
  //
  // N2 = (1 + xi)(1 - eta)/4
  // --> d N2/d xi = (1 - eta)/4, d N2/d eta = -(1 + xi)/4
  //
  // N3 = (1 + xi)(1 + eta)/4
  // --> d N3/d xi = (1 + eta)/4, d N3/d eta = (1 + xi)/4
  //
  // N4 = (1 - xi)(1 + eta)/4
  // --> d N4/d xi = -(1 + eta)/4, d N4/d eta = (1 - xi)/4
  std::vector<std::vector<double>> r;
  r.push_back(std::vector<double>{-0.25 * (1. - p.d_y), -0.25 * (1. - p.d_x)});
  r.push_back(std::vector<double>{0.25 * (1. - p.d_y), -0.25 * (1. + p.d_x)});
  r.push_back(std::vector<double>{0.25 * (1. + p.d_y), 0.25 * (1. + p.d_x)});
  r.push_back(std::vector<double>{-0.25 * (1. + p.d_y), 0.25 * (1. - p.d_x)});

  return r;
}

double fe::QuadElem::getJacobian(const util::Point &p,
                                const std::vector<util::Point> &nodes,
                                std::vector<std::vector<double>> *J) {

  auto der_shapes = getDerShapes(p);
  if (J != nullptr) {
    J->resize(2);
    (*J)[0] = std::vector<double>{
        der_shapes[0][0] * nodes[0].d_x + der_shapes[1][0] * nodes[1].d_x +
            der_shapes[2][0] * nodes[2].d_x + der_shapes[3][0] * nodes[3].d_x,
        der_shapes[0][0] * nodes[0].d_y + der_shapes[1][0] * nodes[1].d_y +
            der_shapes[2][0] * nodes[2].d_y + der_shapes[3][0] * nodes[3].d_y};
    (*J)[1] = std::vector<double>{
        der_shapes[0][1] * nodes[0].d_x + der_shapes[1][1] * nodes[1].d_x +
            der_shapes[2][1] * nodes[2].d_x + der_shapes[3][1] * nodes[3].d_x,
        der_shapes[0][1] * nodes[0].d_y + der_shapes[1][1] * nodes[1].d_y +
            der_shapes[2][1] * nodes[2].d_y + der_shapes[3][1] * nodes[3].d_y};

    return (*J)[0][0] * (*J)[1][1] - (*J)[0][1] * (*J)[1][0];
  }

  return (der_shapes[0][0] * nodes[0].d_x + der_shapes[1][0] * nodes[1].d_x +
          der_shapes[2][0] * nodes[2].d_x + der_shapes[3][0] * nodes[3].d_x) *
             (der_shapes[0][1] * nodes[0].d_y +
              der_shapes[1][1] * nodes[1].d_y +
              der_shapes[2][1] * nodes[2].d_y +
              der_shapes[3][1] * nodes[3].d_y) -
         (der_shapes[0][0] * nodes[0].d_y + der_shapes[1][0] * nodes[1].d_y +
          der_shapes[2][0] * nodes[2].d_y + der_shapes[3][0] * nodes[3].d_y) *
             (der_shapes[0][1] * nodes[0].d_x +
              der_shapes[1][1] * nodes[1].d_x +
              der_shapes[2][1] * nodes[2].d_x +
              der_shapes[3][1] * nodes[3].d_x);
}

void fe::QuadElem::init() {

  //
  // compute quad data for reference quadrangle with vertex at
  // p1 = (-1,-1), p2 = (1,-1), p3 = (1,1), p4 = (-1,1)
  //
  // Shape functions are
  // N1 = (1 - xi)(1 - eta)/4
  // N2 = (1 + xi)(1 - eta)/4
  // N3 = (1 + xi)(1 + eta)/4
  // N4 = (1 - xi)(1 + eta)/4
  //
  //
  //  Let [-1,1] is the 1-d reference element and {x1, x2, x3,.., xN} are N
  //  quad points for 1-d domain and {w1, w2, w3,..., wN} are respective
  //  weights. Then, the Nth order quad points in Quadrangle [-1,1]x[-1,1] is
  //  simply given by N^2 points and
  //
  //  (i,j) point is (xi, xj) and weight is wi \times wj
  //

  if (!d_quads.empty())
    return;

  // no point in zeroth order
  if (d_quadOrder == 0)
    d_quads.resize(0);

  // 2x2 identity matrix
  std::vector<std::vector<double>> ident_mat;
  ident_mat.push_back(std::vector<double>{1., 0.});
  ident_mat.push_back(std::vector<double>{0., 1.});

  //
  // first order quad points
  //
  if (d_quadOrder == 1) {
    d_quads.clear();
    // 1-d points are: {0} and weights are: {2}
    int npts = 1;
    std::vector<double> x = std::vector<double>(1, 0.);
    std::vector<double> w = std::vector<double>(1, 2.);
    for (size_t i = 0; i < npts; i++)
      for (size_t j = 0; j < npts; j++) {

        fe::QuadData qd;
        qd.d_w = w[i] * w[j];
        qd.d_p = util::Point(x[i], x[j], 0.);
        qd.d_shapes = getShapes(qd.d_p);
        qd.d_derShapes = getDerShapes(qd.d_p);
        qd.d_J = ident_mat;
        qd.d_detJ = 1.;
        d_quads.push_back(qd);
      }
  }

  //
  // second order quad points
  //
  if (d_quadOrder == 2) {
    d_quads.clear();
    // 1-d points are: {-1/sqrt{3], 1/sqrt{3}} and weights are: {1,1}
    int npts = 2;
    std::vector<double> x =
        std::vector<double>{-1. / std::sqrt(3.), 1. / std::sqrt(3.)};
    std::vector<double> w = std::vector<double>{1., 1.};
    for (size_t i = 0; i < npts; i++)
      for (size_t j = 0; j < npts; j++) {

        fe::QuadData qd;
        qd.d_w = w[i] * w[j];
        qd.d_p = util::Point(x[i], x[j], 0.);
        qd.d_shapes = getShapes(qd.d_p);
        qd.d_derShapes = getDerShapes(qd.d_p);
        qd.d_J = ident_mat;
        qd.d_detJ = 1.;
        d_quads.push_back(qd);
      }
  }

  //
  // third order quad points
  //
  if (d_quadOrder == 3) {
    d_quads.clear();
    // 1-d points are: {-sqrt{3}/sqrt{5}, 0, sqrt{3}/sqrt{5}}
    // weights are: {5/9, 8/9, 5/9}
    int npts = 3;
    std::vector<double> x = std::vector<double>{
        -std::sqrt(3.) / std::sqrt(5.), 0., std::sqrt(3.) / std::sqrt(5.)};
    std::vector<double> w = std::vector<double>{5. / 9., 8. / 9., 5. / 9.};
    for (size_t i = 0; i < npts; i++)
      for (size_t j = 0; j < npts; j++) {

        fe::QuadData qd;
        qd.d_w = w[i] * w[j];
        qd.d_p = util::Point(x[i], x[j], 0.);
        qd.d_shapes = getShapes(qd.d_p);
        qd.d_derShapes = getDerShapes(qd.d_p);
        qd.d_J = ident_mat;
        qd.d_detJ = 1.;
        d_quads.push_back(qd);
      }
  }

  //
  // fourth order quad points
  //
  if (d_quadOrder == 4) {
    d_quads.clear();
    int npts = 4;
    std::vector<double> x =
        std::vector<double>{-0.3399810435848563, 0.3399810435848563,
                            -0.8611363115940526, 0.8611363115940526};
    std::vector<double> w =
        std::vector<double>{0.6521451548625461, 0.6521451548625461,
                            0.3478548451374538, 0.3478548451374538};
    for (size_t i = 0; i < npts; i++)
      for (size_t j = 0; j < npts; j++) {

        fe::QuadData qd;
        qd.d_w = w[i] * w[j];
        qd.d_p = util::Point(x[i], x[j], 0.);
        qd.d_shapes = getShapes(qd.d_p);
        qd.d_derShapes = getDerShapes(qd.d_p);
        qd.d_J = ident_mat;
        qd.d_detJ = 1.;
        d_quads.push_back(qd);
      }
  }

  //
  // fifth order quad points
  //
  if (d_quadOrder == 5) {
    d_quads.clear();
    int npts = 5;
    std::vector<double> x =
        std::vector<double>{0., -0.5384693101056831, 0.5384693101056831,
                            -0.9061798459386640, 0.9061798459386640};
    std::vector<double> w = std::vector<double>{
        0.5688888888888889, 0.4786286704993665, 0.4786286704993665,
        0.2369268850561891, 0.2369268850561891};
    for (size_t i = 0; i < npts; i++)
      for (size_t j = 0; j < npts; j++) {

        fe::QuadData qd;
        qd.d_w = w[i] * w[j];
        qd.d_p = util::Point(x[i], x[j], 0.);
        qd.d_shapes = getShapes(qd.d_p);
        qd.d_derShapes = getDerShapes(qd.d_p);
        qd.d_J = ident_mat;
        qd.d_detJ = 1.;
        d_quads.push_back(qd);
      }
  }
}
