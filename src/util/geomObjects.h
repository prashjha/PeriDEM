/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef UTIL_GEOMETRY_OBJECTS_H
#define UTIL_GEOMETRY_OBJECTS_H

#include <iostream>
#include <utility>
#include <memory>

#include "function.h"
#include "io.h"
#include "point.h" // definition of Point
#include "transformation.h"

namespace util {

/*! @brief Provides geometrical methods such as point inside rectangle */
namespace geometry {

/*!
 * @brief Defines simple rectangle domain
 */
struct BoxPartition {

  util::Point d_xc; // centroid of box
  std::pair<util::Point, util::Point> d_box; // two corner points
  double d_r; // radius of circle inscribing box
  std::vector<size_t> d_nodes; // id of nodes in this box

  BoxPartition() : d_xc(util::Point()),
                   d_box({util::Point(), util::Point()}),
                   d_r(0.) {}

  bool isNear(const BoxPartition &box, const double &tol) const {
    auto dx = d_xc - box.d_xc;
    return dx.length() < d_r + box.d_r + tol;
  }

  bool isNear(const util::Point &x, const double &tol) const {

    if (util::isLess(x.d_x, d_box.first.d_x - tol) or
        util::isLess(x.d_y, d_box.first.d_y - tol) or
        util::isLess(x.d_z, d_box.first.d_z - tol) or
        util::isGreater(x.d_x, d_box.second.d_x + tol) or
        util::isGreater(x.d_y, d_box.second.d_y + tol) or
        util::isGreater(x.d_z, d_box.second.d_z + tol))
      return false;

    return true;
  }

  void addNode(const size_t &i) {
    for (const auto &j : d_nodes)
      if (j == i)
        return;

    d_nodes.push_back(i);
  }
};

/*!
 * @brief Defines abstract geometrical domain
 */
class GeomObject {

public:
  /*!
   * @brief Constructor
   * @param name Name of the geometric object
   */
  explicit GeomObject(std::string name = "") : d_name(std::move(name)){};

  /*!
   * @brief Computes the volume (area in 2d, length in 1d) of object
   * @return Volume Volume of object
   */
  virtual double volume() const { return 0.; };

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  virtual util::Point center() const { return {}; };

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  virtual std::pair<util::Point, util::Point> box() const {
    return {util::Point(), util::Point()};
  };

  /*!
   * @brief Computes the bounding box of object
   * @param tol Tolerance/padding used in creating bounding box
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  virtual std::pair<util::Point, util::Point> box(const double &tol) const {
    return {util::Point(), util::Point()};
  };

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   */
  virtual double inscribedRadius() const { return 0.; };

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  virtual double boundingRadius() const { return 0.; };

  /*!
   * @brief Checks if point is inside this object
   * @param x Point
   * @return True If point lies inside
   */
  virtual bool isInside(const util::Point &x) const { return false; };

  /*!
   * @brief Checks if point is outside of this object
   * @param x Point
   * @return True If point lies outside
   */
  virtual bool isOutside(const util::Point &x) const { return false; };

  /*!
   * @brief Checks if point is within given distance of this object
   * @param x Point
   * @param tol Tolerance used in checking the nearness
   * @return True True if within the tol distance
   */
  virtual bool isNear(const util::Point &x, const double &tol) const {
    return false;
  };

  /*!
   * @brief Checks if point is within given distance of this object
   * @param x Point
   * @param tol Tolerance used in checking the nearness
   * @param within Check if the point is within (inside) the object
   * @return True True if it is near within tol distance
   */
  virtual bool isNearBoundary(const util::Point &x, const double &tol,
                              const bool &within) const {
    return false;
  };

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @param x Point
   * @return True True if it lies on the boundary
   */
  virtual bool doesIntersect(const util::Point &x) const { return false; };

  /*!
   * @brief Checks if box is completely inside
   * @param box Box
   * @return True True if box lies inside
   */
  virtual bool
  isInside(const std::pair<util::Point, util::Point> &box) const {
    return false;
  };

