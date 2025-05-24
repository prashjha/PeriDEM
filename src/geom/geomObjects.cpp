////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "geomObjects.h"
#include "geomUtilFunctions.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "util/io.h"
#include <vector>
#include <set>

namespace {
  std::string printErrMsg(const std::string &geom_type,
                          const std::vector<double> &params,
                          const std::vector<size_t> &num_params_needed) {

    std::ostringstream oss;

    oss <<  "Error: Number of parameters needed to create geometry = "
        << geom_type << " are "
        << util::io::printStr(num_params_needed, 0)
        << ". But the number of parameters provided are "
        << params.size()
        << " and the parameters are "
        << util::io::printStr(params, 0)
        << ". Exiting.\n";

    return oss.str();
  }
};

//
// Line
//
namespace geom {

    double Line::volume() const {
      return d_L;
    }

    util::Point Line::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Line::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Line::box(const
                                                                  double &tol) const {

      return {d_vertices[0] - tol, d_vertices[1] + tol};
    }

    double Line::inscribedRadius() const {

      return d_r;
    }

    double Line::boundingRadius() const {

      return d_r;
    }

    bool Line::isInside(const util::Point &x) const {

      auto da = (d_vertices[1] - d_vertices[0]) / d_L;
      auto db = x - d_vertices[0];
      double dot = db * da;

      if (util::isLess(dot, 0.) or util::isGreater(dot, d_L))
        return false;


      auto dx = db - dot * da;

      return util::isLess(dx.length(), 1.0E-10);
    }

    bool Line::isOutside(const util::Point &x) const {

      return !isInside(x);
    }

    bool Line::isNear(const util::Point &x,
                                      const double &tol) const {

      auto da = (d_vertices[1] - d_vertices[0]) / d_L;
      auto db = x - d_vertices[0];
      double dot = db * da;

      if (util::isLess(dot, 0.) or util::isGreater(dot, d_L))
        return false;

      auto dx = db - dot * da;

      return util::isLess(dx.length(), tol);
    }

    bool Line::isNearBoundary(const util::Point &x,
                                              const double &tol, const bool
                                              &within) const {

      // check if particle is inside the object
      if (!isNear(x, within ? 0. : tol))
        return false;

      auto da = (d_vertices[1] - d_vertices[0]) / d_L;
      auto db = x - d_vertices[0];
      double dot = db * da;

      if (util::isLess(dot, 0.) or util::isGreater(dot, tol)
          or util::isGreater(dot, d_L) or util::isLess(dot, d_L - tol))
        return false;

      auto dx = db - dot * da;

      return util::isLess(dx.length(), tol);
    }

    bool Line::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Line::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      return false;
    }

    bool Line::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      return true;
    }

    bool Line::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      return true;
    }

    bool Line::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      return false;
    }

    std::string Line::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Line --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Length = " << d_L << std::endl;
      oss << tabS << "Point 1 = " << d_vertices[0].printStr(0, lvl) << std::endl;
      oss << tabS << "Point 2 = " << d_vertices[1].printStr(0, lvl) << std::endl;
      oss << std::endl;

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
} //Line

//
// Triangle
//
namespace geom {
    double Triangle::volume() const {
      return d_r * d_r * (1. + std::sin(M_PI / 3));
    }

    util::Point Triangle::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Triangle::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Triangle::box(const
                                                                      double &tol) const {

      // find lower and upper coordinates
      auto p1 = util::Point(d_x.d_x - d_r, d_x.d_y - d_r, d_x.d_z);
      auto p2 = util::Point(d_x.d_x + d_r, d_x.d_y + d_r, d_x.d_z);

      return {p1 - tol, p2 + tol};
    }

    double Triangle::inscribedRadius() const {

      return d_r * std::sin(M_PI / 3);
    }

    double Triangle::boundingRadius() const {

      return d_r;
    }

    bool Triangle::isInside(const util::Point &x) const {

      if ((x - d_x).length() > d_r)
        return false;

      if ((x - d_x).length() < this->inscribedRadius())
        return true;

      double a = this->volume();
      double a1 =
              std::abs(geom::triangleArea(x, d_vertices[1], d_vertices[2]));
      double a2 =
              std::abs(geom::triangleArea(d_vertices[0], x, d_vertices[2]));
      double a3 =
              std::abs(geom::triangleArea(d_vertices[0], d_vertices[1], x));

      return (a1 + a2 + a3 < a);
    }

