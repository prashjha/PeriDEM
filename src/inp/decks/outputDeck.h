/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_OUTPUTDECK_H
#define INP_OUTPUTDECK_H

#include "util/io.h"
#include <string>
#include <vector>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read input data for performing simulation output */
struct OutputDeck {

  /*! @brief Output format: currently supports vtu, msh, legacy_vtk output
   *
   * Default is vtu format.
   */
  std::string d_outFormat;

  /*! @brief Output Path where the files will be written
   *
   * Default is current working directory
   */
  std::string d_path;

  /*! @brief List of tags of data to be dumped */
  std::vector<std::string> d_outTags;

  /*! @brief Size of time steps (or frequency) for output operation */
  size_t d_dtOut;

  /*! @brief Size of time steps (or frequency) for output operation */
  size_t d_dtOutOld;

  /*!
   * @brief Flag specifying debug level
   * TODO verify below value-description text
   * value = 0: code is almost completely silent
   * value = 1: some information are printed and logged
   * value = 2: more verbosity
   * value = 3: highest verbosity
   */
  size_t d_debug;

  /*!
   * @brief Flag specifying if element-node connectivity should not be
   * dumped
   *
   * For large mesh, vtk writer crashes when writing element-node connectivity.
   */
  bool d_performFEOut;

  /*! @brief Compressor type for .vtu files */
  std::string d_compressType;

  /*! @brief Specify output criteria to change output frequency
   *
   * Choices are:
   * - '' (none/null)
   * - max_Z
   * - max_Z_stop
   *
   * Specify the method used in changing the output frequency. If not
   * specified then we do not change the output frequency from d_dtOut.
   * */
  std::string d_outCriteria;

  /*! @brief Specify output frequency if output criteria is met
   *
   * If criteria is met, then this number if used as output frequency.
   * */
  size_t d_dtOutCriteria;

  /*! @brief List of parameters required in checking output criteria */
  std::vector<double> d_outCriteriaParams;

  /*! @brief Perform vtu output */
  bool d_performOut;

  /*! @brief Size of time steps (or frequency) for output operation */
  size_t d_dtTestOut;

  /*! @brief Tag for postprocessing file */
  std::string d_tagPPFile;

  /*!
   * @brief Constructor
   */
  OutputDeck()
      : d_outFormat("vtu"), d_path("./"), d_dtOut(0), d_dtOutOld(0), d_debug(0),
        d_performFEOut(true), d_dtOutCriteria(0), d_performOut(true),
        d_dtTestOut(0), d_tagPPFile("") {};

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
    oss << tabS << "------- OutputDeck --------" << std::endl << std::endl;
    oss << tabS << "Output format = " << d_outFormat << std::endl;
    oss << tabS << "Output path = " << d_path
        << std::endl;
    oss << tabS << "Output tags = " << util::io::printStr<std::string>(d_outTags, 0) << std::endl;
    oss << tabS << "Output time step = " << d_dtOut << std::endl;
    oss << tabS << "Output time step old = " << d_dtOutOld << std::endl;
    oss << tabS << "Debug level = " << d_debug << std::endl;
    oss << tabS << "Perform FE output = " << d_performFEOut << std::endl;
    oss << tabS << "Output file compression type = " << d_compressType << std::endl;
    oss << tabS << "Output criteria = " << d_outCriteria << std::endl;
    oss << tabS << "Output dt criteria = " << d_dtOutCriteria << std::endl;
    oss << tabS << "Output criteria parameters = " << util::io::printStr<double>(d_outCriteriaParams, 0) << std::endl;
    oss << tabS << "Perform output = " << d_performOut << std::endl;
    oss << tabS << "Output time step when test = " << d_dtTestOut << std::endl;
    oss << tabS << "Tag for postprocessing file = " << d_tagPPFile << std::endl;
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

  /*!
   * @brief Searches list of tags and returns true if the asked tag is in the
   * list
   * @param tag Tag to search
   * @return bool True/false If tag is find return true or else false
   */
  bool isTagInOutput(const std::string &tag) {

    // search for tag in output tag list
    for (const auto &type : d_outTags)
      if (tag == type)
        return true;

    return false;
  };
};

/** @}*/

} // namespace inp

#endif // INP_OUTPUTDECK_H
