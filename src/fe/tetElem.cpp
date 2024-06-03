/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "tetElem.h"
#include "util/function.h"
#include "util/matrix.h"
#include <iostream>
#include <util/io.h>

#include "util/feElementDefs.h"  // global definition of elements

namespace {

void checkPoint(const std::vector<double> &p, const std::vector<util::Point> &nodes) {

  // check to see if p is in reference tet element
  bool check = false;
  if (util::isLess(p[0], -1.0E-5) || util::isLess(p[1], -1.0E-5) || util::isLess(p[2], -1.0E-5) ||
      util::isGreater(p[0], 1. + 1.0E-5) ||
      util::isGreater(p[1], 1. + 1.0E-5) ||
      util::isGreater(p[2], 1. + 1.0E-5)) {

    check = true;
  }

  if (!check) {

    // check if projection of point in x, y, z plane is within the limit
    if (util::isGreater(p[0], 1. + 1.0E-5 - p[1]))
      check = true;

    if (util::isGreater(p[1], 1. + 1.0E-5 - p[2]))
      check = true;

    if (util::isGreater(p[2], 1. + 1.0E-5 - p[0]))
      check = true;
  }

  if (check) {
    std::cerr << "Error: Point p = ("
              << p[0] << ", " << p[1] << ", " << p[2]
              << ") does not belong to reference tet element = {("
              << nodes[0].d_x << ", " << nodes[0].d_y << ", " << nodes[0].d_z
              << "), ("
              << nodes[1].d_x << "," << nodes[1].d_y << ", " << nodes[1].d_z
              << "), ("
              << nodes[2].d_x << "," << nodes[2].d_y << ", " << nodes[2].d_z
              << "), ("
              << nodes[3].d_x << "," << nodes[3].d_y << ", " << nodes[3].d_z
              << ")}.\n"
              << "Coordinates in reference element are: "
              << "xi = " << p[0]
              << ", eta = " << p[1]
              << ", zeta = " << p[2] << "\n";
    exit(1);
  }
}

} // anonymous namespace

fe::TetElem::TetElem(size_t order)
    : fe::BaseElem(order, util::vtk_type_triangle) {

  if (d_quadOrder > 3) {
    std::cout << "Error: For linear tet element, we only support upto 3 quad "
                 "order approximation.\n";
    exit(1);
  }

  // compute quad data
  this->init();
}

double fe::TetElem::elemSize(const std::vector<util::Point> &nodes) {
  // volume of tet element is (1/6) a * (b x c),
  // where a = v2 - v1, b = v3 - v1, c = v4 - v1
  auto a = nodes[1] - nodes[0];
  auto b = nodes[2] - nodes[0];
  auto c = nodes[3] - nodes[0];
  return (1. / 6.) * a * (b.cross(c));
}

std::vector<double> fe::TetElem::getShapes(
    const util::Point &p, const std::vector<util::Point> &nodes) {
  return getShapes(mapPointToRefElem(p, nodes));
}

std::vector<std::vector<double>> fe::TetElem::getDerShapes(
    const util::Point &p, const std::vector<util::Point> &nodes) {

  // get derivatives of shape function in reference tet element
  auto ders_ref = getDerShapes(mapPointToRefElem(p, nodes));

  // get Jacobian and its determinant
  std::vector<std::vector<double>> J;
  auto detJ = getJacobian(p, nodes, &J);

  auto J_inv = util::inv(J);

  // to hold derivatives
  std::vector<std::vector<double>> ders(ders_ref.size(),
                                        std::vector<double>(3, 0.));

  // grad N_i = J_inv * grad N_i^ref
  for (size_t i = 0; i < 4; i++)
    ders[i] = util::dot(J_inv, ders_ref[i]);

  return ders;
}

