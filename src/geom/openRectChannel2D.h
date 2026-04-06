/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef GEOM_OPEN_RECT_CHANNEL_2D_H
#define GEOM_OPEN_RECT_CHANNEL_2D_H

#include "geomObjects.h"
#include <string>
#include <utility>
#include <vector>

namespace geom {

/*!
 * @brief 2D U-shaped cavity: thick rectangular frame with the top (+y) side open.
 *
 * Parameters (createGeomObject): x0, y0, x1, y1, t, z — outer axis-aligned box
 * [x0,x1]×[y0,y1] in the plane z = const, wall thickness t. Requires
 * x1 − x0 > 2t, y1 − y0 > 2t, t > 0.
 */
class OpenRectChannel2D : public GeomObject {
public:
  double d_x0 = 0.;
  double d_y0 = 0.;
  double d_x1 = 0.;
  double d_y1 = 0.;
  double d_t = 0.;
  double d_z = 0.;
  /*! @brief CCW boundary of the U-shaped domain (closed polygon). */
  std::vector<util::Point> d_vertices;

  OpenRectChannel2D();

  OpenRectChannel2D(double x0, double y0, double x1, double y1, double t, double z,
                    std::string description = "");

  OpenRectChannel2D(const OpenRectChannel2D &other);

  OpenRectChannel2D &operator=(const OpenRectChannel2D &other);

  void buildVertices();

  static bool pointInPolygon2D(const util::Point &p, const std::vector<util::Point> &poly);
  static double polygonArea2D(const std::vector<util::Point> &poly);
  static util::Point polygonCentroid2D(const std::vector<util::Point> &poly);

  void transform(const util::Point &center, const double &scale, const double &angle,
                 const util::Point &axis) override;

  double volume() const override;
  util::Point center() const override;
  std::pair<util::Point, util::Point> box() const override;
  std::pair<util::Point, util::Point> box(const double &tol) const override;
  double inscribedRadius() const override;
  double boundingRadius() const override;

  bool isInside(const util::Point &x) const override;
  bool isOutside(const util::Point &x) const override;
  bool isNear(const util::Point &x, const double &tol) const override;
  bool isNearBoundary(const util::Point &x, const double &tol, const bool &within) const override;
  bool doesIntersect(const util::Point &x) const override;

  bool isInside(const std::pair<util::Point, util::Point> &bx) const override;
  bool isOutside(const std::pair<util::Point, util::Point> &bx) const override;
  bool isNear(const std::pair<util::Point, util::Point> &bx, const double &tol) const override;
  bool doesIntersect(const std::pair<util::Point, util::Point> &bx) const override;

  std::string printStr(int nt = 0, int lvl = 0) const override;
  void print(int nt = 0, int lvl = 0) const override { std::cout << printStr(nt, lvl); };
  void print() const override { print(0, 0); };
};

} // namespace geom

#endif