  /*!
   * @brief Checks if box is outside of the object
   * @param box Box
   * @return True True if box lies outside
   */
  virtual bool
  isOutside(const std::pair<util::Point, util::Point> &box) const {
    return false;
  };

  /*!
   * @brief Checks if box is within given distance of this object
   * @param box Box
   * @param tol Tolerance used in checking the nearness
   * @return True True if box is inside within the tol distance
   */
  virtual bool isNear(const std::pair<util::Point, util::Point> &box,
                      const double &tol) const {
    return false;
  };

  /*!
   * @brief Checks if box intersects this object
   * @param box Box
   * @return True True if intersects
   */
  virtual bool
  doesIntersect(const std::pair<util::Point, util::Point> &box) const {
    return false;
  };

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  virtual std::string printStr(int nt, int lvl) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;

    oss << tabS << "------- GeomObject --------" << std::endl << std::endl;
    oss << tabS << "Base geometrical object" << std::endl;
    oss << tabS << "Base implementation of GeomObject." << std::endl;

    return oss.str();
  };

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  virtual void print(int nt, int lvl) const { std::cout << printStr(nt, lvl); };

  /*! @brief Prints the information about the object */
  virtual void print() const { print(0, 0); };

public:
  /*! @brief name of object */
  const std::string d_name;
};

/*!
 * @brief Defines line in 3d
 */
class Line : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Line() : GeomObject("line"), d_x1(util::Point()), d_x2(util::Point()){};

  /*!
   * @brief Constructor
   *
   * @param x1 Left-bottom corner point
   * @param x2 Right-top corner point
   */
  Line(util::Point x1, util::Point x2)
      : GeomObject("line"), d_x1(x1), d_x2(x2){};

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief left-bottom corner point */
  util::Point d_x1;

  /*! @brief right-top corner point */
  util::Point d_x2;
};

/*!
 * @brief Defines rectangle
 */
class Triangle : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Triangle()
      : GeomObject("triangle"), d_r(0.), d_a(util::Point(1., 0., 0.)),
        d_x(util::Point()),
        d_vertices(std::vector<util::Point>(3, util::Point())){};

  /*!
   * @brief Constructor
   *
   * @param x1 Left-bottom corner point
   * @param x2 Right-top corner point
   */
  Triangle(double r, util::Point x = util::Point(0., 0., 0.),
          util::Point a = util::Point(1., 0., 0.))
      : GeomObject("triangle"), d_r(r), d_a(a),
        d_x(x),
        d_vertices(std::vector<util::Point>(3, util::Point())){

    // generate vertices
    auto rotate_axis = util::Point(0., 0., 1.); // z-axis
    for (int i = 0; i < 3; i++) {
      d_vertices[i] = d_x + d_r * util::rotate(
          d_a, i * 2. * M_PI / 3., rotate_axis);
    }
  };

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   *
   * @note TODO
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   *
   * @note TODO
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief vertices */
  std::vector<util::Point> d_vertices;

  /*! @brief center */
  util::Point d_x;

  /*! @brief distance between center and the farthest vertex of triang;e */
  double d_r;

  /*! @brief Axis: defined as the vector pointing from center to the first
   * vertex
   *                       v2
   *                        +
   *
   *
   *                                o           +
   *                                x            v1
   *
   *                        +
   *                        v3
   *
   * Axis is a vector from x to v1
   */
  util::Point d_a;
};

/*!
 * @brief Defines rectangle
 */
class Rectangle : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Rectangle()
      : GeomObject("rectangle"), d_x1(util::Point()), d_x2(util::Point()){};

  /*!
   * @brief Constructor
   *
   * @param x1 Left-bottom corner point
   * @param x2 Right-top corner point
   */
  Rectangle(util::Point x1, util::Point x2)
      : GeomObject("rectangle"), d_x1(x1), d_x2(x2){};

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief left-bottom corner point */
  util::Point d_x1;

  /*! @brief right-top corner point */
  util::Point d_x2;
};

/*!
 * @brief Defines rectangle
 */
