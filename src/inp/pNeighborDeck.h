/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_PNEIGHBORDECK_H
#define INP_PNEIGHBORDECK_H

#include "util/io.h"
#include "deckField.h"
#include "util/json.h"

namespace inp {

/**@{*/

/*! @brief User-input data for particle neighbor search */
struct PNeighborDeck {

  /*! @brief Neighbor search update criteria (if any) */
  std::string d_updateCriteria;

  /*! @brief Neighbor search factor (search length is factor times biggest
   * radius of particle) */
  double d_sFactor;

  /*! @brief Neighbor update time interval (for contact) */
  size_t d_neighUpdateInterval;

  /*! @brief Specify how deep we search for nodes near boundary for contact calculations */
  double d_nearBdNodesTol;

  /*!
   * @brief Constructor
   */
  PNeighborDeck(const json &j = json({}))
      : d_updateCriteria("simple_all"),
        d_sFactor(1.),
        d_neighUpdateInterval(1),
        d_nearBdNodesTol(0.5) {
    readFromJson(j);
  };

  /*!
   * @brief Constructor
   */
  PNeighborDeck(std::string updateCriteria, double sFactor = 1.,
                size_t neighUpdateInterval = 1, double nearBdNodesTol = 0.5)
      : d_updateCriteria(updateCriteria),
        d_sFactor(sFactor),
        d_neighUpdateInterval(neighUpdateInterval),
        d_nearBdNodesTol(nearBdNodesTol) {};

  /*!
   * @brief The fields of this deck, declared once
   *
   * Reading, writing, printing and the schema are generated from this table.
   * @return fields The field table
   */
  static const std::vector<Field<PNeighborDeck>> &fields() {
    static const std::vector<Field<PNeighborDeck>> f = {
        field(&PNeighborDeck::d_updateCriteria, "Update_Criteria",
              std::string("simple_all"),
              "When the contact neighbor list is rebuilt"),
        field(&PNeighborDeck::d_sFactor, "Search_Factor", 1.,
              "Search length as a multiple of the largest particle radius"),
        field(&PNeighborDeck::d_neighUpdateInterval, "Search_Interval",
              size_t(1), "Steps between contact neighbor list updates"),
        field(&PNeighborDeck::d_nearBdNodesTol, "Near_Bd_Nodes_Tol", 0.5,
              "Depth within which a node counts as near the boundary"),
    };
    return f;
  }

  /*!
   * @brief Returns the block with the given fields set
   * @param given Field names and values to set, checked against the table
   * @return JSON object for this deck
   */
  static json getExampleJson(const json &given = json::object()) {
    return applyGiven(given, fields());
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
    oss << tabS << "------- PNeighborDeck --------" << std::endl << std::endl;
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

#endif // INP_PNEIGHBORDECK_H
