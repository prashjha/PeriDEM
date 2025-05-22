/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_RESTARTDECK_H
#define INP_RESTARTDECK_H

#include <string>

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
    static json getExampleJson(std::string file = "", size_t step = 0, bool changeRefFreeDofs = false) {


      if (file.empty()) return json({});

      return json({{"File", file}, {"Step", step}, {"Change_Reference_Free_Dofs", changeRefFreeDofs}});
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;

      d_file = j.value("File", std::string());
      d_step = j.value("Step", size_t(0));
      d_changeRefFreeDofs = j.value("Change_Reference_Free_Dofs", false);
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
      oss << tabS << "Filename = " << d_file << std::endl;
      oss << tabS << "Restart step = " << d_step << std::endl;
      oss << tabS << "Change only free dofs? = " << d_changeRefFreeDofs << std::endl;
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