class Hexagon : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Hexagon()
      : GeomObject("hexagon"), d_r(0.), d_a(util::Point(1., 0., 0.)),
        d_x(util::Point()),
        d_vertices(std::vector<util::Point>(6, util::Point())){};

  /*!
   * @brief Constructor
   *
   * @param x1 Left-bottom corner point
   * @param x2 Right-top corner point
   */
  Hexagon(double r, util::Point x = util::Point(0., 0., 0.),
          util::Point a = util::Point(1., 0., 0.))
      : GeomObject("hexagon"), d_r(r), d_a(a),
        d_x(x),
        d_vertices(std::vector<util::Point>(6, util::Point())){

            // generate vertices
            auto rotate_axis = util::Point(0., 0., 1.); // z-axis
            for (int i = 0; i < 6; i++) {
              d_vertices[i] = d_x + d_r * util::rotate(
                                              d_a, i * M_PI / 3., rotate_axis);
            }
  };

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   *
   * @note TODO Robust calculation needed
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief vertices */
  std::vector<util::Point> d_vertices;

  /*! @brief center */
  util::Point d_x;

  /*! @brief distance between center and the farthest vertex of hexagon */
  double d_r;

  /*! @brief Axis: defined as the vector pointing from center to the first
   * vertex
   *
   *                        +              +
   *
   *
   *                    +           o           +
   *                                x            v1
   *
   *                        +              +
   *
   * Axis is a vector from x to v1
   */
  util::Point d_a;
};

/*!
 * @brief Defines rectangle
 */
class Drum2D : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Drum2D()
      : GeomObject("drum2d"), d_r(0.), d_w(0.), d_a(util::Point(1., 0., 0.)),
        d_x(util::Point()),
        d_vertices(std::vector<util::Point>(6, util::Point())){};

  /*!
   * @brief Constructor
   *
   * @param x1 Left-bottom corner point
   * @param x2 Right-top corner point
   */
  Drum2D(double r, double w, util::Point x = util::Point(0., 0., 0.),
           util::Point a = util::Point(1., 0., 0.))
      : GeomObject("drum2d"), d_r(r), d_w(w), d_a(a),
        d_x(x),
        d_vertices(std::vector<util::Point>(6, util::Point())){

    // generate vertices
    auto rotate_axis = util::Point(0., 0., 1.); // z-axis

    d_vertices[0] = d_x + d_w*d_a;
    d_vertices[3] = d_x - d_w*d_a;

    d_vertices[1] = d_x + d_r * util::rotate(d_a, M_PI/3.,
                                                             rotate_axis);
    d_vertices[2] = d_vertices[1] - d_r * d_a;

    d_vertices[4] = d_x + d_r * util::rotate(d_a, -M_PI/3.,
                                                             rotate_axis);
    d_vertices[5] = d_vertices[4] - d_r * d_a;
  };

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   *
   * @note TODO
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   *
   * @note TODO
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   *
   * @note TODO make this precise
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   *
   * @note TODO make this precise
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief vertices */
  std::vector<util::Point> d_vertices;

  /*! @brief center */
  util::Point d_x;

  /*! @brief half width of neck */
  double d_w;

  /*! @brief distance between center and the farthest vertex of hexagon */
  double d_r;

  /*! @brief Axis: defined as the vector pointing from center to the first
   * vertex
   *             v3                                v2
   *               +                               +
   *
   *
   *                      +         o           +
   *                     v4         x            v1
   *
   *               +                                +
   *               v5                               v6
   *
   * Axis is a vector from x to v1
   */
  util::Point d_a;
};

/*!
 * @brief Defines cuboid
 */
class Cuboid : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Cuboid() : GeomObject("cuboid"), d_x1(util::Point()), d_x2(util::Point()){};

  /*!
   * @brief Constructor
   *
   * @param x1 Left-bottom-back corner point
   * @param x2 Right-top-front corner point
   */
  Cuboid(util::Point x1, util::Point x2)
      : GeomObject("cuboid"), d_x1(x1), d_x2(x2){};

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief left-bottom-back corner point */
  util::Point d_x1;

  /*! @brief right-top-front corner point */
  util::Point d_x2;
};

