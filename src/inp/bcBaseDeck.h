/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_BCBASEDECK_H
#define INP_BCBASEDECK_H

#include "geom/geomIncludes.h"
#include "util/json.h"

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief User-input data for particle neighbor search */
  struct BCBaseDeck {
    /*! @brief Method for applying force. E.g., Force_BC, Displacement_BC, IC */
    std::string d_type;

    /*! @brief Method for applying force
     * e.g.
     *
     * - particle: provides global id of particle to which force should be
     * applied
     * - wall: provides global id of wall to which force should be applied
     * - region: provides area within which all nodes of any particle will get
     * this force
     */
    std::string d_selectionType;

    /*!
     * @brief Flag that indicates if region-based application of boundary condition is active.
     * So cases of 'region', 'region_with_include_list', 'region_with_exclude_list', and
     * 'region_with_include_list_with_exclude_list' will have region-based application of boundary condition.
     */
    bool d_isRegionActive;

    /*! @brief Region geometry (if any) */
    geom::GeomData d_regionGeomData;

    /*! @brief List of particles (if any) */
    std::vector<size_t> d_pList;

    /*! @brief List of particles to not include (if any) */
    std::vector<size_t> d_pNotList;

    /*!
     * @brief Name of the formula with respect to time
     *
     * List of allowed values are:
     * - "" (none)
     * - \a constant
     * - \a linear
     * - \a linear_step
     * - \a linear_slow_fast
     * - \a rotation
     */
    std::string d_timeFnType;

    /*!
     * @brief Name of the formula of with respect to spatial coordinate
     *
     * List of allowed values are:
     * - "" (none)
     * - \a constant
     * - \a hat_x
     * - \a hat_y
     * - \a sin
     * - \a rotation
     */
    std::string d_spatialFnType;

    /*!
     * @brief List of dofs on which this bc will be applied
     *
     * E.g. if bc is only applied on x-component, d_direction will be 1. If
     * bc is applied on x- and y-component, d_direction will be vector with
     * elements 1 and 2.
     */
    std::vector<size_t> d_direction;

    /*! @brief List of parameters for function wrt time */
    std::vector<double> d_timeFnParams;

    /*! @brief List of parameters for function wrt spatial coordinate */
    std::vector<double> d_spatialFnParams;

    /*! @brief Specify if this bc corresponds to zero displacement condition */
    bool d_isDisplacementZero;

    /*! @brief Type */
    std::string d_icType;

    /*! @brief Initial velocity vector */
    util::Point d_icVec;

    /*!
     * @brief Constructor
     */
    BCBaseDeck(const json &j = json({}), std::string type = "Force_BC") :
        d_type(type), d_isRegionActive(false), d_isDisplacementZero(false) {
      readFromJson(j, type);
    };

    /*!
     * @brief Constructor
     */
    BCBaseDeck(bool isRegionActive,
               geom::GeomData regionGeomData = geom::GeomData(),
               std::vector<size_t> pList = std::vector<size_t>(),
               std::vector<size_t> pNotList = std::vector<size_t>(),
               std::string timeFnType = "",
               std::vector<double> timeFnParams = std::vector<double>(),
               std::string spatialFnType = "",
               std::vector<double> spatialFnParams = std::vector<double>(),
               std::vector<size_t> direction = std::vector<size_t>(),
               bool isDisplacementZero = false)
        : d_isRegionActive(isRegionActive), d_regionGeomData(regionGeomData),
          d_pList(pList), d_pNotList(pNotList), d_timeFnType(timeFnType), d_timeFnParams(timeFnParams),
          d_spatialFnType(spatialFnType), d_spatialFnParams(spatialFnParams),
          d_direction(direction), d_isDisplacementZero(isDisplacementZero) {

      // fix selection type
      if (d_isRegionActive) {
        d_selectionType = "region";
        d_isRegionActive = true;
        if (d_pList.size() > 0)
          d_selectionType += "_with_include_list";

        if (d_pNotList.size() > 0)
          d_selectionType += "_with_exclude_list";
      } else {
        if (d_pList.size() > 0)
          d_selectionType = "particle";
      }

      if (d_isRegionActive) {
        // create a geometry object based on the data
        geom::createGeomObject(d_regionGeomData);
      }
    };


    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    static json getExampleJson(std::string type = "Foce_BC",
                                bool isRegionActive = false,
                               geom::GeomData regionGeomData = geom::GeomData(),
                               std::vector<size_t> pList = std::vector<size_t>(),
                               std::vector<size_t> pNotList = std::vector<size_t>(),
                               std::string timeFnType = "",
                               std::vector<double> timeFnParams = std::vector<double>(),
                               std::string spatialFnType = "",
                               std::vector<double> spatialFnParams = std::vector<double>(),
                               std::vector<size_t> direction = std::vector<size_t>(),
                               bool isDisplacementZero = false,
                               std::string icType = "",
                               std::vector<double> icVec = std::vector<double>()) {

      auto j = json({});

      if (isRegionActive) {
        // get json object for geometry
        auto j_geom = json({});
        geom::writeGeometry(j_geom, regionGeomData);
        j["Region"] = {"Geometry", j_geom};
      }

      if (pList.size() > 0) j["Particle_List"] = pList;

      if (pNotList.size() > 0) j["Particle_Exclude_List"] = pNotList;

      if (timeFnType != "") {
        j["Time_Function"] = {"Type", timeFnType};
        if (timeFnParams.size() > 0)
          j["Time_Function"] = {"Parameters", timeFnParams};
      }

      if (spatialFnType != "") {
        j["Spatial_Function"] = {"Type", spatialFnType};
        if (spatialFnParams.size() > 0)
          j["Spatial_Function"] = {"Parameters", spatialFnParams};
      }

      if (type != "IC") {
        if (direction.size() > 0)
          j["Direction"] = direction;

        if (isDisplacementZero) {
          j["Zero_Displacement"] = true;
        }
      }

      if (type == "IC") {
        if (icType == "Constant_Velocity") {
          j[icType]["Velocity_Vector"] = icVec;
        }
      }

      return j;
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j, std::string type) {
      if (j.empty())
        return;

      // fix selection type
      if (j.find("Region") != j.end()) {
        d_selectionType = "region";
        d_isRegionActive = true;
        if (j.find("Particle_List") != j.end())
          d_selectionType += "_with_include_list";

        if (j.find("Particle_Exclude_List") != j.end())
          d_selectionType += "_with_exclude_list";
      } else {
        d_isRegionActive = false;
        if (j.find("Particle_List") != j.end())
          d_selectionType = "particle";
      }

      if (d_isRegionActive) {
        geom::readGeometry(j.at("Region").at("Geometry"), d_regionGeomData);
        // create a geometry object based on the data
        geom::createGeomObject(d_regionGeomData);
      }

      if (j.find("Particle_List") != j.end()) d_pList = j.value("Particle_List", std::vector<size_t>());

      if (j.find("Particle_Exclude_List") != j.end())
        d_pNotList = j.value("Particle_Exclude_List", std::vector<size_t>());

      if (j.find("Time_Function") != j.end()) {
        d_timeFnType = j.at("Time_Function").value("Type", "");
        if (j.at("Time_Function").find("Parameters") != j.at("Time_Function").end())
          d_timeFnParams = j.at("Time_Function").value("Parameters", std::vector<double>());
      }

      if (j.find("Spatial_Function") != j.end()) {
        d_spatialFnType = j.at("Spatial_Function").value("Type", "");
        if (j.at("Spatial_Function").find("Parameters") != j.at("Spatial_Function").end())
          d_spatialFnParams = j.at("Spatial_Function").value("Parameters", std::vector<double>());
      }

      if (type != "IC") {
        if (j.find("Direction") == j.end()) {
          throw std::runtime_error("Direction must be specified for boundary condition");
          return;
        }

        d_direction = j.value("Direction", std::vector<size_t>());

        if (j.find("Zero_Displacement") != j.end())
          d_isDisplacementZero = j.at("Zero_Displacement");
      }

      // if it is initial condition
      if (type == "IC") {
        if (j.find("Constant_Velocity") != j.end()) {
          d_icType = "Constant_Velocity";
          d_icVec = util::Point(j.at("Constant_Velocity").value("Velocity_Vector", std::vector<double>({0., 0., 0.})));
        }
      }
    }

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
      oss << tabS << "------- BCBaseDeck --------" << std::endl << std::endl;
      oss << tabS << "Selection type  = " << d_selectionType << std::endl;
      oss << tabS << "Is region active  = " << d_isRegionActive << std::endl;
      if (d_regionGeomData.d_geom_p != nullptr) {
        oss << tabS << "Region geometry info: " << std::endl;
        oss << d_regionGeomData.printStr(nt + 1, lvl);
      }
      if (!d_pList.empty())
        oss << tabS << "Particle list = ["
            << util::io::printStr<size_t>(d_pList, 0) << "]" << std::endl;
      if (!d_pNotList.empty())
        oss << tabS << "Particle excluded list = ["
            << util::io::printStr<size_t>(d_pNotList, 0) << "]" << std::endl;
      oss << tabS << "Time function type = " << d_timeFnType << std::endl;
      oss << tabS << "Time function parameters = ["
          << util::io::printStr<double>(d_timeFnParams, 0) << "]" << std::endl;
      oss << tabS << "Spatial function type = " << d_spatialFnType << std::endl;
      oss << tabS << "Spatial function parameters = ["
          << util::io::printStr<double>(d_spatialFnParams, 0) << "]" << std::endl;
      oss << tabS << "Direction = [" << util::io::printStr<size_t>(d_direction, 0)
          << "]" << std::endl;
      oss << tabS << "Is displacement zero = " << d_isDisplacementZero << std::endl;
      oss << tabS << std::endl;

      return oss.str();
    }

    /*!
     * @brief Prints the information about the object
     *
     * @param nt Number of tabs to append before printing
     * @param lvl Information level (higher means more information)
     */
    void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
  };

/** @}*/

} // namespace inp

#endif // INP_BCBASEDECK_H
