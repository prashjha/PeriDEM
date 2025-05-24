/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_PGENDECK_H
#define INP_PGENDECK_H

#include "util/io.h"
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
      : d_genMethod("From_File"),
        d_pGenJson({}) {
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
    static json getExampleJson(std::string genMethod = "From_File") {

      auto j = json({{"Method", "From_File"}, {"Random_Rotation", true}});
      return j;
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;

      d_genMethod = j.value("Method", std::string("Use_Particle_Geometry"));
      d_genWithRandomRotation = j.value("Random_Rotation", true);

      if (d_genMethod == "From_File") {
        if (j.find("Data") == j.end())
          throw std::runtime_error("Need information inside key Data.");

        d_pGenJson = j.at("Data");
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
      oss << tabS << "------- PGenDeck --------" << std::endl << std::endl;
      oss << tabS << "Method = " << d_genMethod << std::endl;
      oss << tabS << "Random rotation of particles = " << d_genWithRandomRotation << std::endl;
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
