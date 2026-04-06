/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0.
 */

#ifndef GEOM_OPEN_CUBOID_CHANNEL_3D_H
#define GEOM_OPEN_CUBOID_CHANNEL_3D_H

#include "geomObjects.h"
#include <string>
#include <utility>
#include <vector>

namespace geom {

/*!
 * @brief Hollow axis-aligned cuboid shell with uniform wall thickness and one outer face open.
 *
 * Deck / `GeomObject::d_name`: **`open_cuboid_channel_3d`** (pairs with 2D `open_rect_channel_2d`,
 * which uses a rectangular footprint in the plane).
 *
 * Parameters (createGeomObject): `x0,y0,z0,x1,y1,z1,t,open_face` — outer AABB
 * `[x0,x1]×[y0,y1]×[z0,z1]`, wall thickness `t`, and `open_face` ∈ {0,…,5}:
 * `0` = +x, `1` = −x, `2` = +y, `3` = −y, `4` = +z, `5` = −z (world axes).
 *
 * The solid is the closed shell (outer minus inner cavity) with the corresponding outer
 * face slab removed (the “roof” / lid of that side), so the cavity connects to the exterior
 * through that opening.
 */
class OpenCuboidChannel3D : public GeomObject {
public:
  /*! @brief Outer AABB low corner */
  util::Point d_lo;
  /*! @brief Outer AABB high corner */
  util::Point d_hi;
  /*! @brief Wall thickness */
  double d_t = 0.;
  /*!
   * @brief Which outer face is open: 0=+x, 1=−x, 2=+y, 3=−y, 4=+z, 5=−z (world axes after transform).
   */
  int d_openFace = 0;

  /*! @brief Centroid of the open shell (cached). */
  util::Point d_x;

  OpenCuboidChannel3D();

  OpenCuboidChannel3D(double x0, double y0, double z0, double x1, double y1, double z1, double t,
                      int open_face, std::string description = "");

  OpenCuboidChannel3D(const OpenCuboidChannel3D &other);

  OpenCuboidChannel3D &operator=(const OpenCuboidChannel3D &other);

  void transform(const util::Point &translation, const double &scale, const double &angle,
                 const util::Point &axis, const util::Point *rotationPoint) override;

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

private:
  void validateAndRefreshCenter();
  bool inClosedShell(const util::Point &p) const;
  bool inRemovedFaceSlab(const util::Point &p) const;
  util::Point computeCenter() const;
};

} // namespace geom

#endif
