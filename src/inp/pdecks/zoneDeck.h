/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_P_ZONEDECK_H
#define INP_P_ZONEDECK_H

#include "inp/decks/materialDeck.h"
#include "inp/decks/meshDeck.h"
#include "util/geomObjects.h" // geometrical objects
#include "util/io.h"
#include <memory>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief User-input data for zones */
struct Zone {

  /*! @brief Zone type */
  std::string d_geomName;

  /*! @brief Zone parameters */
  std::vector<double> d_geomParams;

  /*! @brief Zone geometry */
  std::shared_ptr<util::geometry::GeomObject> d_geom_p;

  /*! @brief Specify zone to which this particle belongs to */
  size_t d_zoneId;

  /*!
   * @brief Constructor
   */
  Zone() : d_geom_p(nullptr){};

  /*!
   * @brief Constructor
   *
   * @param z Another zone object
   */
  Zone(const Zone &z)
      : d_geomName(z.d_geomName), d_geomParams(z.d_geomParams), d_geom_p(z.d_geom_p),
        d_zoneId(z.d_zoneId){};

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
    oss << tabS << "------- Zone --------" << std::endl << std::endl;
    oss << tabS << "Type = " << d_geomName << ", Zone id = " << d_zoneId
        << std::endl;
    oss << tabS << "Parameters = [" << util::io::printStr<double>(d_geomParams, 0)
        << "]" << std::endl;
    oss << d_geom_p->printStr(nt+1, lvl);

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

/*! @brief User-input data for particle zone */
struct ParticleZone {

  /*! @brief Zone data */
  inp::Zone d_zone;

  /*!
   * @brief Particle information. E.g., "rigid".
   * If nothing specific is available, value will be empty string
   */
  std::string d_particleDescription;

  /*! @brief Is this particle actually a wall? */
  bool d_isWall;

  /*!
   * @brief geometry of particle. Currently, we only support circle (2-d)
   * and sphere (3-d).
   *
   *  For wall, geometry can be "flat", "circular", "spherical" etc.
   */
  std::shared_ptr<util::geometry::GeomObject> d_geom_p;

  /*! @brief Particle parameters
   *
   * In case of circle, sphere, cylinder, the first parameter must be
   * radius.
   */
  std::vector<double> d_geomParams;

  /*!
   * @brief Particle generation method
   *
   * "from_file" means particle location, radius and other details will be
   * loaded from the input .csv file
   */
  std::string d_genMethod;

  /*! @brief Specify what data to be expected in the particle file
   * e.g.
   * - loc_rad : location and radius data
   * - loc_rad_orient: location, radius, and orientation
   *
   * By default, zone_id of particle will be there. Total data in each row
   * will be 5 for "loc_rad" (1 zone, 3 location, 1 radius). For
   * "loc_rad_orient", 6 data will be expected.
   */
  std::string d_particleFileDataType;

  /*! @brief Read particle from a file */
  std::string d_particleFile;

  /*!
   * @brief Reference particle information
   */
  std::shared_ptr<util::geometry::GeomObject> d_refParticleGeom_p;

  /*! @brief Reference particle parameters
   *
   * In case of circle, sphere, cylinder, the first parameter must be
   * radius.
   */
  std::vector<double> d_refParticleGeomParams;

  /*! @brief Store material information */
  inp::MaterialDeck d_matDeck;

  /*! @brief Store mesh information */
  inp::MeshDeck d_meshDeck;

  /*! @brief Specify if we mesh particle (intended to handle rigid wall in future) */
  bool d_meshFlag;

  /*! @brief Specify if all dofs are constrained */
  bool d_allDofsConstrained;

  /*! @brief Specify how deep we search for nodes near boundary for contact calculations */
  double d_nearBdNodesTol;

  /*!
   * @brief Constructor
   */
  ParticleZone()
      : d_zone(inp::Zone()),
        d_particleDescription(""),
        d_isWall(false),
        d_geom_p(nullptr),
        d_geomParams(),
        d_genMethod(""),
        d_particleFileDataType(""),
        d_particleFile(""),
        d_refParticleGeom_p(nullptr),
        d_refParticleGeomParams(),
        d_matDeck(inp::MaterialDeck()),
        d_meshDeck(inp::MeshDeck()),
        d_meshFlag(true),
        d_allDofsConstrained(false),
        d_nearBdNodesTol(0.5)
        {};

  /*!
   * @brief Copy constructor
   *
   * @param pz Another ParticleZone object
   */
  ParticleZone(const ParticleZone &pz)
      : d_zone(pz.d_zone),
        d_particleDescription(pz.d_particleDescription),
        d_isWall(pz.d_isWall),
        d_geom_p(pz.d_geom_p),
        d_geomParams(pz.d_geomParams),
        d_genMethod(pz.d_genMethod),
        d_particleFileDataType(pz.d_particleFileDataType),
        d_particleFile(pz.d_particleFile),
        d_refParticleGeom_p(pz.d_refParticleGeom_p),
        d_refParticleGeomParams(pz.d_refParticleGeomParams),
        d_matDeck(pz.d_matDeck),
        d_meshDeck(pz.d_meshDeck),
        d_meshFlag(pz.d_meshFlag),
        d_allDofsConstrained(pz.d_allDofsConstrained),
        d_nearBdNodesTol(pz.d_nearBdNodesTol)
        {};

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
    oss << tabS << "------- ParticleZone --------" << std::endl << std::endl;
    oss << tabS << "Zone info: " << std::endl;
    oss << d_zone.printStr(nt+1, lvl);
    oss << tabS << "Particle type = " << d_particleDescription << std::endl;
    oss << tabS << "Generation method = " << d_genMethod << std::endl;
    oss << tabS << "Bdry nodes tol = " << d_nearBdNodesTol << std::endl;
    oss << tabS << "Mesh flag = " << d_meshFlag << std::endl;
    oss << tabS << "All dofs constrained = " << d_allDofsConstrained << std::endl;
    oss << tabS << "Particle: " << std::endl;
    oss << tabS << "Parameters = [ " << util::io::printStr<double>(d_geomParams, 0)
        << "]" << std::endl;
    oss << d_geom_p->printStr(nt + 1, lvl);
    oss << tabS << "Reference particle: " << std::endl;
    oss << tabS << "Parameters = ["
        << util::io::printStr<double>(d_refParticleGeomParams, 0) << "]" << std::endl;
    oss << d_refParticleGeom_p->printStr(nt + 1, lvl);
    oss << d_matDeck.printStr(nt+1, lvl);
    oss << d_meshDeck.printStr(nt+1, lvl);
    oss << std::endl;

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

} // namespace inp

#endif // INP_P_ZONEDECK_H