/*!
 * @brief Defines rectangle
 */
class Circle : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Circle() : GeomObject("circle"), d_xc(util::Point()), d_r(0.){};

  /*!
   * @brief Constructor
   *
   * @param r Radius
   * @param xc Center
   */
  Circle(double r, util::Point xc) : GeomObject("circle"), d_xc(xc), d_r(r){};

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief center point */
  util::Point d_xc;

  /*! @brief radius */
  double d_r;
};

/*!
 * @brief Defines rectangle
 */
class Cylinder : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Cylinder()
      : GeomObject("cylinder"), d_xBegin(util::Point()),
        d_xa(util::Point(1., 1., 1.)), d_r(0.), d_l(0.){};

  /*!
   * @brief Constructor
   *
   * @param r Radius
   * @param xc Center
   * @param xa Axis of cylinder
   */
  Cylinder(double r, double l, util::Point x_begin, util::Point xa)
      : GeomObject("cylinder"), d_xBegin(x_begin), d_xa(xa / xa.length()),
        d_r(r), d_l(r){};

  /*!
   * @brief Constructor
   *
   * @param r Radius
   * @param xc Center
   * @param xa Axis of cylinder
   */
  Cylinder(double r, util::Point x_begin, util::Point xa)
      : GeomObject("cylinder"), d_xBegin(x_begin), d_xa(xa / xa.length()),
        d_r(r), d_l(xa.length()){};

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   *
   * @note TODO
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief Center point of cross-section at the beginning */
  util::Point d_xBegin;

  /*! @brief axis of cylinder (unit vector) */
  util::Point d_xa;

  /*! @brief radius */
  double d_r;

  /*! @brief length */
  double d_l;
};

/*!
 * @brief Defines rectangle
 */
class Sphere : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  Sphere() : GeomObject("sphere"), d_xc(util::Point()), d_r(0.){};

  /*!
   * @brief Constructor
   *
   * @param r Radius
   * @param xc Center
   */
  Sphere(double r, util::Point xc) : GeomObject("sphere"), d_xc(xc), d_r(r){};

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief center point */
  util::Point d_xc;

  /*! @brief radius */
  double d_r;
};

/*!
 * @brief Defines annulus rectangle
 */
class AnnulusGeomObject : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  AnnulusGeomObject()
      : GeomObject("annulus_object"), d_outObj_p(nullptr), d_inObj_p(nullptr),
        d_dim(0){};

  /*!
   * @brief Constructor
   *
   * @param rin Inner rectangle
   * @param rout Outer rectangle
   */
  AnnulusGeomObject(GeomObject *in, GeomObject *out, size_t dim)
      : GeomObject("annulus_object"), d_outObj_p(out), d_inObj_p(in),
        d_dim(dim){};

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   *
   * @note TODO
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   *
   * @note TODO
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief Outer object */
  util::geometry::GeomObject *d_outObj_p;

  /*! @brief Inner object */
  util::geometry::GeomObject *d_inObj_p;

  /*! @brief Dimension objects live in */
  size_t d_dim;
};

/*!
 * @brief Defines complex geometrical object
 */
class ComplexGeomObject : public GeomObject {

public:
  /*!
   * @brief Constructor
   */
  ComplexGeomObject() : GeomObject("complex"), d_dim(0){};

  /*!
   * @brief Constructor
   *
   * @param rin Inner rectangle
   * @param rout Outer rectangle
   */
  ComplexGeomObject(
      std::vector<std::shared_ptr<util::geometry::GeomObject>> &obj,
      std::vector<std::string> obj_flag, size_t dim)
      : GeomObject("complex"), d_obj(obj), d_objFlag(obj_flag), d_dim(dim) {

    for (const auto &s : d_objFlag)
      if (s == "plus")
        d_objFlagInt.push_back(1);
      else if (s == "minus")
        d_objFlagInt.push_back(-1);
      else {
        std::cerr
            << "Error: Check object flag " + s +
               " passed to create ComplexGeomObject\n";
        exit(1);
      }
  };