std::vector<fe::QuadData> fe::TetElem::getQuadDatas(
    const std::vector<util::Point> &nodes) {

  // copy quad data associated to reference element
  auto qds = d_quads;

  // modify data
  for (auto &qd : qds) {

    // get Jacobian and determinant
    qd.d_detJ = getJacobian(qd.d_p, nodes, &(qd.d_J));

    // transform quad weight
    qd.d_w *= qd.d_detJ;

    // map point to triangle
    qd.d_p = util::Point();
    for (size_t i=0; i<4; i++) {
      qd.d_p.d_x += qd.d_shapes[i] * nodes[i].d_x;
      qd.d_p.d_y += qd.d_shapes[i] * nodes[i].d_y;
      qd.d_p.d_z += qd.d_shapes[i] * nodes[i].d_z;
    }

    // get inverse of Jacobian
    auto J_inv = util::inv(qd.d_J);

    // to hold derivatives
    std::vector<std::vector<double>> ders(qd.d_derShapes.size(),
                                          std::vector<double>(3, 0.));

    // grad N_i = J_inv * grad N_i^ref
    for (size_t i = 0; i < 4; i++)
      ders[i] = util::dot(J_inv, qd.d_derShapes[i]);

    qd.d_derShapes = ders;
  }

  return qds;
}

std::vector<fe::QuadData> fe::TetElem::getQuadPoints(
    const std::vector<util::Point> &nodes) {
  // copy quad data associated to reference element
  auto qds = d_quads;

  // modify data
  for (auto &qd : qds) {

    // transform quad weight
    qd.d_w *= getJacobian(qd.d_p, nodes, nullptr);

    // map point to triangle
    qd.d_p = util::Point();
    for (size_t i=0; i<4; i++) {
      qd.d_p.d_x += qd.d_shapes[i] * nodes[i].d_x;
      qd.d_p.d_y += qd.d_shapes[i] * nodes[i].d_y;
      qd.d_p.d_z += qd.d_shapes[i] * nodes[i].d_z;
    }
  }

  return qds;
}

std::vector<double> fe::TetElem::getShapes(const util::Point &p) {
  // N1 = 1 - xi - eta - zeta, N2 = xi, N3 = eta, N4 = zeta
  return std::vector<double>{1. - p.d_x - p.d_y - p.d_z, p.d_x, p.d_y, p.d_z};
}

std::vector<std::vector<double>> fe::TetElem::getDerShapes(
    const util::Point &p) {

  // d N1/d xi = -1, d N1/d eta = -1, d N1/d zeta = -1,
  // d N2/ d xi = 1, d N2/d eta = 0, d N2/d zeta = 0,
  // d N3/ d xi = 0, d N3/d eta = 1, d N3/d zeta = 0,
  // d N4/ d xi = 0, d N4/d eta = 0, d N4/d zeta = 1,
  std::vector<std::vector<double>> r;
  r.push_back(std::vector<double>{-1., -1., -1.});
  r.push_back(std::vector<double>{1., 0., 0.});
  r.push_back(std::vector<double>{0., 1., 0.});
  r.push_back(std::vector<double>{0., 0., 1.});

  return r;
}

util::Point
fe::TetElem::mapPointToRefElem(
    const util::Point &p, const std::vector<util::Point> &nodes) {

  // get Jacobian matrix and compute its transpose
  std::vector<std::vector<double>> J(3, std::vector<double>(3, 0.));
  auto detJ = getJacobian(p, nodes, &J);

  // get transpose of Jacobian
  auto B = util::transpose(J);
  auto detB = detJ;

  // get inverse of B
  auto B_inv = util::inv(B);

  // get vector from first vertex to point p
  std::vector<double> vec_p = {p.d_x - nodes[0].d_x,
                               p.d_y - nodes[0].d_y,
                               p.d_z - nodes[0].d_z};
  // multiply B_inv to vector to transform point
  auto p_ref = util::dot(B_inv, vec_p);

  // check point
  checkPoint(p_ref, nodes);

  if (util::isLess(p_ref[0], 0.)) p_ref[0] = 0.;
  if (util::isLess(p_ref[1], 0.)) p_ref[1] = 0.;
  if (util::isLess(p_ref[2], 0.)) p_ref[2] = 0.;
  if (util::isGreater(p_ref[0], 1.)) p_ref[0] = 1.;
  if (util::isGreater(p_ref[1], 1.)) p_ref[1] = 1.;
  if (util::isGreater(p_ref[2], 1.)) p_ref[2] = 1.;

  return util::Point(p_ref);
}

