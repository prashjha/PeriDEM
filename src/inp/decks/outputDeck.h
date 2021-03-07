/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INP_OUTPUTDECK_H
#define INP_OUTPUTDECK_H

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

  /*! @brief Flag specifying debug level */
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
   * - <none>
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
        d_dtTestOut(0), d_tagPPFile("0") {};

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