  /*!
   * @brief Computes the area of the rectangle
   * @return Area Area of rectangle
   */
  double volume() const override;

  /*!
   * @brief Computes the center of object
   * @return Point Coordinates of center
   */
  util::Point center() const override;

  /*!
   * @brief Computes the bounding box of object
   * @return Pair Left-bottom-back and right-top-front corner points of box
   */
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;

  /*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   * @return Radius Radius of inscribed circle/sphere
   *
   * @note TODO
   */
  double inscribedRadius() const override;

  /*!
   * @brief Computes the radius of smallest circle/sphere such that object
   * can be fit into it
   * @return Radius Radius of bounding circle/sphere
   *
   * @note TODO
   */
  double boundingRadius() const override;

  /*!
   * @brief Checks if point is inside this object
   * @return True If point lies inside
   */
  bool isInside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is outside of this object
   * @return True If point lies outside
   */
  bool isOutside(const util::Point &x) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if within the tol distance
   */
  bool isNear(const util::Point &x, const double &tol) const override;

  /*!
   * @brief Checks if point is within given distance of this object
   * @return True True if it is near within tol distance
   */
  bool isNearBoundary(const util::Point &x, const double &tol,
                      const bool &within) const override;

  /*!
   * @brief Checks if point lies exactly on the boundary
   * @return True True if it lies on the boundary
   */
  bool doesIntersect(const util::Point &x) const override;

  /*!
   * @brief Checks if box is completely inside
   * @return True True if box lies inside
   */
  bool
  isInside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is outside of the object
   * @return True True if box lies outside
   */
  bool
  isOutside(const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Checks if box is within given distance of this object
   * @return True True if box is inside within the tol distance
   */
  bool isNear(const std::pair<util::Point, util::Point> &box,
              const double &tol) const override;

  /*!
   * @brief Checks if box intersects this object
   * @return True True if intersects
   */
  bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

  /*!
   * @brief Prints the information
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  std::string printStr(int nt, int lvl) const override;
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };
  void print() const override { print(0, 0); };

  /*! @brief Object */
  std::vector<std::shared_ptr<util::geometry::GeomObject>> d_obj;

  /*!
   * @brief Object flag
   *
   * Ordering of objects is important. To describe a rectangle with circular
   * hole, we will have d_obj = {rectangle, circle} and have flag = {plus,
   * minux}. This means final object is rectangle - circle
   *
   */
  std::vector<std::string> d_objFlag;
  std::vector<int> d_objFlagInt;

  /*! @brief Dimension objects live in */
  size_t d_dim;
};

/*!
 * @brief Get num params required for creation of object
 *
 * @param geom_type Geometry type of object
 * @return n Number of parameters required
 */
int getNumParamsNRequired(std::string geom_type);

/*!
 * @brief Check parameter data for validity
 *
 * @param n Number of parameters available
 * @param geom_type Geometry type of object
 * @return True True if number of parameter is correct
 */
bool checkParamForGeometry(size_t n, std::string geom_type);

/*!
 * @brief Check parameter data for validity
 *
 * @param n Number of parameters available
 * @param geom_type Geometry type of object
 * @param vec_type For complex objects, specify types of sub-objects
 * @return True True if number of parameter is correct
 */
bool checkParamForComplexGeometry(size_t n, std::string geom_type,
                                  std::vector<std::string> vec_type);

/*!
 * @brief Create geometrical object from the given data
 *
 * @param type Type of object
 * @param params Vector of parameters
 * @param vec_type Sub-types of complex object
 * @param vec_flag Flags of sub-types of complex object
 * @param obj Pointer to object to which new object will be associated
 * @param dim Dimension
 * @param perform_check Perform check for sufficient parameters
 */
void createGeomObject(const std::string &type,
                      const std::vector<double> &params,
                      const std::vector<std::string> &vec_type,
                      const std::vector<std::string> &vec_flag,
                      std::shared_ptr<util::geometry::GeomObject> &obj,
                      const size_t &dim,
                      bool perform_check = true);

} // namespace geometry

} // namespace util

#endif // UTIL_GEOMETRY_OBJECTS_H