double fe::TetElem::getJacobian(const util::Point &p,
                                const std::vector<util::Point> &nodes,
                                std::vector<std::vector<double>> *J) {
  if (J != nullptr) {
    J->resize(3);
    (*J)[0] = std::vector<double>{nodes[1].d_x - nodes[0].d_x,
                                  nodes[1].d_y - nodes[0].d_y,
                                  nodes[1].d_z - nodes[0].d_z};
    (*J)[1] = std::vector<double>{nodes[2].d_x - nodes[0].d_x,
                                  nodes[2].d_y - nodes[0].d_y,
                                  nodes[2].d_z - nodes[0].d_z};
    (*J)[2] = std::vector<double>{nodes[3].d_x - nodes[0].d_x,
                                  nodes[3].d_y - nodes[0].d_y,
                                  nodes[3].d_z - nodes[0].d_z};

    return util::det(*J);
  } else {
    std::vector<std::vector<double>> J_local;
    J_local.resize(3);
    J_local[0] = std::vector<double>{nodes[1].d_x - nodes[0].d_x,
                                  nodes[1].d_y - nodes[0].d_y,
                                  nodes[1].d_z - nodes[0].d_z};
    J_local[1] = std::vector<double>{nodes[2].d_x - nodes[0].d_x,
                                  nodes[2].d_y - nodes[0].d_y,
                                  nodes[2].d_z - nodes[0].d_z};
    J_local[2] = std::vector<double>{nodes[3].d_x - nodes[0].d_x,
                                  nodes[3].d_y - nodes[0].d_y,
                                  nodes[3].d_z - nodes[0].d_z};

    return util::det(J_local);
  }
}

void fe::TetElem::init() {
  //
  // compute quad data for reference triangle with vertex at
  // (0,0), (1,0), (0,1)
  //

  if (!d_quads.empty()) return;

  // no point in zeroth order
  if (d_quadOrder == 0) {
    d_quads.resize(0);
  }

  // 3x3 identity matrix
  std::vector<std::vector<double>> ident_mat;
  ident_mat.push_back(std::vector<double>{1., 0., 0.});
  ident_mat.push_back(std::vector<double>{0., 1., 0.});
  ident_mat.push_back(std::vector<double>{0., 0., 1.});

  //
  // These datas are from LibMesh code
  // See: https://libmesh.github.io/doxygen/quadrature__gauss__3D_8C_source.html

  //
  // first order quad points for triangle
  //
  if (d_quadOrder == 1) {
    d_quads.clear();
    fe::QuadData qd;
    qd.d_w = 1. / 6.;
    qd.d_p = util::Point(1. / 4., 1. / 4., 1. / 4.);
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

    double w = 1. / 24.;
    double a = 0.585410196624969;
    double b = 0.138196601125011;
    // point 1
    qd.d_w = w;
    qd.d_p = util::Point(a, b, b);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 2
    qd.d_w = w;
    qd.d_p = util::Point(b, a, b);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 3
    qd.d_w = w;
    qd.d_p = util::Point(b, b, a);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 4
    qd.d_w = w;
    qd.d_p = util::Point(b, b, b);
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

    double w1 = -2. / 15.;
    double w2 = 0.075;

    double a = 0.25;
    double b = 0.5;
    double c = 1. / 6.;

    // point 1
    qd.d_w = w1;
    qd.d_p = util::Point(a, a, a);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 2
    qd.d_w = w2;
    qd.d_p = util::Point(b, c, c);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 3
    qd.d_w = w2;
    qd.d_p = util::Point(c, b, c);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 4
    qd.d_w = w2;
    qd.d_p = util::Point(c, c, b);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
    // point 5
    qd.d_w = w2;
    qd.d_p = util::Point(c, c, c);
    qd.d_shapes = getShapes(qd.d_p);
    qd.d_derShapes = getDerShapes(qd.d_p);
    qd.d_J = ident_mat;
    qd.d_detJ = 1.;
    d_quads.push_back(qd);
  }
}