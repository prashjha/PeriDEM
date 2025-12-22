/*
* -------------------------------------------
* Copyright (c) 2021 - 2024 Prashant K. Jha
* -------------------------------------------
* PeriDEM https://github.com/prashjha/PeriDEM
*
* Distributed under the Boost Software License, Version 1.0. (See accompanying
* file LICENSE)
*/

#pragma once

#include "geomObjects.h"

namespace util {

/*! @brief Provides geometrical methods such as point inside rectangle */
namespace geometry {

/*! @brief Input data for geometrical objects */
struct GeomData {

/*! @brief Zone type */
std::string d_geomName;

/*! @brief Zone parameters */
std::vector<double> d_geomParams;

/*! @brief Zone geometry */
std::shared_ptr<util::geometry::GeomObject> d_geom_p;

/*! @brief Zone geometry info if it is a complex type */
std::pair<std::vector<std::string>, std::vector<std::string>> d_geomComplexInfo;

/*!
 * @brief Constructor
 */
GeomData() : d_geom_p(nullptr) {};

/*!
 * @brief Constructor
 *
 * @param z Another GeomData object
 */
GeomData(const GeomData &z)
        : d_geomName(z.d_geomName),
          d_geomParams(z.d_geomParams),
          d_geom_p(z.d_geom_p),
          d_geomComplexInfo(z.d_geomComplexInfo) {};

/*!
 * @brief Creates NullGeomObject
 * @param description Description (if any)
 */
void createNullGeomObject(std::string description = "") {
  d_geomName = "null";
  d_geomParams.resize(0);
  d_geom_p = std::make_shared<util::geometry::NullGeomObject>(
          description);
};

/*!
 * @brief Copies the geometry details
 *
 * @param z Another GeomData object that will be modified
 * @param dim Dimension of the object
 */
void copyGeometry(GeomData &z, size_t dim);

/*!
* @brief Copies the geometry details
*
* @param name Name of geometry
* @param params Parameters
* @param complexInfo Pair of vector of geometry names and flags for complex geometry
* @param geom Pointer to geometry object
* @param dim Dimension of the object
*/
void copyGeometry(std::string &name,
                  std::vector<double> &params,
                  std::pair<std::vector<std::string>, std::vector<std::string>> &complexInfo,
                  std::shared_ptr<util::geometry::GeomObject> &geom,
                  size_t dim);

/*!
 * @brief Returns the string containing printable information about the object
 *
 * @param nt Number of tabs to append before printing
 * @param lvl Information level (higher means more information)
 * @return string String containing printable information about the object
 */
std::string printStr(int nt = 0, int lvl = 0) const {

  auto tabS = util::io::getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "------- GeomData --------" << std::endl
      << std::endl;
  oss << tabS << "Type = " << d_geomName << std::endl;
  oss << tabS << "Parameters = ["
      << util::io::printStr<double>(d_geomParams, 0)
      << "]" << std::endl;
  if (!d_geomComplexInfo.first.empty()) {
    oss << tabS << "Vec type for complex geometry = ["
        << util::io::printStr(d_geomComplexInfo.first, 0)
        << "]" << std::endl;

    oss << tabS << "Vec flag for complex geometry = ["
        << util::io::printStr(d_geomComplexInfo.second, 0)
        << "]" << std::endl;
  }
  if (d_geom_p != nullptr)
    oss << d_geom_p->printStr(nt + 1, lvl);

  return oss.str();
}

/*!
 * @brief Prints the information about the object
 *
 * @param nt Number of tabs to append before printing
 * @param lvl Information level (higher means more information)
 */
void print(int nt = 0, int lvl = 0) const {
  std::cout << printStr(nt, lvl);
}
}; // struct GeomData

/*! @brief List of acceptable geometries for particles in PeriDEM */
const std::vector<std::string> acceptable_geometries = {"circle",
                                                    "square",
                                                    "rectangle",
                                                    "hexagon",
                                                    "triangle",
                                                    "drum2d",
                                                    "sphere",
                                                    "cube",
                                                    "cuboid",
                                                    "ellipse"};

/*! @brief Returns list of acceptable geometries for PeriDEM simulation */
inline const std::vector<std::string> &getAcceptableGeometries() {
return acceptable_geometries;
};

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
  for (const auto &j: d_nodes)
    if (j == i)
      return;

  d_nodes.push_back(i);
}
};

/*!
* @brief Get num params required for creation of object
*
* @param geom_type Geometry type of object
* @return n Number of parameters required
*/
std::vector<size_t> getNumParamsRequired(std::string geom_type);

/*!
* @brief Check parameter data for validity
*
* @param n Number of parameters available
* @param geom_type Geometry type of object
* @return bool True if number of parameter is incorrect so require further checks
*/
bool checkParamForGeometry(size_t n, std::string geom_type);

/*!
* @brief Ascertain if number of parameters are correct for the given geometry
*
* @param n Number of parameters available
* @param geom_type Geometry type of object
* @return bool True if number of parameter is correct
*/
bool isNumberOfParamForGeometryValid(size_t n, std::string geom_type);

/*!
* @brief Check parameter data for validity
*
* @param n Number of parameters available
* @param geom_type Geometry type of object
* @param vec_type For complex objects, specify types of sub-objects
* @return bool True if number of parameter is incorrect so require further checks
*/
bool checkParamForComplexGeometry(size_t n, std::string geom_type,
                              std::vector<std::string> vec_type);

/*!
* @brief Ascertain if number of parameters are correct for the given geometry
*
* @param n Number of parameters available
* @param geom_type Geometry type of object
* @param vec_type For complex objects, specify types of sub-objects
* @return bool True if number of parameter is correct
*/
bool
isNumberOfParamForComplexGeometryValid(size_t n, std::string geom_type,
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
void createGeomObjectOld(const std::string &type,
                     const std::vector<double> &params,
                     const std::vector<std::string> &vec_type,
                     const std::vector<std::string> &vec_flag,
                     std::shared_ptr<util::geometry::GeomObject> &obj,
                     const size_t &dim,
                     bool perform_check = true);

void createGeomObject(const std::string &geom_type,
                  const std::vector<double> &params,
                  const std::vector<std::string> &vec_type,
                  const std::vector<std::string> &vec_flag,
                  std::shared_ptr<util::geometry::GeomObject> &obj,
                  const size_t &dim,
                  bool perform_check = true);


void createGeomObject(GeomData &geomData,
                  const size_t &dim,
                  bool perform_check = true);

} // namespace geometry

} // namespace util

