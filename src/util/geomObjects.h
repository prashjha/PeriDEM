/*
* -------------------------------------------
* Copyright (c) 2021 - 2024 Prashant K. Jha
* -------------------------------------------
* PeriDEM https://github.com/prashjha/PeriDEM
*
* Distributed under the Boost Software License, Version 1.0. (See accompanying
* file LICENSE)
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
    /*! @brief Geometry type to dimension map */
    const std::map<std::string, size_t> geom_type_to_dim = {
      {"circle", 2},
      {"square", 2},
      {"rectangle", 2},
      {"hexagon", 2},
      {"triangle", 2},
      {"drum2d", 2},
      {"sphere", 3},
      {"cube", 3},
      {"cuboid", 3}
    };

    /*! @brief Returns list of acceptable geometries for PeriDEM simulation */
    inline size_t getGeomTypeToDim(std::string type) {
      return geom_type_to_dim.at(type);
    };

    /*!
     * @brief Defines abstract geometrical domain
     */
    class GeomObject {
    public:
      /*!
       * @brief Constructor
       * @param name Name of the geometric object
       * @param description Description of object (e.g., further classification or any tag)
       */
      explicit GeomObject(std::string name = "",
                          std::string description = "")
        : d_name(name), d_description(description) {
      };

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
      virtual std::pair<util::Point, util::Point>
      box(const double &tol) const {
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

      /**
       * @name Interaction with point
       */
      /** @{*/
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
      virtual bool
      isOutside(const util::Point &x) const { return false; };

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
      virtual bool
      doesIntersect(const util::Point &x) const { return false; };

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/
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
      doesIntersect(
        const std::pair<util::Point, util::Point> &box) const {
        return false;
      };

      /** @}*/

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

        oss << tabS << "------- GeomObject --------" << std::endl
            << std::endl;
        oss << tabS << "Base geometrical object" << std::endl;
        oss << tabS << "Base implementation of GeomObject." << std::endl;
        oss << tabS << "Name of GeomObject = " << d_name << std::endl;
        oss << tabS << "Description of GeomObject = " << d_description
            << std::endl;

        return oss.str();
      };

      /*!
       * @brief Prints the information about the object
       *
       * @param nt Number of tabs to append before printing
       * @param lvl Information level (higher means more information)
       */
      virtual void print(int nt, int lvl) const {
        std::cout << printStr(nt, lvl);
      };

      /*! @brief Prints the information about the object */
      virtual void print() const { print(0, 0); };

    public:
      /*! @brief name of object */
      const std::string d_name;

      /*! @brief Further description of object */
      const std::string d_description;

      /*! @brief Tags/attributes about the object */
      std::vector<std::string> d_tags;
    };

    /*!
    * @brief Defines null (empty) geom object
    */
    class NullGeomObject : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      NullGeomObject(std::string description = "") : GeomObject("null", description) {
      };

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      virtual std::string printStr(int nt, int lvl) const override {
        auto tabS = util::io::getTabS(nt);
        std::ostringstream oss;

        oss << tabS << "------- NullGeomObject --------" << std::endl
            << std::endl;
        oss << tabS << "Name of GeomObject = " << d_name << std::endl;

        return oss.str();
      };

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };
    };

    /*!
    * @brief Defines Line
    */
    class Line : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Line()
        : GeomObject("line", ""),
          d_r(0.),
          d_L(0.),
          d_x(util::Point()),
          d_vertices({util::Point(), util::Point()}) {
      };

      /*!
       * @brief Constructor
       *
       * @param x1 Left-bottom corner point
       * @param x2 Right-top corner point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Line(util::Point x1, util::Point x2, std::string description = "")
        : GeomObject("line", description),
          d_L((x1 - x2).length()),
          d_r(0.5 * (x1 - x2).length()),
          d_x(0.5 * (x1 + x2)),
          d_vertices({x1, x2}) {
      };

      /*!
       * @brief Constructor
       *
       * @param L Length of a line
       * @param x Center point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Line(double L, util::Point x = util::Point(),
           std::string description = "")
        : GeomObject("line", description),
          d_L(L),
          d_r(0.5 * L),
          d_x(x),
          d_vertices({
            x + util::Point(-0.5 * L, 0., 0.),
            x + util::Point(0.5 * L, 0., 0.)
          }) {
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       * TODO Implement methods
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Length of line */
      double d_L;

      /*! @brief Radius of bounding circle */
      double d_r;
    };

    /*!
    * @brief Defines Triangle
    */
    class Triangle : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Triangle()
        : GeomObject("triangle", ""),
          d_r(0.),
          d_a(util::Point(1., 0., 0.)),
          d_x(util::Point()),
          d_vertices(std::vector<util::Point>(3, util::Point())) {
      };

      /*!
       * @brief Constructor
       *
       * @param r Radius (distance of vertex from center)
       * @param x Center point
       * @param a Axis vector that will be rotated and scaled by r to find coordiantes of triangle vertices
       * @param description Description of object (e.g., further classification or any tag)
       */
      Triangle(double r, util::Point x = util::Point(0., 0., 0.),
               util::Point a = util::Point(1., 0., 0.),
               std::string description = "uniform")
        : GeomObject("triangle", description),
          d_r(r),
          d_a(a),
          d_x(x),
          d_vertices(std::vector<util::Point>(3, util::Point())) {
        // generate vertices
        auto rotate_axis = util::Point(0., 0., 1.); // z-axis
        for (int i = 0; i < 3; i++) {
          d_vertices[i] = d_x + d_r * util::rotate(
                            d_a, i * 2. * M_PI / 3., rotate_axis);
        }
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       * TODO Implement methods
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Distance between center and the farthest vertex of triangle */
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
    * @brief Defines Square
    */
    class Square : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Square()
        : GeomObject("square", ""),
          d_L(0.),
          d_r(0.),
          d_x(util::Point()),
          d_vertices({
            util::Point(), util::Point(),
            util::Point(), util::Point()
          }) {
      };

      /*!
       * @brief Constructor
       *
       * @param L Length along x and y-directions (Square)
       * @param x Center point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Square(double L, util::Point x = util::Point(),
             std::string description = "")
        : GeomObject("square", description),
          d_L(L),
          d_r(L / std::sqrt(2)),
          d_x(x),
          d_vertices({
            x + util::Point(-0.5 * L, -0.5 * L, 0.),
            x + util::Point(0.5 * L, -0.5 * L, 0.),
            x + util::Point(0.5 * L, 0.5 * L, 0.),
            x + util::Point(-0.5 * L, 0.5 * L, 0.)
          }) {
      };

      /*!
       * @brief Constructor
       *
       * @param x1 Left-bottom corner point
       * @param x2 Right-top corner point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Square(util::Point x1, util::Point x2, std::string description = "")
        : GeomObject("square", description),
          d_L((x1 - x2).length() * (1. / std::sqrt(2))),
          d_r((x1 - x2).length() / 2.),
          d_x(0.5 * (x1 + x2)) {
        d_vertices.push_back(x1);
        d_vertices.push_back(util::Point(x1.d_x + d_L, x1.d_y, x1.d_z));
        d_vertices.push_back(x2);
        d_vertices.push_back(util::Point(x2.d_x - d_L, x2.d_y, x2.d_z));
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Edge length of a square */
      double d_L;

      /*! @brief Radius of bounding circle */
      double d_r;
    };

    /*!
    * @brief Defines Rectangle
    */
    class Rectangle : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Rectangle()
        : GeomObject("rectangle", ""),
          d_Lx(0.),
          d_Ly(0.),
          d_r(0.),
          d_x(util::Point()),
          d_vertices({
            util::Point(), util::Point(),
            util::Point(), util::Point()
          }) {
      };

      /*!
       * @brief Constructor
       *
       * @param Lx Length along x-direction
       * @param Ly Length along y-direction
       * @param x Center point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Rectangle(double Lx, double Ly, util::Point x = util::Point(),
                std::string description = "")
        : GeomObject("rectangle", description),
          d_Lx(Lx),
          d_Ly(Ly),
          d_r(0.5 * std::sqrt(std::pow(Lx, 2) + std::pow(Ly, 2))),
          d_x(x) {
        d_vertices.push_back(
          x + util::Point(-0.5 * d_Lx, -0.5 * d_Ly, 0.));
        d_vertices.push_back(
          x + util::Point(0.5 * d_Lx, -0.5 * d_Ly, 0.));
        d_vertices.push_back(x + util::Point(0.5 * d_Lx, 0.5 * d_Ly, 0.));
        d_vertices.push_back(
          x + util::Point(-0.5 * d_Lx, 0.5 * d_Ly, 0.));
      };

      /*!
       * @brief Constructor
       *
       * @param x1 Left-bottom corner point
       * @param x2 Right-top corner point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Rectangle(util::Point x1, util::Point x2,
                std::string description = "")
        : GeomObject("rectangle", description),
          d_Lx(x2.d_x - x1.d_x),
          d_Ly(x2.d_y - x1.d_y),
          d_r(0.5 *
              std::sqrt(std::pow(d_Lx, 2) + std::pow(d_Ly, 2))),
          d_x(0.5 * (x1 + x2)) {
        d_vertices.push_back(x1);
        d_vertices.push_back(util::Point(x1.d_x + d_Lx, x1.d_y, x1.d_z));
        d_vertices.push_back(x2);
        d_vertices.push_back(util::Point(x2.d_x - d_Lx, x2.d_y, x2.d_z));
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Edge length of a rectangle in x-direction */
      double d_Lx;

      /*! @brief Edge length of a rectangle in y-direction */
      double d_Ly;

      /*! @brief Radius of bounding circle */
      double d_r;
    };

    /*!
    * @brief Defines Hexagon
    */
    class Hexagon : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Hexagon()
        : GeomObject("hexagon", ""),
          d_r(0.),
          d_a(util::Point(1., 0., 0.)),
          d_x(util::Point()),
          d_vertices(std::vector<util::Point>(6, util::Point())) {
      };

      /*!
       * @brief Constructor
       *
       * @param r Radius (distance between center and one of the vertices)
       * @param x Center point
       * @param a Axis vector that will be rotated and scaled by r to obtain coordinates of vertices of hexagon
       * @param description Description of object (e.g., further classification or any tag)
       */
      Hexagon(double r, util::Point x = util::Point(0., 0., 0.),
              util::Point a = util::Point(1., 0., 0.),
              std::string description = "")
        : GeomObject("hexagon", ""),
          d_r(r),
          d_a(a),
          d_x(x),
          d_vertices(std::vector<util::Point>(6, util::Point())) {
        // generate vertices
        auto rotate_axis = util::Point(0., 0., 1.); // z-axis
        for (int i = 0; i < 6; i++) {
          d_vertices[i] = d_x + d_r * util::rotate(
                            d_a, i * M_PI / 3., rotate_axis);
        }
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       *
       * TODO implement methods robustly
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Distance between center and the farthest vertex of hexagon */
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
    * @brief Defines Drum2D
    *
    *             v3   o-------------------o   v2
    *                  \                   /
    *                   \                 /
    *                    \               /
    *               v4    o      +      o v1
    *                    /       c       \
    *                   /                 \
    *                  /                   \
    *            v5   o---------------------o  v6
    *
    * w = distance between c and v1 = half-width of neck
    * r = distance between c and v2
    * theta = pi/3 = angle between c-v2 and c-v1
    * a = axis = unit vector from c to v1
    *
    */
    class Drum2D : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Drum2D()
        : GeomObject("drum2d", ""),
          d_r(0.),
          d_w(0.),
          d_a(util::Point(1., 0., 0.)),
          d_x(util::Point()),
          d_vertices(std::vector<util::Point>(6, util::Point())) {
      };

      /*!
       * @brief Constructor
       *
       * @param r Distance between center and farthest vertex of drum2d structure
       * @param w Half of the distance between two vertices in the neck (half width of neck)
       * @param x Center point
       * @param a Axis vector that will be rotated and scaled by r to get the
       * farthest four vertices of structure
       * @param description Description of object (e.g., further classification or any tag)
       */
      Drum2D(double r, double w, util::Point x = util::Point(0., 0., 0.),
             util::Point a = util::Point(1., 0., 0.),
             std::string description = "")
        : GeomObject("drum2d", description),
          d_r(r),
          d_w(w),
          d_a(a),
          d_x(x),
          d_vertices(std::vector<util::Point>(6, util::Point())) {
        // generate vertices
        auto rotate_axis = util::Point(0., 0., 1.); // z-axis

        // half-width of big (top and bottom) edge
        double w_big_edge = d_r * std::cos(M_PI / 3.);

        d_vertices[0] = d_x + d_w * d_a;
        d_vertices[3] = d_x - d_w * d_a;

        d_vertices[1] =
            d_x + d_r * util::rotate(d_a, M_PI / 3., rotate_axis);
        // to get third vertex, we could either rotate axis further or use second vertex to get
        // to the third vertex
        // Option 1
        // d_vertices[2] = d_x + d_r * util::rotate(d_a, 2.*M_PI/3., rotate_axis);
        // Option 2
        d_vertices[2] = d_vertices[1] - 2 * w_big_edge * rotate_axis;

        d_vertices[4] = d_x + d_r * util::rotate(-1. * d_a, M_PI / 3.,
                                                 rotate_axis);
        // Option 2
        d_vertices[5] = d_vertices[4] + 2. * w_big_edge * d_a;
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Half width of neck */
      double d_w;

      /*! @brief Distance between center and the farthest vertex */
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
       * Axis is a unit vector from x to v1
       */
      util::Point d_a;
    };

    /*!
    * @brief Defines cube
    */
    class Cube : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Cube()
        : GeomObject("cube", ""),
          d_L(0.),
          d_r(0.),
          d_x(util::Point()),
          d_vertices(std::vector<util::Point>(8, util::Point())) {
      };

      /*!
       * @brief Constructor
       *
       * @param L Length along x, y, and z-direction
       * @param x Coordinate of center
       * @param description Description of object (e.g., further classification or any tag)
       */
      Cube(double L, util::Point x = util::Point(),
           std::string description = "")
        : GeomObject("cube", description),
          d_L(L),
          d_r(0.5 * std::sqrt(3.) * L),
          d_x(x),
          d_vertices(std::vector<util::Point>(8, util::Point())) {
        // numbering assuming x - left-right, y - back-front, z - bottom-top
        // counterclockwise in bottom plane (i.e.,
        // 0 - left-back-bottom, 1 - right-back-bottom,
        // 2 - right-front-bottom, 3 - left-front-bottom
        // similarly, in top plane
        d_vertices[0] =
            d_x + util::Point(-0.5 * d_L, -0.5 * d_L, -0.5 * d_L);
        d_vertices[1] =
            d_x + util::Point(0.5 * d_L, -0.5 * d_L, -0.5 * d_L);
        d_vertices[2] =
            d_x + util::Point(0.5 * d_L, 0.5 * d_L, -0.5 * d_L);
        d_vertices[3] =
            d_x + util::Point(-0.5 * d_L, 0.5 * d_L, -0.5 * d_L);

        d_vertices[4] =
            d_x + util::Point(-0.5 * d_L, -0.5 * d_L, 0.5 * d_L);
        d_vertices[5] =
            d_x + util::Point(0.5 * d_L, -0.5 * d_L, 0.5 * d_L);
        d_vertices[6] =
            d_x + util::Point(0.5 * d_L, 0.5 * d_L, 0.5 * d_L);
        d_vertices[7] =
            d_x + util::Point(-0.5 * d_L, 0.5 * d_L, 0.5 * d_L);
      };

      /*!
       * @brief Constructor
       *
       * @param x1 Left-bottom-back corner point
       * @param x2 Right-top-front corner point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Cube(util::Point x1, util::Point x2, std::string description = "")
        : GeomObject("cube", description),
          d_L((x2 - x1).length() / std::sqrt(3.)),
          d_r(0.5 * (x2 - x1).length()),
          d_x(0.5 * (x1 + x2)),
          d_vertices(std::vector<util::Point>(8, util::Point())) {
        // numbering assuming x - left-right, y - back-front, z - bottom-top
        // counterclockwise in bottom plane (i.e.,
        // 0 - left-back-bottom, 1 - right-back-bottom,
        // 2 - right-front-bottom, 3 - left-front-bottom
        // similarly, in top plane
        d_vertices[0] =
            d_x + util::Point(-0.5 * d_L, -0.5 * d_L, -0.5 * d_L);
        d_vertices[1] =
            d_x + util::Point(0.5 * d_L, -0.5 * d_L, -0.5 * d_L);
        d_vertices[2] =
            d_x + util::Point(0.5 * d_L, 0.5 * d_L, -0.5 * d_L);
        d_vertices[3] =
            d_x + util::Point(-0.5 * d_L, 0.5 * d_L, -0.5 * d_L);

        d_vertices[4] =
            d_x + util::Point(-0.5 * d_L, -0.5 * d_L, 0.5 * d_L);
        d_vertices[5] =
            d_x + util::Point(0.5 * d_L, -0.5 * d_L, 0.5 * d_L);
        d_vertices[6] =
            d_x + util::Point(0.5 * d_L, 0.5 * d_L, 0.5 * d_L);
        d_vertices[7] =
            d_x + util::Point(-0.5 * d_L, 0.5 * d_L, 0.5 * d_L);
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Edge length of cube */
      double d_L;

      /*! @brief Radius of bounding circle */
      double d_r;
    };

    /*!
    * @brief Defines cuboid
    */
    class Cuboid : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Cuboid()
        : GeomObject("cuboid", ""),
          d_Lx(0.),
          d_Ly(0.),
          d_Lz(0.),
          d_r(0.),
          d_x(util::Point()),
          d_vertices(std::vector<util::Point>(8, util::Point())) {
      };

      /*!
       * @brief Constructor
       *
       * @param Lx Length along x-direction
       * @param Ly Length along y-direction
       * @param Lz Length along z-direction
       * @param x Center point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Cuboid(double Lx, double Ly, double Lz,
             util::Point x = util::Point(),
             std::string description = "")
        : GeomObject("cuboid", description),
          d_Lx(Lx),
          d_Ly(Ly),
          d_Lz(Lz),
          d_r(std::sqrt(std::pow(Lx, 2) + std::pow(Ly, 2) +
                        std::pow(Lz, 2))),
          d_x(x),
          d_vertices(std::vector<util::Point>(8, util::Point())) {
        // numbering assuming x - left-right, y - back-front, z - bottom-top
        // counterclockwise in bottom plane (i.e.,
        // 0 - left-back-bottom, 1 - right-back-bottom,
        // 2 - right-front-bottom, 3 - left-front-bottom
        // similarly, in top plane
        d_vertices[0] =
            d_x + util::Point(-0.5 * d_Lx, -0.5 * d_Ly, -0.5 * d_Lz);
        d_vertices[1] =
            d_x + util::Point(0.5 * d_Lx, -0.5 * d_Ly, -0.5 * d_Lz);
        d_vertices[2] =
            d_x + util::Point(0.5 * d_Lx, 0.5 * d_Ly, -0.5 * d_Lz);
        d_vertices[3] =
            d_x + util::Point(-0.5 * d_Lx, 0.5 * d_Ly, -0.5 * d_Lz);

        d_vertices[4] =
            d_x + util::Point(-0.5 * d_Lx, -0.5 * d_Ly, 0.5 * d_Lz);
        d_vertices[5] =
            d_x + util::Point(0.5 * d_Lx, -0.5 * d_Ly, 0.5 * d_Lz);
        d_vertices[6] =
            d_x + util::Point(0.5 * d_Lx, 0.5 * d_Ly, 0.5 * d_Lz);
        d_vertices[7] =
            d_x + util::Point(-0.5 * d_Lx, 0.5 * d_Ly, 0.5 * d_Lz);
      };

      /*!
       * @brief Constructor
       *
       * @param x1 Left-bottom-back corner point
       * @param x2 Right-top-front corner point
       * @param description Description of object (e.g., further classification or any tag)
       */
      Cuboid(util::Point x1, util::Point x2, std::string description = "")
        : GeomObject("cuboid", description),
          d_Lx(x2.d_x - x1.d_x),
          d_Ly(x2.d_y - x1.d_y),
          d_Lz(x2.d_z - x1.d_z),
          d_r(std::sqrt(
              std::pow(x2.d_x - x1.d_x, 2)
              + std::pow(x2.d_y - x1.d_y, 2)
              + std::pow(x2.d_z - x1.d_z, 2)
            )
          ),
          d_x(0.5 * (x1 + x2)),
          d_vertices(std::vector<util::Point>(8, util::Point())) {
        // numbering assuming x - left-right, y - back-front, z - bottom-top
        // counterclockwise in bottom plane (i.e.,
        // 0 - left-back-bottom, 1 - right-back-bottom,
        // 2 - right-front-bottom, 3 - left-front-bottom
        // similarly, in top plane
        d_vertices[0] =
            d_x + util::Point(-0.5 * d_Lx, -0.5 * d_Ly, -0.5 * d_Lz);
        d_vertices[1] =
            d_x + util::Point(0.5 * d_Lx, -0.5 * d_Ly, -0.5 * d_Lz);
        d_vertices[2] =
            d_x + util::Point(0.5 * d_Lx, 0.5 * d_Ly, -0.5 * d_Lz);
        d_vertices[3] =
            d_x + util::Point(-0.5 * d_Lx, 0.5 * d_Ly, -0.5 * d_Lz);

        d_vertices[4] =
            d_x + util::Point(-0.5 * d_Lx, -0.5 * d_Ly, 0.5 * d_Lz);
        d_vertices[5] =
            d_x + util::Point(0.5 * d_Lx, -0.5 * d_Ly, 0.5 * d_Lz);
        d_vertices[6] =
            d_x + util::Point(0.5 * d_Lx, 0.5 * d_Ly, 0.5 * d_Lz);
        d_vertices[7] =
            d_x + util::Point(-0.5 * d_Lx, 0.5 * d_Ly, 0.5 * d_Lz);
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Vertices */
      std::vector<util::Point> d_vertices;

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Edge length of cuboid in x-direction */
      double d_Lx;

      /*! @brief Edge length of cuboid in y-direction */
      double d_Ly;

      /*! @brief Edge length of cuboid in z-direction */
      double d_Lz;

      /*! @brief Radius of bounding circle */
      double d_r;
    };

    /*!
    * @brief Defines circle
    */
    class Circle : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Circle()
        : GeomObject("circle", ""),
          d_x(util::Point()),
          d_r(0.) {
      };

      /*!
       * @brief Constructor
       *
       * @param r Radius
       * @param x Center
       * @param description Description of object (e.g., further classification or any tag)
       */
      Circle(double r, util::Point x = util::Point(),
             std::string description = "")
        : GeomObject("circle", description),
          d_x(x),
          d_r(r) {
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Radius */
      double d_r;
    };

    /*!
    * @brief Defines sphere
    */
    class Sphere : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Sphere()
        : GeomObject("sphere", ""),
          d_x(util::Point()),
          d_r(0.) {
      };

      /*!
       * @brief Constructor
       *
       * @param r Radius
       * @param x Center
       * @param description Description of object (e.g., further classification or any tag)
       */
      Sphere(double r, util::Point x = util::Point(),
             std::string description = "")
        : GeomObject("sphere", description),
          d_x(x),
          d_r(r) {
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Radius */
      double d_r;
    };

    /*!
    * @brief Defines cylinder
    */
    class Cylinder : public GeomObject {
    public:
      /*!
       * @brief Constructor
       */
      Cylinder()
        : GeomObject("cylinder", ""),
          d_xBegin(util::Point()),
          d_xa(util::Point(1., 1., 1.)),
          d_r(0.),
          d_l(0.),
          d_x(util::Point()) {
      };

      /*!
       * @brief Constructor
       *
       * @param r Radius of cylinder
       * @param l Length of cylinder
       * @param x_begin Center of the bottom bottom cross-section
       * @param xa Axis of cylinder (if it is not a unit vector, we compute unit vector along xa)
       * @param description Description of object (e.g., further classification or any tag)
       */
      Cylinder(double r, double l, util::Point x_begin, util::Point xa,
               std::string description = "")
        : GeomObject("cylinder", description),
          d_xBegin(x_begin),
          d_xa(xa / xa.length()),
          d_r(r),
          d_l(l),
          d_x(x_begin + 0.5 * l * xa) {
      };

      /*!
       * @brief Constructor
       *
       * @param r Radius of cylinder
       * @param x_begin Center of the bottom bottom cross-section
       * @param xa Axis of cylinder (should be the actual vector from center of bottom
       * section to top section so that the length of the vector is a length of cylinder)
       * @param description Description of object (e.g., further classification or any tag)
       */
      Cylinder(double r, util::Point x_begin, util::Point xa,
               std::string description = "")
        : GeomObject("cylinder", description),
          d_xBegin(x_begin),
          d_xa(xa / xa.length()),
          d_r(r),
          d_l(xa.length()),
          d_x(x_begin + 0.5 * xa.length() * xa) {
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Center */
      util::Point d_x;

      /*! @brief Center point of cross-section at the beginning */
      util::Point d_xBegin;

      /*! @brief Axis of cylinder (unit vector) */
      util::Point d_xa;

      /*! @brief Radius */
      double d_r;

      /*! @brief Length */
      double d_l;
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
        : GeomObject("annulus_object", ""),
          d_outObj_p(nullptr),
          d_inObj_p(nullptr),
          d_dim(0) {
      };

      /*!
       * @brief Constructor
       *
       * @param in Inner object
       * @param out Outer object
       * @param description Description of object (e.g., further classification or any tag)
       */
      AnnulusGeomObject(GeomObject *in, GeomObject *out, std::string description = "")
        : GeomObject("annulus_object", description),
          d_outObj_p(out),
          d_inObj_p(in),
          d_dim(0) {
        d_dim = getGeomTypeToDim(d_outObj_p->d_name); // assume all types have same dim
      };

      /*!
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
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
      ComplexGeomObject() : GeomObject("complex", ""), d_dim(0) {
      };

      /*!
       * @brief Constructor
       *
       * @param obj Vector of geometrical objects
       * @param obj_flag Specifies which objects are filling and which are void
       * @param description Description of object (e.g., further classification or any tag)
       */
      ComplexGeomObject(
        std::vector<std::shared_ptr<util::geometry::GeomObject> > &obj,
        std::vector<std::string> obj_flag, std::string description = "")
        : GeomObject("complex", description),
          d_obj(obj),
          d_objFlag(obj_flag),
          d_dim(0) {

        d_dim = getGeomTypeToDim(d_obj[0]->d_name); // assume all types have same dim

        for (const auto &s: d_objFlag)
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
       * @copydoc GeomObject::volume() const
       */
      double volume() const override;

      /*!
       * @copydoc GeomObject::center() const
       */
      util::Point center() const override;

      /*!
       * @copydoc GeomObject::box() const
       */
      std::pair<util::Point, util::Point> box() const override;

      /*!
       * @copydoc GeomObject::box(const double &tol) const
       */
      std::pair<util::Point, util::Point>
      box(const double &tol) const override;

      /*!
       * @copydoc GeomObject::inscribedRadius() const
       */
      double inscribedRadius() const override;

      /*!
       * @copydoc GeomObject::boundingRadius() const
       */
      double boundingRadius() const override;

      /**
       * @name Interaction with point
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(const util::Point &x) const
       */
      bool isInside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isOutside(const util::Point &x) const
       */
      bool isOutside(const util::Point &x) const override;

      /*!
       * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
       */
      bool isNear(const util::Point &x, const double &tol) const override;

      /*!
       * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) cons
       */
      bool isNearBoundary(const util::Point &x, const double &tol,
                          const bool &within) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(const util::Point &x) const
       */
      bool doesIntersect(const util::Point &x) const override;

      /** @}*/

      /**
       * @name Interaction with box
       */
      /** @{*/

      /*!
       * @copydoc GeomObject::isInside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isInside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isOutside(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool
      isOutside(
        const std::pair<util::Point, util::Point> &box) const override;

      /*!
       * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const
       */
      bool isNear(const std::pair<util::Point, util::Point> &box,
                  const double &tol) const override;

      /*!
       * @copydoc GeomObject::doesIntersect(
              const std::pair<util::Point, util::Point> &box) const
       */
      bool doesIntersect(
        const std::pair<util::Point, util::Point> &box) const override;

      /** @}*/

      /*!
       * @copydoc GeomObject::printStr(int nt, int lvl) const
       */
      std::string printStr(int nt, int lvl) const override;

      /*!
       * @copydoc GeomObject::print(int nt, int lvl) const
       */
      void print(int nt, int lvl) const override {
        std::cout << printStr(nt, lvl);
      };

      /*!
       * @copydoc GeomObject::print() const
       */
      void print() const override { print(0, 0); };

      /*! @brief Object */
      std::vector<std::shared_ptr<util::geometry::GeomObject> > d_obj;

      /*!
       * @brief Object flag
       *
       * Ordering of objects is important. To describe a rectangle with circular
       * hole, we will have d_obj = {rectangle, circle} and have flag = {plus,
       * minus}. This means final object is rectangle - circle
       *
       */
      std::vector<std::string> d_objFlag;

      /*!
       * @brief Object integer flags. Here, +1 means object is filling and -1 means object is void
       */
      std::vector<int> d_objFlagInt;

      /*! @brief Dimension objects live in */
      size_t d_dim;
    };
  } // namespace geometry
} // namespace util

#endif // UTIL_GEOMETRY_OBJECTS_H
