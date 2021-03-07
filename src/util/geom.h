/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef UTIL_GEOMETRY_H
#define UTIL_GEOMETRY_H

#include "point.h" // definition of Point
#include <vector>

namespace util {

/*!
 * @brief Returns all corner points in the box
 * @param dim Dimension of the box
 * @param box Pair of points representing cuboid (rectangle in 2d)
 * @return Vector Vector of corner points
 */
std::vector<util::Point> getCornerPoints(size_t dim, const
std::pair<util::Point, util::Point> &box);

/*!
 * @brief Returns all corner points in the box
 * @param dim Dimension of the box
 * @param box Pair of points representing cuboid (rectangle in 2d)
 * @return Vector Vector of corner points
 */
std::vector<std::pair<util::Point, util::Point>> getEdges(size_t dim, const
std::pair<util::Point, util::Point> &box);

/*!
 * @brief Returns center point
 * @param dim Dimension of the box
 * @param box Pair of points representing cuboid (rectangle in 2d)
 * @return Point Coordinates of center point
 */
util::Point getCenter(size_t dim,
                       const std::pair<util::Point, util::Point> &box);

/*!
   * @brief Computes the radius of biggest circle/sphere completely within the
   * object
   *
   * @param dim Dimension of the box
   * @param box Pair of corner points of the box
   * @return Radius Radius of inscribed circle/sphere
   */
double inscribedRadiusInBox(size_t dim,
                            const std::pair<util::Point, util::Point> &box);

/*!
   * @brief Computes the radius of smallest circle/sphere which can
   * have the box inside
   *
   * @param dim Dimension of the box
   * @param box Pair of corner points of the box
   * @return Radius Radius of inscribed circle/sphere
   */
double circumscribedRadiusInBox(size_t dim,
                            const std::pair<util::Point, util::Point> &box);

/**
   * @name Methods to check point in domain
   */
/**@{*/

/*!
 * @brief Returns true if point is inside box
 * @param x Point
 * @param dim Dimension of the box
 * @param box Pair of points representing cuboid (rectangle in 2d)
 * @return True If point is inside box
 */
bool isPointInsideBox(util::Point x, size_t dim,
                      const std::pair<util::Point, util::Point> &box);

/*!
 * @brief Checks if point is inside a rectangle
 * @param x Point
 * @param x_min X coordinate of left-bottom corner point
 * @param x_max X coordinate of right-top corner point
 * @param y_min Y coordinate of left-bottom corner point
 * @param y_max Y coordinate of right-top corner point
 * @return bool True if point inside rectangle, else false
 */
bool isPointInsideRectangle(util::Point x, double x_min, double x_max,
                            double y_min, double y_max);

/*!
 * @brief Checks if point is inside a rectangle
 * @param x Point
 * @param x_lb Coordinate of left-bottom corner point
 * @param x_rt Coordinate of right-top corner point
 * @return bool True if point inside rectangle, else false
 */
bool isPointInsideRectangle(util::Point x, util::Point x_lb,
        util::Point x_rt);

/*!
 * @brief Checks if point is inside an angled rectangle
 * @param x Point
 * @param x_min X coordinate of left-bottom corner point
 * @param x_max X coordinate of right-top corner point
 * @param y_min Y coordinate of left-bottom corner point
 * @param y_max Y coordinate of right-top corner point
 * @param theta Angle of orientation of rectangle from x-axis
 * @return bool True if point inside rectangle, else false
 */
bool isPointInsideAngledRectangle(util::Point x, double x_min, double x_max,
                                  double y_min, double y_max, double theta);

/*!
 * @brief Checks if point is inside a cuboid
 * @param x Point
 * @param x_lbb Coordinate of left-bottom-back corner point
 * @param x_rtf Coordinate of right-top-front corner point
 * @return bool True if point inside cuboid, else false
 */
bool isPointInsideCuboid(util::Point x, util::Point x_lbb,
                            util::Point x_rtf);

/*!
 * @brief Returns true if point is inside the cylinder
 *
 * @param p Point
 * @param length Length of cylinder
 * @param radius Radius of cylinder
 * @param axis Axis of cylinder
 * @return True If point inside cylinder otherwise false
 */
bool isPointInsideCylinder(const util::Point &p, const double &length, const double
&radius, const util::Point &axis);

/*!
 * @brief Returns true if point is inside the cylinder
 *
 * @param p Point
 * @param radius Radius of cylinder
 * @param x1 Point at the center of cross-section at s=0
 * @param x2 Point at the center of cross-section at s=L
 * @return True If point inside cylinder otherwise false
 */
bool isPointInsideCylinder(const util::Point &p, const double &radius, const util::Point &x1,
                        const util::Point &x2);

/*!
 * @brief Returns true if point is inside the ellipsoid
 *
 * @param p Point
 * @param center Center of ellipse
 * @param radius_vec Vector of radius describing ellipse
 * @param dim Dimension
 * @return True If point inside otherwise false
 */
bool isPointInsideEllipse(const util::Point &p, const util::Point &center, const
std::vector<double> &radius_vec, unsigned int dim);

/*!
 * @brief Returns true if point is inside the ellipsoid
 *
 * Also computes
 * d = x^2 / r1^2 + y^2 / r2^2 + z^2 / r3^2
 *
 * @param p Point
 * @param center Center of ellipse
 * @param radius_vec Vector of radius describing ellipse
 * @param dim Dimension
 * @param d
 * @return True If point inside otherwise false
 */
bool isPointInsideEllipse(const util::Point &p, const util::Point &center, const
std::vector<double> &radius_vec, unsigned int dim, double &d);

/** @}*/

/**
   * @name Methods to check intersection of various objects
   */
/**@{*/

/*!
   * @brief Checks if given two boxes are within given distance from each other
   * @param b1 Box 1
   * @param b2 Box 2
   * @param tol Tolerance for checking
   * @param dim Dimension of the objects
   * @return True If within given distance
   *
   * @note TODO This is not precise and can be improved.
   */
bool areBoxesNear(const std::pair<util::Point, util::Point> &b1,
                  const std::pair<util::Point, util::Point> &b2,
                  const double &tol,
                  size_t dim);

/*!
 * @brief Do lines intersect
 *
 * @param line_1 Line 1
 * @param line_2 Line 2
 * @return True If lines intersect, else false
 */
bool doLinesIntersect(const std::pair<util::Point, util::Point> &line_1,
                     const std::pair<util::Point, util::Point> &line_2);

/*!
 * @brief Compute distance between lines
 *
 * @param line_1 Line 1
 * @param line_2 Line 2
 * @return Value Distance
 */
double distanceBetweenLines(const std::pair<util::Point, util::Point> &line_1,
                              const std::pair<util::Point, util::Point> &line_2);
double
distanceBetweenSegments(const std::pair<util::Point, util::Point> &line_1,
                        const std::pair<util::Point, util::Point> &line_2);

/*!
 * @brief Compute distance between planes
 *
 * @param plane_1 Plane 1 given by pair of normal and one point which it
 * contains
 * @param plane_2 Plane 2 given by pair of normal and one point which it
 * contains
 * @return Value Distance
 */
double distanceBetweenPlanes(const std::pair<util::Point, util::Point> &plane_1,
                               const std::pair<util::Point, util::Point> &plane_2);

/*!
 * @brief Compute distance between point and line
 *
 * @param p Point
 * @param line Line
 * @return Value Distance
 */
double pointDistanceLine(const util::Point &p,
                           const std::pair<util::Point, util::Point> &line);
double pointDistanceSegment(const util::Point &p,
                              const std::pair<util::Point, util::Point> &line);

/*!
 * @brief Compute distance between point and plane
 *
 * @param p Point
 * @param plane Plane given by pair of normal and one point which it
 * contains
 * @return Value Distance
 */
double pointDistancePlane(const util::Point &p,
                            const std::pair<util::Point, util::Point> &plane);


/** @}*/

/*!
 * @brief Returns point in line formed by points p1 and p2
 *
 * @param p1 Point 1
 * @param p2 Point 2
 * @param s Parametric coordinate
 * @return p Point
 */
util::Point getPointOnLine(const util::Point &p1, const util::Point &p2, const double &s);

/*!
 * @brief Computes minimum distance between any two nodes
 *
 * @param nodes List of nodal coordinates
 * @return h Minimum distance
 */
double computeMeshSize(const std::vector<util::Point> &nodes);

/*!
 * @brief Computes minimum distance between any two nodes
 *
 * This only considers the nodes between start and end
 *
 * @param nodes List of nodal coordinates
 * @param start Start index from where to start
 * @param end End index
 * @return h Minimum distance
 */
double computeMeshSize(const std::vector<util::Point> &nodes, size_t start,
                       size_t end);

/*!
 * @brief Computes bounding box for vector nodes
 *
 * @param nodes List of nodal coordinates
 * @return Box Pair of corner points of box
 */
std::pair<util::Point, util::Point> computeBBox(const
                                                  std::vector<util::Point> &nodes);

/*!
 * @brief Computes maximum radius of circle/sphere within a given box
 *
 * @param box Pair of corner points of box
 * @return Radius Radius of circle/sphere
 */
double computeInscribedRadius(const std::pair<util::Point, util::Point>
    &box);

/*!
 * @brief Create box from two coordinate data
 *
 * @param p1 Point 1
 * @param p2 Point 2
 * @return Box Pair of corner points of box
 */
std::pair<util::Point, util::Point> toPointBox(const std::vector<double>
    &p1, const std::vector<double>
&p2);

/*!
 * @brief Compute area of triangle
 *
 * @param x1 Vertex 1
 * @param x2 Vertex 2
 * @param x3 Vertex 3
 * @return area Area of triangle
 */
double triangleArea(const util::Point &x1, const util::Point &x2,
                    const util::Point &x3);

} // namespace util

#endif // UTIL_GEOMETRY_H