    bool Triangle::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Triangle::isNear(const util::Point &x,
                                          const double &tol) const {

      // get a bigger box containing this object
      auto bbox = box(tol);

      return geom::isPointInsideBox(x, 2, bbox);
    }

    bool Triangle::isNearBoundary(const util::Point &x,
                                                  const double &tol, const bool
                                                  &within) const {

      // check if particle is inside the object
      if (!isNear(x, within ? 0. : tol))
        return false;

      double a = this->volume();
      double l = 0.5 * std::sqrt(a);

      double a1 =
              std::abs(geom::triangleArea(x, d_vertices[1], d_vertices[2]));
      if (a1 < tol * l)
        return true;

      double a2 =
              std::abs(geom::triangleArea(d_vertices[0], x, d_vertices[2]));
      if (a2 < tol * l)
        return true;

      double a3 =
              std::abs(geom::triangleArea(d_vertices[0], d_vertices[1], x));
      if (a3 < tol * l)
        return true;

      return false;
    }

    bool Triangle::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Triangle::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(2, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Triangle::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(2, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Triangle::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      return geom::areBoxesNear(this->box(), box, tol, 2);
    }

    bool Triangle::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(2, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Triangle::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Triangle --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
      oss << tabS << "Radius = " << d_r << std::endl;
      oss << tabS << "Vertices = " << util::io::printStr(d_vertices)
          << std::endl;
      oss << std::endl;

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
}// Triangle

//
// Square
//
namespace geom {
  double Square::volume() const {
    return std::pow(d_L, 2);
  }

  util::Point Square::center() const {
    return d_x;
  }

  std::pair<util::Point, util::Point> Square::box() const {

    return box(0.);
  }
  std::pair<util::Point, util::Point> Square::box(const
                                                                     double &tol) const {

    return {util::Point(d_vertices[0].d_x - tol, d_vertices[0].d_y - tol,
                        0.),
            util::Point(d_vertices[2].d_x + tol, d_vertices[2].d_y + tol,
                        0.)};
  }

  double Square::inscribedRadius() const {

    return 0.5*d_L;
  }

  double Square::boundingRadius() const {

    return d_r;
  }

  bool Square::isInside(const util::Point &x) const {
    return geom::isPointInsideRectangle(x, d_vertices[0], d_vertices[2]);
  }

  bool Square::isOutside(const util::Point &x) const {
    return !isInside(x);
  }

  bool Square::isNear(const util::Point &x,
                                         const double &tol) const {

    // get a bigger box containing this object
    auto bbox = box(tol);

    return geom::isPointInsideBox(x, 2, bbox);
  }

  bool Square::isNearBoundary(const util::Point &x,
                                                 const double &tol, const bool
                                                 &within) const {

    // check if particle is inside the object
    if (!isNear(x, within ? 0. : tol))
      return false;

    bool near_x_edge = util::isLess(std::abs(x.d_x - d_vertices[0].d_x), tol) or
                       util::isLess(std::abs(x.d_x - d_vertices[2].d_x), tol);

    bool near_y_edge = util::isLess(std::abs(x.d_y - d_vertices[0].d_y), tol) or
                       util::isLess(std::abs(x.d_y - d_vertices[2].d_y), tol);

    return near_x_edge || near_y_edge;
  }

  bool Square::doesIntersect(const util::Point &x) const {

    return isNearBoundary(x, 1.0E-8, false);
  }

  bool Square::isInside(
          const std::pair<util::Point, util::Point> &box) const {

    for (auto p : geom::getCornerPoints(2, box))
      if(!this->isInside(p))
        return false;

    return true;
  }

  bool Square::isOutside(
          const std::pair<util::Point, util::Point> &box) const {

    bool intersect = false;
    for (auto p : geom::getCornerPoints(2, box))
      if (!intersect)
        intersect = this->isInside(p);

    return !intersect;
  }

  bool Square::isNear(
          const std::pair<util::Point, util::Point> &box, const double &tol) const {

    return geom::areBoxesNear(this->box(), box, tol, 2);
  }

  bool Square::doesIntersect(
          const std::pair<util::Point, util::Point> &box) const {

    // need to check all four corner points
    for (auto p : geom::getCornerPoints(2, box))
      if (this->isInside(p))
        return true;

    return false;
  }

  std::string Square::printStr(int nt, int lvl) const {

    auto tabS = util::io::getTabS(nt);

    std::ostringstream oss;

    oss << tabS << "------- Rectangle --------" << std::endl << std::endl;
    oss << tabS << "Name = " << d_name << std::endl;
    oss << tabS << "Length = " << d_L << std::endl;
    oss << tabS << "Bounding radius = " << d_r << std::endl;
    oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
    oss << tabS << "Vertices = " << util::io::printStr(d_vertices, 0) << std::endl;
    oss << std::endl;

    if (lvl > 0)
      oss << tabS << "Bounding box: " << util::io::printBoxStr(box(0.), nt + 1);

    if (lvl == 0)
      oss << std::endl;

    return oss.str();
  }
}// Square

//
// Rectangle
//
namespace geom {
    double Rectangle::volume() const {
      return d_Lx * d_Ly;
    }

    util::Point Rectangle::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Rectangle::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Rectangle::box(const
                                                                       double &tol) const {

      return {util::Point(d_vertices[0].d_x - tol, d_vertices[0].d_y - tol,
                          0.),
              util::Point(d_vertices[2].d_x + tol, d_vertices[2].d_y + tol,
                          0.)};
    }

    double Rectangle::inscribedRadius() const {

      return util::isLess(d_Lx, d_Ly) ? d_Lx : d_Ly;
    }

    double Rectangle::boundingRadius() const {

      return d_r;
    }

    bool Rectangle::isInside(const util::Point &x) const {
      return geom::isPointInsideRectangle(x, d_vertices[0], d_vertices[2]);
    }

    bool Rectangle::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Rectangle::isNear(const util::Point &x,
                                           const double &tol) const {

      // get a bigger box containing this object
      auto bbox = box(tol);

      return geom::isPointInsideBox(x, 2, bbox);
    }

    bool Rectangle::isNearBoundary(const util::Point &x,
                                                   const double &tol, const bool
                                                   &within) const {

      // check if particle is inside the object
      if (!isNear(x, within ? 0. : tol))
        return false;

      bool near_x_edge = util::isLess(std::abs(x.d_x - d_vertices[0].d_x), tol) or
                         util::isLess(std::abs(x.d_x - d_vertices[2].d_x), tol);

      bool near_y_edge = util::isLess(std::abs(x.d_y - d_vertices[0].d_y), tol) or
                         util::isLess(std::abs(x.d_y - d_vertices[2].d_y), tol);

      return near_x_edge || near_y_edge;
    }

    bool Rectangle::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Rectangle::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(2, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Rectangle::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(2, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Rectangle::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      return geom::areBoxesNear(this->box(), box, tol, 2);
    }

    bool Rectangle::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(2, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Rectangle::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Rectangle --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Lengths (Lx, Ly) = (" << d_Lx << ", " << d_Ly << ")" << std::endl;
      oss << tabS << "Bounding circle radius = " << d_r << std::endl;
      oss << tabS << "Vertices = " << util::io::printStr(d_vertices, 0) << std::endl;
      oss << std::endl;

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
}// Rectangle

//
// Hexagon
//
namespace geom {
    double Hexagon::volume() const {
      // https://en.wikipedia.org/wiki/Hexagon
      double r_small = this->inscribedRadius();
      return 2. * std::sqrt(3.) * r_small * r_small;
    }

    util::Point Hexagon::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Hexagon::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Hexagon::box(const
                                                                     double &tol) const {

      auto p1 = d_x - util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
      auto p2 = d_x + util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
      return {p1, p2};
    }

    double Hexagon::inscribedRadius() const {

      return d_r * 0.5 * std::sqrt(3.);
    }

    double Hexagon::boundingRadius() const {

      return d_r;
    }

    bool Hexagon::isInside(const util::Point &x) const {

      if ((x - d_x).length() > d_r)
        return false;

      if ((x - d_x).length() < inscribedRadius())
        return true;

      return false;
    }

    bool Hexagon::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Hexagon::isNear(const util::Point &x,
                                         const double &tol) const {

      // get a bigger box containing this object
      auto bbox = box(tol);

      return geom::isPointInsideBox(x, 2, bbox);
    }

    bool Hexagon::isNearBoundary(const util::Point &x,
                                                 const double &tol, const bool
                                                 &within) const {

      if ((x - d_x).length() > d_r + tol)
        return false;

      if ((x - d_x).length() < inscribedRadius() - tol)
        return false;

      return true;
    }

    bool Hexagon::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Hexagon::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(2, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Hexagon::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(2, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Hexagon::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      return geom::areBoxesNear(this->box(), box, tol, 2);
    }

    bool Hexagon::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(2, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Hexagon::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Hexagon --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Radius = " << d_r << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
      oss << tabS << "Axis = " << d_a.printStr(0, lvl) << std::endl;
      oss << tabS << "Vertices = " << util::io::printStr(d_vertices, lvl) <<
          std::endl;
      oss << std::endl;

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
}// Hexagon

//
// Drum2D
//
namespace geom {
    double Drum2D::volume() const {

      return (2. * d_r * d_r - d_r * (d_r - 2. * d_w)) * std::sin(M_PI / 3.);
    }

    util::Point Drum2D::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Drum2D::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Drum2D::box(const
                                                                    double &tol) const {

      auto p1 = d_x - util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
      auto p2 = d_x + util::Point(d_r + tol, d_r + tol, d_x[2] + tol);
      return {p1, p2};
    }

    double Drum2D::inscribedRadius() const {

      return d_w;
    }

    double Drum2D::boundingRadius() const {

      return d_r;
    }

    bool Drum2D::isInside(const util::Point &x) const {

      if ((x - d_x).length() > d_r)
        return false;

      if ((x - d_x).length() < inscribedRadius())
        return true;

      // rotate axis to get orthogonal axis
      auto ortho_axis = util::rotate(d_a, M_PI * 0.5, util::Point(0., 0., 1.));

      //
      //                                   + v2
      //                                  /
      //                                /    x
      //                              /
      //                            /
      //                     o----+v1
      //
      auto ox = x - d_x;
      double angle_ox_ov1 = std::acos(std::abs(d_a.dot(ox)) / ox.length());
      double max_length = d_w + angle_ox_ov1 * (d_r - d_w) / (M_PI / 3.);

      return ox.length() <= max_length;
    }

    bool Drum2D::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Drum2D::isNear(const util::Point &x,
                                        const double &tol) const {

      // get a bigger box containing this object
      auto bbox = box(tol);

      return geom::isPointInsideBox(x, 2, bbox);
    }

    bool Drum2D::isNearBoundary(const util::Point &x,
                                                const double &tol, const bool
                                                &within) const {

      if ((x - d_x).length() > d_r + tol)
        return false;

      if ((x - d_x).length() < this->inscribedRadius() - tol)
        return false;

      return true;
    }

    bool Drum2D::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Drum2D::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(2, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Drum2D::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(2, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Drum2D::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      return geom::areBoxesNear(this->box(), box, tol, 2);
    }

    bool Drum2D::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(2, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Drum2D::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Drum2D --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Radius = " << d_r << std::endl;
      oss << tabS << "Neck half-width = " << d_w << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
      oss << tabS << "Axis = " << d_a.printStr(0, lvl) << std::endl;
      oss << tabS << "Vertices = " << util::io::printStr(d_vertices, lvl) <<
          std::endl;
      oss << std::endl;

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
}// Drum2D

//
// Cube
//
namespace geom {
    double Cube::volume() const {
      return std::pow(d_L, 3);
    }

    util::Point Cube::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Cube::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Cube::box(const
                                                                    double &tol) const {
      return {util::Point(d_vertices[0].d_x - tol, d_vertices[0].d_y - tol,
                          d_vertices[0].d_z - tol),
              util::Point(d_vertices[6].d_x + tol, d_vertices[6].d_y + tol,
                          d_vertices[6].d_z + tol)};
    }

    double Cube::inscribedRadius() const {

      return d_L;
    }

    double Cube::boundingRadius() const {

      return d_r;
    }

    bool Cube::isInside(const util::Point &x) const {
      return geom::isPointInsideCuboid(x, d_vertices[0], d_vertices[6]);
    }

    bool Cube::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Cube::isNear(const util::Point &x,
                                        const double &tol) const {

      // get a bigger box containing this object
      auto bbox = box(tol);

      return geom::isPointInsideBox(x, 3, bbox);
    }

    bool Cube::isNearBoundary(const util::Point &x,
                                                const double &tol, const bool
                                                &within) const {

      // check if particle is within the tolerance distance
      if (!isNear(x, within ? 0. : tol))
        return false;

      bool near_x_edge = util::isLess(std::abs(x.d_x - d_vertices[0].d_x), tol) or
                         util::isLess(std::abs(x.d_x - d_vertices[6].d_x), tol);

      bool near_y_edge = util::isLess(std::abs(x.d_y - d_vertices[0].d_y), tol) or
                         util::isLess(std::abs(x.d_y - d_vertices[6].d_y), tol);

      bool near_z_edge = util::isLess(std::abs(x.d_z - d_vertices[0].d_z), tol) or
                         util::isLess(std::abs(x.d_z - d_vertices[6].d_z), tol);

      return near_x_edge || near_y_edge || near_z_edge;
    }

    bool Cube::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Cube::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(3, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Cube::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(3, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Cube::isNear(
            const std::pair<util::Point, util::Point> &bbox, const double &tol)
    const {

      return geom::areBoxesNear(box(), bbox, tol, 3);
    }

    bool Cube::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(3, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Cube::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Cube --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Length = " << d_L << std::endl;
      oss << tabS << "Bounding sphere radius = " << d_r << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, 0) << std::endl;
      oss << tabS << "Vertices = " << util::io::printStr(d_vertices, 0) << std::endl;
      oss << std::endl;

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
}// Cube

//
// Cuboid
//
namespace geom {
    double Cuboid::volume() const {
      return d_Lx * d_Ly * d_Lz;
    }

    util::Point Cuboid::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Cuboid::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Cuboid::box(const
                                                                    double &tol) const {
      return {util::Point(d_vertices[0].d_x - tol, d_vertices[0].d_y - tol,
                          d_vertices[0].d_z - tol),
              util::Point(d_vertices[6].d_x + tol, d_vertices[6].d_y + tol,
                          d_vertices[6].d_z + tol)};
    }

    double Cuboid::inscribedRadius() const {

      auto l = util::isLess(d_Lx, d_Ly) ? d_Lx : d_Ly;
      return util::isLess(l, d_Lz) ? l : d_Lz;
    }

    double Cuboid::boundingRadius() const {

      return d_r;
    }

    bool Cuboid::isInside(const util::Point &x) const {
      return geom::isPointInsideCuboid(x, d_vertices[0], d_vertices[6]);
    }

    bool Cuboid::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Cuboid::isNear(const util::Point &x,
                                        const double &tol) const {

      // get a bigger box containing this object
      auto bbox = box(tol);

      return geom::isPointInsideBox(x, 3, bbox);
    }

    bool Cuboid::isNearBoundary(const util::Point &x,
                                                const double &tol, const bool
                                                &within) const {

      // check if particle is within the tolerance distance
      if (!isNear(x, within ? 0. : tol))
        return false;

      bool near_x_edge = util::isLess(std::abs(x.d_x - d_vertices[0].d_x), tol) or
                         util::isLess(std::abs(x.d_x - d_vertices[6].d_x), tol);

      bool near_y_edge = util::isLess(std::abs(x.d_y - d_vertices[0].d_y), tol) or
                         util::isLess(std::abs(x.d_y - d_vertices[6].d_y), tol);

      bool near_z_edge = util::isLess(std::abs(x.d_z - d_vertices[0].d_z), tol) or
                         util::isLess(std::abs(x.d_z - d_vertices[6].d_z), tol);

      return near_x_edge || near_y_edge || near_z_edge;
    }

    bool Cuboid::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Cuboid::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(3, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Cuboid::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(3, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Cuboid::isNear(
            const std::pair<util::Point, util::Point> &bbox, const double &tol)
    const {

      return geom::areBoxesNear(box(), bbox, tol, 3);
    }

    bool Cuboid::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(3, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Cuboid::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Cuboid --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Lengths (Lx, Ly, Lz) = "
                  << util::io::printStr(std::vector<double>{d_Lx, d_Ly, d_Lz}, 0)
                  << std::endl;
      oss << tabS << "Bounding sphere radius = " << d_r << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, 0) << std::endl;
      oss << tabS << "Vertices = " << util::io::printStr(d_vertices, 0) << std::endl;
      oss << std::endl;

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
}// Cuboid

//
// Circle
//
namespace geom {
    double Circle::volume() const {
      return M_PI * d_r * d_r;
    }

    util::Point Circle::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Circle::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Circle::box(const
                                                                    double &tol) const {
      double r = d_r + tol;
      return {
              util::Point(d_x.d_x - r, d_x.d_y - r, 0.),
              util::Point(d_x.d_x + r, d_x.d_y + r, 0.)
      };
    }

    double Circle::inscribedRadius() const {

      return d_r;
    }

    double Circle::boundingRadius() const {

      return d_r;
    }

    bool Circle::isInside(const util::Point &x) const {

      return util::isLess(d_x.dist(x), d_r + 1.0E-12);
    }

    bool Circle::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Circle::isNear(const util::Point &x,
                                        const double &tol) const {

      // translate to origin
      auto x0 = x - d_x;

      return util::isLess(x0.length(), d_r + tol);
    }

    bool Circle::isNearBoundary(const util::Point &x,
                                                const double &tol, const bool
                                                &within) const {

      // check if particle is within the tolerance distance
      if (!isNear(x, within ? 0. : tol))
        return false;

      // check if it is close enough to circumference
      auto x0 = x - d_x;

      return util::isLess(x0.length(), d_r + tol) ||
             util::isLess(x0.length(), d_r - tol);
    }

    bool Circle::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Circle::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(2, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Circle::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(2, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Circle::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      if (this->isInside(box))
        return true;

      // get corner points of box
      auto cp = geom::getCornerPoints(2, box);

      for (auto p: cp) {

        // check the distance of corner point with the center
        auto dx = p - d_x;
        if (util::isLess(dx.length(), d_r + tol))
          return true;
      }

      // check center to center distance
      auto dxc = geom::getCenter(2, box) - d_x;

      // check wrt inscribed circle
      auto r = geom::inscribedRadiusInBox(2, box);
      if (util::isLess(dxc.length(), d_r + r + tol))
        return true;

      // check wrt circumscribed circle
      r = geom::circumscribedRadiusInBox(2, box);
      return util::isLess(dxc.length(), d_r + r + tol);
    }

    bool Circle::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(2, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Circle::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Circle --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
      oss << tabS << "Radius = " << d_r << std::endl;

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }
}// Circle

//
// Cylinder
//
namespace geom {
    double Cylinder::volume() const {
      return M_PI * d_r * d_r * d_l;
    }

    util::Point Cylinder::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Cylinder::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point>
    Cylinder::box(const double &tol) const {

      if (d_xa.length() < 1.0E-10)
        return {util::Point(), util::Point()};

      auto xb = d_xBegin - tol * d_xa;
      auto xt = d_xBegin + (d_l + tol) * d_xa;

      double r = d_r + tol;

      return {xb - r, xt + r};
    }

    double Cylinder::inscribedRadius() const {

      auto box = this->box();

      return 0.5 * (box.second - box.first).length();
    }

    double Cylinder::boundingRadius() const {

      return 0.5 * std::sqrt(d_l * d_l + 4. * d_r * d_r);
    }

    bool Cylinder::isInside(const util::Point &x) const {

      auto dx = x - d_xBegin;

      if (dx.length() < 1.0E-10)
        return true;

      double dx_dot_xa = dx * d_xa;
      if (util::isLess(dx_dot_xa, 0.) or
          util::isGreater(dx_dot_xa, d_l))
        return false;
      else {

        // project dx onto cross-section plane of cylinder
        auto dx_project = dx - dx_dot_xa * d_xa;

        return !util::isGreater(dx_project.length(), d_r + 1.0E-12);
      }
    }

    bool Cylinder::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Cylinder::isNear(const util::Point &x, const double
    &tol) const {

      auto dx = x - d_xBegin;

      if (dx.length() < tol)
        return true;

      double dx_dot_xa = dx * d_xa;
      if (util::isLess(dx_dot_xa, -tol) or
          util::isGreater(dx_dot_xa, d_l + tol))
        return false;
      else {

        // project dx onto cross-section plane of cylinder
        auto dx_project = dx - dx_dot_xa * d_xa;

        return !util::isGreater(dx_project.length(), d_r + tol);
      }
    }

    bool Cylinder::isNearBoundary(const util::Point &x,
                                                  const double &tol, const bool
                                                  &within) const {

      auto dx = x - d_xBegin;

      if (dx.length() < tol)
        return true;

      double dx_dot_xa = dx * d_xa;
      if (util::isLess(dx_dot_xa, -tol) or
          util::isGreater(dx_dot_xa, tol) or
          util::isGreater(dx_dot_xa, d_l + tol) or
          util::isLess(dx_dot_xa, d_l - tol))
        return false;
      else {

        // project dx onto cross-section plane of cylinder
        auto dx_project = dx - dx_dot_xa * d_xa;

        return !(util::isLess(dx_project.length(), d_r - tol) or
                 util::isGreater(dx_project.length(), d_r + tol));
      }
    }

    bool Cylinder::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Cylinder::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(3, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Cylinder::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(3, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Cylinder::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      return geom::areBoxesNear(this->box(), box, tol, 3);
    }

    bool Cylinder::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(3, box))
        if (this->isInside(p))
          return true;

      return false;
    }


    std::string Cylinder::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Cylinder --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Center = " << d_xBegin.printStr(0, lvl) << std::endl;
      oss << tabS << "Axis = " << d_xa.printStr(0, lvl) << std::endl;
      oss << tabS << "Radius = " << d_r << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, 0) << std::endl;

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }

}// Cylinder

//
// Sphere
//
namespace geom {
    double Sphere::volume() const {
      return 4. * M_PI * d_r * d_r * d_r / 3.;
    }

    util::Point Sphere::center() const {
      return d_x;
    }

    std::pair<util::Point, util::Point> Sphere::box() const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> Sphere::box(const
                                                                    double &tol) const {
      double r = d_r + tol;

      return {
              util::Point(d_x.d_x - r, d_x.d_y - r, d_x.d_z - r),
              util::Point(d_x.d_x + r, d_x.d_y + r, d_x.d_z + r)
      };
    }

    double Sphere::inscribedRadius() const {

      return d_r;
    }

    double Sphere::boundingRadius() const {

      return d_r;
    }

    bool Sphere::isInside(const util::Point &x) const {

      return util::isLess(d_x.dist(x), d_r + 1.0E-12);
    }

    bool Sphere::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool Sphere::isNear(const util::Point &x,
                                        const double &tol) const {

      // translate to origin
      auto x0 = x - d_x;

      return util::isLess(x0.length(), d_r + tol);
    }

    bool Sphere::isNearBoundary(const util::Point &x,
                                                const double &tol, const bool
                                                &within) const {

      // check if particle is within the tolerance distance
      if (!isNear(x, within ? 0. : tol))
        return false;

      // check if it is close enough to circumference
      auto x0 = x - d_x;

      return util::isLess(x0.length(), d_r + tol) ||
             util::isLess(x0.length(), d_r - tol);
    }

    bool Sphere::doesIntersect(const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool Sphere::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(3, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool Sphere::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(3, box))
        if (!intersect)
          intersect = this->isInside(p);

      return !intersect;
    }

    bool Sphere::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      if (this->isInside(box))
        return true;

      // get corner points of box
      auto cp = geom::getCornerPoints(3, box);

      for (auto p: cp) {

        // check the distance of corner point with the center
        auto dx = p - d_x;
        if (util::isLess(dx.length(), d_r + tol))
          return true;
      }

      // check center to center distance
      auto dxc = geom::getCenter(3, box) - d_x;

      // check wrt inscribed circle
      auto r = geom::inscribedRadiusInBox(3, box);
      if (util::isLess(dxc.length(), d_r + r + tol))
        return true;

      // check wrt circumscribed circle
      r = geom::circumscribedRadiusInBox(3, box);
      return util::isLess(dxc.length(), d_r + r + tol);
    }

    bool Sphere::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(3, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string Sphere::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- Sphere --------" << std::endl << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Center = " << d_x.printStr(0, lvl) << std::endl;
      oss << tabS << "Radius = " << d_r << std::endl;

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }

}// Sphere

