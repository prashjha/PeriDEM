/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_RESTARTDECK_H
#define INP_RESTARTDECK_H

#include <string>
#include "deckField.h"
#include "util/json.h"

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store restart related data input */
  struct RestartDeck {

    /*! @brief restart filename */
    std::string d_file;

    /*! @brief Restart time step */
    size_t d_step;

    /*! @brief Change only those particles/walls which have any one of its
     * nodes dof free */
    bool d_changeRefFreeDofs;

    /*!
     * @brief Constructor
     */
    RestartDeck(std::string file, size_t step = 0, bool changeRefFreeDofs = false)
      : d_file(file), d_step(step), d_changeRefFreeDofs(changeRefFreeDofs) {};

    /*!
     * @brief Constructor
     */
    RestartDeck(const json &j = json({}))
        : d_file(""), d_step(0), d_changeRefFreeDofs(false) {
      readFromJson(j);
    };


    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    static const std::vector<Field<RestartDeck>> &fields() {
      static const std::vector<Field<RestartDeck>> f = {
          field(&RestartDeck::d_file, "File", std::string(),
                "File the state is read from"),
          field(&RestartDeck::d_step, "Step", size_t(0),
                "Step the state was written at"),
          field(&RestartDeck::d_changeRefFreeDofs,
                "Change_Reference_Free_Dofs", false,
                "Move only the bodies that have a free degree of freedom"),
      };
      return f;
    }

    /*!
     * @brief Returns the block with the given fields set
     *
     * A restart is requested by naming a file. Without one the block is
     * empty, and the model starts from the initial condition.
     *
     * @param given Field names and values to set, checked against the table
     * @return JSON object for this deck
     */
    static json getExampleJson(const json &given = json::object()) {
      json j = applyGiven(given, fields());
      if (j.at("File").get<std::string>().empty())
        return json({});
      return j;
    }

    /*!
     * @brief Returns the block with the given fields set
     *
     * Kept so that existing callers continue to compile. Prefer the form that
     * names each field.
     *
     * @param file File the state is read from
     * @param step Step the state was written at
     * @param changeRefFreeDofs Move only bodies with a free degree of freedom
     * @return JSON object for this deck
     */
    static json getExampleJson(std::string file, size_t step = 0,
                               bool changeRefFreeDofs = false) {
      return getExampleJson(json{{"File", file},
                                 {"Step", step},
                                 {"Change_Reference_Free_Dofs",
                                  changeRefFreeDofs}});
    }

    /*!
     * @brief Reads from json object
     * @param j JSON object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;
      readFields(*this, j, fields());
    }

    /*!
     * @brief Returns this deck as a JSON object
     * @return JSON object for this deck
     */
    json writeToJson() const {
      json j = json::object();
      writeFields(*this, j, fields());
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
      oss << tabS << "------- RestartDeck --------" << std::endl << std::endl;
      printFields(*this, oss, fields(), tabS);
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

#endif // INP_RESTARTDECK_H
