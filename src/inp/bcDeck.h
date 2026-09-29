/*
* -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_BCDECK_H
#define INP_BCDECK_H

#include <string>

#include "bcBaseDeck.h"
#include "util/io.h"
#include "util/json.h"

namespace inp {
  /**
   * \ingroup Input
   */
  /**@{*/

  /*! @brief Structure to read and store test-related input data */
  struct BCDeck {

    /*! @brief Force loading deck */
    std::vector<inp::BCBaseDeck> d_forceDeck;

    /*! @brief Displacement loading deck */
    std::vector<inp::BCBaseDeck> d_dispDeck;

    /*! @brief Initial condition deck */
    std::vector<inp::BCBaseDeck> d_icDeck;

    /*! @brief Gravity loading value */
    bool d_gravityActive;

    /*! @brief Gravity loading value */
    util::Point d_gravity;


    /*!
     * @brief Constructor
     */
    BCDeck(const json &j = json({}))
      : d_gravityActive(false) {
      readFromJson(j);
    };

    /*!
     * @brief Constructor
     */
    BCDeck(size_t nForceSets, size_t nDispSets = 0, size_t nICSets = 0,
           bool gravityActive = false, util::Point gravity = util::Point())
        : d_gravityActive(gravityActive), d_gravity(gravity) {

        d_forceDeck.resize(nForceSets);
        d_dispDeck.resize(nDispSets);
        d_icDeck.resize(nICSets);
    };

    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    /*!
     * @brief Returns the block with the given fields set
     *
     * Each of the three kinds of set is asked for by its count. Naming
     * Gravity is what turns gravity on; it is a force on every particle and
     * so sits beside the force sets rather than replacing them.
     *
     * @param given Force_BC_Sets, Displacement_BC_Sets, IC_Sets and Gravity
     * @return JSON object for this deck
     */
    static json getExampleJson(const json &given = json::object()) {
      checkNames(given, {"Force_BC_Sets", "Displacement_BC_Sets", "IC_Sets",
                         "Gravity"});

      const auto nForceSets = given.value("Force_BC_Sets", size_t(0));
      const auto nDispSets = given.value("Displacement_BC_Sets", size_t(0));
      const auto nICSets = given.value("IC_Sets", size_t(0));
      const bool gravityActive = given.find("Gravity") != given.end();

      if (nForceSets + nDispSets + nICSets == 0 and !gravityActive)
        return json({});

      auto j = json({});

      if (gravityActive)
        j["Force_BC"]["Gravity"] = given.at("Gravity");

      if (nForceSets > 0)
        j["Force_BC"]["Sets"] = nForceSets;

      if (nDispSets > 0)
        j["Displacement_BC"] = {{"Sets", nDispSets}};

      if (nICSets > 0)
        j["IC"] = {{"Sets", nICSets}};

      return j;
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {

      if (j.empty())
        return;

      // gravity
      if (j.find("Force_BC") != j.end()) {
        if (j.at("Force_BC").find("Gravity") != j.at("Force_BC").end()) {
          d_gravityActive = true;
          d_gravity = util::Point(j.at("Force_BC").value("Gravity", std::vector<double>({0., 0., 0.})));
        }
      }

      // process Force_BC
      d_forceDeck.clear();
      d_dispDeck.clear();
      d_icDeck.clear();
      for (auto tag : std::vector<std::string>({"Force_BC", "Displacement_BC", "IC"})) {
        if (j.find(tag) == j.end())
          continue;

        auto nSets = j.at(tag).value("Sets", 0);

        for (size_t i = 0; i < nSets; i++) {

          auto set = j.at(tag).at("Set_" + std::to_string(i + 1));

          if (tag == "Force_BC") {
            inp::BCBaseDeck bcData;
            bcData.readFromJson(set, tag);
            d_forceDeck.push_back(bcData);
          } else if (tag == "Displacement_BC") {
            inp::BCBaseDeck bcData;
            bcData.readFromJson(set, tag);
            d_dispDeck.push_back(bcData);
          } else if (tag == "IC") {
            inp::BCBaseDeck icData;
            icData.readFromJson(set, tag);
            d_icDeck.push_back(icData);
          }
        } // loop over sets
      } // loop over tag
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
      oss << tabS << "------- BCDeck --------" << std::endl << std::endl;
      oss << tabS << "Force loading deck size = " << d_forceDeck.size() << std::endl;
      oss << tabS << "Force loading data:" << std::endl;
      for (size_t i = 0; i < d_forceDeck.size(); i++) {
        oss << tabS << "Force data for set = " << i << std::endl;
        oss << d_forceDeck[i].printStr(nt + 1, lvl);
      }

      oss << tabS << "Displacement loading deck size = " << d_dispDeck.size() << std::endl;
      oss << tabS << "Displacement loading data:" << std::endl;
      for (size_t i = 0; i < d_dispDeck.size(); i++) {
        oss << tabS << "Displacement data for set = " << i << std::endl;
        oss << d_dispDeck[i].printStr(nt + 1, lvl);
      }

      oss << tabS << "Initial condition deck size = " << d_icDeck.size() << std::endl;
      oss << tabS << "Initial condition data:" << std::endl;
      for (size_t i = 0; i < d_icDeck.size(); i++) {
        oss << tabS << "Initial condition for set = " << i << std::endl;
        oss << d_icDeck[i].printStr(nt + 1, lvl);
      }

      oss << tabS << "Gravity loading active = " << d_gravityActive << std::endl;
      oss << tabS << "Gravity loading value = " << d_gravity.printStr(0, 0) << std::endl;
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

#endif //INP_BCDECK_H
