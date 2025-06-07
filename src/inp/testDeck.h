/*
* -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_TESTDECK_H
#define INP_TESTDECK_H

#include <string>
#include "util/io.h"
#include "util/json.h"

namespace inp {
  /**
   * \ingroup Input
   */
  /**@{*/

  /*! @brief Structure to read and store test-related input data */
  struct TestDeck {
    /*! @brief Specify test name (if any) */
    std::string d_testName;

    /*! @brief if it is a compressive test, specify wall id and direction */
    size_t d_particleIdCompressiveTest;

    /*! @brief if it is a compressive test, specify force direction on wall */
    size_t d_particleForceDirectionCompressiveTest;


    /*!
     * @brief Constructor
     */
    TestDeck(const json &j = json({}))
      : d_testName(""), d_particleIdCompressiveTest(0), d_particleForceDirectionCompressiveTest(0) {
      readFromJson(j);
    };

    /*!
    * @brief Constructor
    */
    TestDeck(std::string testName, size_t particleIdCompressiveTest = 0,
             size_t particleForceDirectionCompressiveTest = 0)
          : d_testName(testName),
          d_particleIdCompressiveTest(particleIdCompressiveTest),
          d_particleForceDirectionCompressiveTest(particleForceDirectionCompressiveTest) {};

    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    static json getExampleJson(std::string testName = "", size_t particleIdCompressiveTest = 0,
        size_t particleForceDirectionCompressiveTest = 0) {

      if (testName == "")
        return json({});

      auto j = json({"Test_Name", testName});

      if (testName == "Compressive_Test") {
        j["Compressive_Test"] = json{{"Wall_Id", particleIdCompressiveTest},
          {"Wall_Force_Direction", particleForceDirectionCompressiveTest}};
      }

      return j;
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;

      d_testName = j.value("Test_Name", "");
      if (d_testName == "Compressive_Test") {
        if (j.find("Compressive_Test") == j.end())
          throw std::runtime_error("Compressive test requires Compressive_Test section");

        d_particleIdCompressiveTest = j.at("Compressive_Test").value("Wall_Id", 0);
        d_particleForceDirectionCompressiveTest = j.at("Compressive_Test").value("Wall_Force_Direction", 0);
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
      oss << tabS << "------- TestDeck --------" << std::endl << std::endl;
      oss << tabS << "Test name = " << d_testName << std::endl;
      oss << tabS << "Particle id for compressive test = " << d_particleIdCompressiveTest << std::endl;
      oss << tabS << "Particle force direction for compressive test = " << d_particleForceDirectionCompressiveTest <<
          std::endl;
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

#endif //INP_TESTDECK_H
