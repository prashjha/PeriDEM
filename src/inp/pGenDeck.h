/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_PGENDECK_H
#define INP_PGENDECK_H

#include "deckField.h"
#include "util/io.h"
#include "util/json.h"
#include "geom/geomIncludes.h"
#include <string>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store particle generation data, such as particle locations and group file */
  struct PGenDeck {

    /*!
     * @brief Particle generation method
     *
     * "From_File" means particle location, radius and other details will be
     * loaded from the input json file. Use key 'Particle_Generation' to provide the data.
     * 
     * "Use_Particle_Geometry" means particle geometry will be used to generate particles.
     */
    std::string d_genMethod;

    /*! @brief Json object loaded from the input json file or jason file for particle generation */
    json d_pGenJson;

    /*! @brief Random rotation of particles if orientation is not provided */
    bool d_genWithRandomRotation;

    /*!
     * @brief Constructor
     */
    PGenDeck(const json &j = json({}))
      : d_genMethod("Use_Particle_Geometry"),
        d_pGenJson({}), d_genWithRandomRotation(true) {
      readFromJson(j);
    };

    /*!
     * @brief Constructor
     */
    PGenDeck(std::string genMethod, json pGenJson = json({}), bool genWithRandomRotation = true)
        : d_genMethod(genMethod),
          d_pGenJson(pGenJson), d_genWithRandomRotation(genWithRandomRotation) {};

    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    static const std::vector<Field<PGenDeck>> &fields() {
      static const std::vector<Field<PGenDeck>> f = {
          field(&PGenDeck::d_genMethod, "Method", std::string("From_File"),
                "Where the particle positions come from",
                {{"From_File", "Use_Particle_Geometry"}, {}, {}}),
          field(&PGenDeck::d_genWithRandomRotation, "Random_Rotation", true,
                "Give each particle a random orientation when none is given"),
      };
      return f;
    }

    /*!
     * @brief Returns the block with the given fields set
     * @param given Field names and values to set, checked against the table
     * @return JSON object for this deck
     */
    static json getExampleJson(const json &given = json::object()) {
      return applyGiven(given, fields(), {"Data"});
    }

    /*!
     * @brief Rejects a value given where a set of named values is expected
     *
     * A string converts to a JSON string, which would otherwise bind to the
     * overload above and be read as a block with no keys. Deleting these
     * makes that a compile error rather than a failure when the deck is
     * built.
     */
    static json getExampleJson(const std::string &) = delete;
    /*! @copydoc getExampleJson(const std::string &) */
    static json getExampleJson(const char *) = delete;

    /*!
     * @brief Reads from json object
     * @param j JSON object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;
      readFields(*this, j, fields());

      // The positions themselves are a block of their own, not a field.
      if (d_genMethod == "From_File") {
        if (j.find("Data") == j.end())
          throw std::runtime_error("Need information inside key Data.");

        d_pGenJson = j.at("Data");
      }
    }

    /*!
     * @brief Returns this deck as a JSON object
     * @return JSON object for this deck
     */
    json writeToJson() const {
      json j = json::object();
      writeFields(*this, j, fields());
      if (d_genMethod == "From_File")
        j["Data"] = d_pGenJson;
      return j;
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
      oss << tabS << "------- PGenDeck --------" << std::endl << std::endl;
      printFields(*this, oss, fields(), tabS);
      size_t nParticles = d_pGenJson.value("N", 0);
      oss << tabS << "Number of particles in json object = " << nParticles << std::endl;
      oss << tabS << "Data for first five particles: " << std::endl;
      if (nParticles > 0) {
        nParticles = nParticles > 5 ? 5 : nParticles;
        for (size_t i=0; i< nParticles; i++) {
          oss << tabS << "Particle number = " << i << std::endl;
          oss << tabS << d_pGenJson.at(std::to_string(i)) << std::endl;
        }
      }
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

#endif // INP_PGENDECK_H
