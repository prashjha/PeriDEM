/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_CONTACTDECK_H
#define INP_CONTACTDECK_H

#include "contactPairDeck.h"
#include "util/json.h"

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store particle-particle contact related input
 * data */
struct ContactDeck {

  /*!
   * @brief Store contact parameters for each pair of zone
   */
  std::vector<std::vector<ContactPairDeck>> d_data;

  /*!
   * @brief Constructor
   */
  ContactDeck(const json &j = json({})) {
    readFromJson(j);
  };

  /*!
   * @brief Returns the contact data
   *
   * @param i Zone i
   * @param j Zone j
   * @return data Contact data between zone i and j
   */
  const ContactPairDeck &getContact(const size_t &i, const size_t &j) const {
    //return d_data[i < j ? i : j][i < j ? j : i];
    return d_data[i][j];
  }

  /*! @copydoc getContact(const size_t &i, const size_t &j) const */
  ContactPairDeck &getContact(const size_t &i, const size_t &j) {
    //return d_data[i < j ? i : j][i < j ? j : i];
    return d_data[i][j];
  }

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getExampleJson(size_t nSets = 0) {

    if (nSets == 0)
      return json({});

    auto j = json({{"Sets", nSets}});

    for (size_t i = 0; i < nSets; i++) {
      for (size_t k=i; k<nSets; k++) {
        std::string set_name = "Set_" + std::to_string(i+1) + "_" + std::to_string(k+1);
        j[set_name] = {};
      }
    }

    return j;
  }

  /*!
   * @brief Reads from json object
   */
  void readFromJson(const json &j) {
    if (j.empty())
      return;

    auto nSets = j.value("Sets", size_t(0));
    d_data.resize(nSets);
    for (size_t i=0; i<nSets; i++)
      d_data[i].resize(nSets);

    for (size_t i = 0; i < nSets; i++) {
      for (size_t k = i; k < nSets; k++) {
        std::string set_name = "Set_" + std::to_string(i+1) + "_" + std::to_string(k+1);
        //std::cout << "\n\n\n processing " << set_name << "\n\n\n" << std::flush;

        auto cd = inp::ContactPairDeck();

        if (j.find(set_name) == j.end())
          throw std::runtime_error("Set " + set_name + " not found in contact block.");

        auto js = j.at(set_name);

        // copy this set from previous set?
        std::vector<int> copy_set = js.value("Copy_Data", std::vector<int>({-1, -1}));
        //std::cout << "copy_data = " << copy_set[0] << ", " << copy_set[1] << "\n" << std::flush;

        // check
        if ((copy_set[0] != -1 and copy_set[1] == -1) or (copy_set[0] == -1 and copy_set[1] != -1) or (copy_set.size() != 2))
          throw std::runtime_error("The pair of set ids for copying is invalid in contact block.");

        if (copy_set[0] != -1) {
          std::string copy_set_tag = "Set_" + std::to_string(copy_set[0]) + "_" + std::to_string(copy_set[1]);
          auto js_copy = j.at(copy_set_tag);
          cd.readFromJson(js_copy);
        } else {
          cd.readFromJson(js);
        }

        d_data[i][k] = cd;
        if (i != k)
          d_data[k][i] = cd;
      }
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
    oss << tabS << "------- ContactDeck --------" << std::endl << std::endl;
    for (size_t i =0; i<d_data.size(); i++) {
      for (size_t j = 0; j < d_data.size(); j++) {
        oss << tabS << "ContactPairData id = (" << i << "," << j << ") info:"
                  << std::endl;
        oss << getContact(i,j).printStr(nt+2, lvl);
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

#endif // INP_CONTACTDECK_H
