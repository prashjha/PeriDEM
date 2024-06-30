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
#include "util/geomObjectsUtil.h"
#include <memory>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief User-input data for zones */
struct Zone {

  /*! @brief Zone geometry data */
  util::geometry::GeomData d_zoneGeomData;

  /*! @brief Specify zone to which this particle belongs to */
  size_t d_zoneId;

  /*!
   * @brief Constructor
   */
  Zone() : d_zoneGeomData(){};

  /*!
   * @brief Constructor
   *
   * @param z Another zone object
   */
  Zone(const Zone &z)
      : d_zoneGeomData(z.d_zoneGeomData),
        d_zoneId(z.d_zoneId) {};

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
    oss << tabS << "Zone id = " << d_zoneId << std::endl;
    oss << tabS << "Zone geometry data: " << std::endl;
    oss << d_zoneGeomData.printStr(nt+1, lvl);

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

  /*! @brief Geometry of details of particle */
  util::geometry::GeomData d_particleGeomData;

  /*! @brief Geometry of details of reference particle */
  util::geometry::GeomData d_refParticleGeomData;

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
   * @brief Specify if the particle should be created using the particle
   * geometry in the zone data and mesh file. I.e., we will not expect
   * location information from the particle generation file for this particle
   * and create particle in this zone using the particle geometry object 'd_geom_p' and
   * use identity transform
   */
  bool d_createParticleUsingParticleZoneGeomObject;

  /*!
   * @brief Constructor
   */
  ParticleZone()
      : d_zone(inp::Zone()),
        d_particleDescription(""),
        d_isWall(false),
        d_particleGeomData(),
        d_refParticleGeomData(),
        d_genMethod(""),
        d_particleFileDataType(""),
        d_particleFile(""),
        d_matDeck(),
        d_meshDeck(),
        d_meshFlag(true),
        d_allDofsConstrained(false),
        d_nearBdNodesTol(0.5),
        d_createParticleUsingParticleZoneGeomObject (false)
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
        d_particleGeomData(pz.d_particleGeomData),
        d_refParticleGeomData(pz.d_refParticleGeomData),
        d_genMethod(pz.d_genMethod),
        d_particleFileDataType(pz.d_particleFileDataType),
        d_particleFile(pz.d_particleFile),
        d_matDeck(pz.d_matDeck),
        d_meshDeck(pz.d_meshDeck),
        d_meshFlag(pz.d_meshFlag),
        d_allDofsConstrained(pz.d_allDofsConstrained),
        d_nearBdNodesTol(pz.d_nearBdNodesTol),
        d_createParticleUsingParticleZoneGeomObject(pz.d_createParticleUsingParticleZoneGeomObject)
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
    oss << tabS << "d_createParticleUsingParticleZoneGeomObject = " << d_createParticleUsingParticleZoneGeomObject << std::endl;
    oss << tabS << "Particle geometry details: " << std::endl;
    oss << d_particleGeomData.printStr(nt+1, lvl);
    oss << tabS << "Reference rarticle geometry details: " << std::endl;
    oss << d_refParticleGeomData.printStr(nt+1, lvl);
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
