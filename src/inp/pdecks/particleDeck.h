/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_PARTICLEDECK_H
#define INP_PARTICLEDECK_H

#include "zoneDeck.h"
#include "pNeighborDeck.h"
#include "pBCData.h"
#include "pICDeck.h"
#include "util/geomObjectsUtil.h"
#include <memory>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/



/*! @brief Structure to read and store particle related input data */
struct ParticleDeck {

  /*! @brief All zones */
  std::vector<inp::Zone> d_zoneVec;

  /*! @brief Maps particle/wall to corresponding zone */
  std::vector<std::pair<std::string, size_t>> d_zoneToParticleORWallDeck;

  /*! @brief Particle in zones */
  std::vector<inp::ParticleZone> d_particleZones;

  /*!
   * @brief Geometry of container in which all particles reside. Currently,
   * we only support rectangle (2-d) and cuboid (3-d)
   */
  util::geometry::GeomData d_contGeomData;

  /*! @brief Neighbor search data */
  inp::PNeighborDeck d_pNeighDeck;

  /*! @brief Gravity loading value */
  bool d_gravityActive;

  /*! @brief Gravity loading value */
  util::Point d_gravity;

  /*! @brief Force loading deck */
  std::vector<inp::PBCData> d_forceDeck;

  /*! @brief Displacement loading deck */
  std::vector<inp::PBCData> d_dispDeck;

  /*! @brief Initial condition deck */
  inp::PICDeck d_icDeck;

  /*! @brief Specify test name (if any) */
  std::string d_testName;

  /*! @brief if it is a compressive test, specify wall id and direction */
  size_t d_particleIdCompressiveTest;

  /*! @brief if it is a compressive test, specify force direction on wall */
  size_t d_particleForceDirectionCompressiveTest;

  /*!
   * @brief Constructor
   */
  ParticleDeck()
      : d_contGeomData(),
        d_pNeighDeck(),
        d_gravityActive(false),
        d_gravity(),
        d_icDeck(),
        d_testName(""),
        d_particleIdCompressiveTest(0),
        d_particleForceDirectionCompressiveTest(0) {};

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
    oss << tabS << "------- ParticleDeck --------" << std::endl << std::endl;
    oss << tabS << "Number of particle zones  = " << d_particleZones.size() << std::endl;
    oss << tabS << "Particle data:" << std::endl;
    for (size_t i = 0; i < d_particleZones.size(); i++) {
      oss << tabS << "Particle data for zone = " << i << std::endl;
      oss << d_particleZones[i].printStr(nt + 1, lvl);
    }
    oss << tabS << "Container geometry details:" << std::endl;
    oss << d_contGeomData.printStr(nt+1, lvl);
    oss << tabS << "Neighbor data:" << std::endl;
    oss << d_pNeighDeck.printStr(nt+1, lvl);
    oss << tabS << "Gravity: Status = " << d_gravityActive
        << ", force = " << d_gravity.printStr(0, 0) << std::endl;
    oss << tabS << "Num of Force BC = " << d_forceDeck.size() << std::endl;
    oss << tabS << "Force BC info:" << std::endl;
    size_t bc_count = 0;
    for (const auto &f: d_forceDeck) {
      oss << tabS << "  Force BC id = " << bc_count++ << std::endl;
      oss << f.printStr(nt+2, lvl);
    }
    oss << tabS << "Num of Displacement BC = " << d_dispDeck.size() <<
    std::endl;
    oss << tabS << "Displacement BC info:" << std::endl;
    bc_count = 0;
    for (const auto &f: d_dispDeck) {
      oss << tabS << "  Displacement BC id = " << bc_count++ << std::endl;
      oss << f.printStr(nt+2, lvl);
    }
    oss << tabS << "IC data:" << std::endl;
    oss << d_icDeck.printStr(nt+1, lvl);
    oss << tabS << "Test name = " << d_testName << std::endl;
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

#endif // INP_PARTICLEDECK_H
