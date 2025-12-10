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
  RestartDeck() : d_step(0), d_changeRefFreeDofs(false) {};
};

/** @}*/

} // namespace inp

#endif // INP_RESTARTDECK_H
