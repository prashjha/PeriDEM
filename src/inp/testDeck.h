/*
* -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_TESTDECK_H
#define INP_TESTDECK_H

#include <string>
#include "deckField.h"
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
    static const std::vector<Field<TestDeck>> &fields() {
      static const std::vector<Field<TestDeck>> f = {
          field(&TestDeck::d_testName, "Test_Name", std::string(),
                "Post-processing applied after the run"),
      };
      return f;
    }

    /*!
     * @brief True when the name selects the compressive test
     * @param name Test name
     * @return bool True for either spelling
     */
    static bool isCompressive(const std::string &name) {
      return name == "Compressive_Test" || name == "compressive_test";
    }

    /*!
     * @brief Returns the block with the given fields set
     *
     * Without a test name the block is empty and no post-processing is done.
     * The compressive test reads a sub-block that names the wall, so it is
     * written here rather than declared as a field.
     *
     * @param given Field names and values to set, checked against the table
     * @return JSON object for this deck
     */
    static json getExampleJson(const json &given = json::object()) {
      json j = applyGiven(given, fields(), {"Compressive_Test"});
      const auto name = j.at("Test_Name").get<std::string>();
      if (name.empty())
        return json({});
      if (isCompressive(name) && j.find("Compressive_Test") == j.end())
        j["Compressive_Test"] =
            json{{"Wall_Id", size_t(0)}, {"Wall_Force_Direction", size_t(0)}};
      return j;
    }

    /*!
     * @brief Returns the block with the given fields set
     *
     * Kept so that existing callers continue to compile. Prefer the form that
     * names each field.
     *
     * @param testName Post-processing applied after the run
     * @param particleIdCompressiveTest Wall the reaction is measured on
     * @param particleForceDirectionCompressiveTest Direction of that reaction
     * @return JSON object for this deck
     */
    /*!
     * @brief Rejects a value given where a set of named values is expected
     *
     * A test name passed on its own would convert to a JSON string and be
     * read as a block with no keys, so such a call is rejected when
     * compiling.
     */
    static json getExampleJson(const std::string &) = delete;
    /*! @copydoc getExampleJson(const std::string &) */
    static json getExampleJson(const char *) = delete;

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;

      readFields(*this, j, fields());
      if (isCompressive(d_testName)) {
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
      printFields(*this, oss, fields(), tabS);
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
