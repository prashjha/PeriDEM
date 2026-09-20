/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_OUTPUTDECK_H
#define INP_OUTPUTDECK_H

#include "deckField.h"
#include "util/io.h"
#include "util/json.h"
#include "util/vecMethods.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace inp {
  /**
   * \ingroup Input
   */
  /**@{*/

  /*! @brief Structure to read input data for performing simulation output */
  struct OutputDeck {
    /*! @brief Output format. Allowed values are vtu, msh, legacy_vtk
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
     * @brief If true, write a ParaView .pvd collection alongside VTU files so one file
     * can be opened to animate all timesteps (references the per-step .vtu files).
     */
    bool d_pvdCollection;

    /*!
     * @brief Constructor
     */
    OutputDeck(const json& j = json({}))
      : d_outFormat("vtu"), d_path("./"), d_dtOut(0), d_dtOutOld(0), d_debug(0),
        d_performFEOut(true), d_dtOutCriteria(0), d_performOut(true),
        d_dtTestOut(0), d_tagPPFile(""), d_pvdCollection(false) {

      readFromJson(j);
    };


    /*!
     * @brief Constructor
     */
    OutputDeck(std::string outFormat,
        std::string path = "./",
        std::vector<std::string> outTags = {"Displacement"},
                size_t outputInterval = 1,
                size_t debug = 2,
                bool performFEOut = true,
                std::string compressType = "zlib",
                bool performOut = true,
        size_t dtTestOut = 1,
        std::string tagPPFile = "",
        bool pvdCollection = false)
      : d_outFormat(outFormat), d_path(path),
        d_dtOut(outputInterval), d_dtOutOld(outputInterval), d_dtOutCriteria(outputInterval),
        d_debug(debug),
        d_performFEOut(performFEOut), d_compressType(compressType),
        d_performOut(performOut), d_dtTestOut(dtTestOut), d_tagPPFile(tagPPFile),
        d_pvdCollection(pvdCollection), d_outTags(outTags){};

    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    /*!
     * @brief Rejects a value given where a set of named values is expected
     *
     * A format or path passed on its own would convert to a JSON string and
     * be read as a block with no keys, so such a call is rejected when
     * compiling.
     */
    static json getExampleJson(const std::string &) = delete;
    /*! @copydoc getExampleJson(const std::string &) */
    static json getExampleJson(const char *) = delete;

    /*!
     * @brief The fields of this deck, declared once
     *
     * Reading, writing, printing and the schema are generated from this
     * table. The output criteria are a block of their own and are read in
     * readDerived.
     *
     * @return fields The field table
     */
    static const std::vector<Field<OutputDeck>> &fields() {
      static const std::vector<Field<OutputDeck>> f = {
          field(&OutputDeck::d_path, "Path", std::string("./"),
                "Directory the output is written to"),
          field(&OutputDeck::d_performOut, "Perform_Out", true,
                "Write output at all"),
          field(&OutputDeck::d_outTags, "Tags", std::vector<std::string>(),
                "Fields written at each output step"),
          field(&OutputDeck::d_dtOut, "Output_Interval", size_t(1),
                "Steps between outputs", {{}, size_t(1), {}}),
          field(&OutputDeck::d_debug, "Debug", size_t(2),
                "How much is written to the log"),
          field(&OutputDeck::d_performFEOut, "Perform_FE_Out", true,
                "Write the finite element mesh as well as the nodes"),
          field(&OutputDeck::d_compressType, "Compress_Type",
                std::string("zlib"), "Compression used in the output files"),
          field(&OutputDeck::d_outFormat, "File_Format", std::string("vtu"),
                "Output file format"),
          field(&OutputDeck::d_dtTestOut, "Test_Output_Interval", size_t(1),
                "Steps between post-processing outputs"),
          field(&OutputDeck::d_tagPPFile, "Tag_PP", std::string(),
                "Suffix of the post-processing file"),
          // Absent unless a collection is asked for, which is how the decks
          // in the repository were written.
          field<OutputDeck, bool>(&OutputDeck::d_pvdCollection,
                                  "PVD_Collection", false,
                                  "Write a PVD file indexing the time steps",
                                  {}, [](const bool &v) { return v; }),
      };
      return f;
    }

    /*!
     * @brief Returns the block with the given fields set
     * @param given Field names and values to set, checked against the table
     * @return JSON object for this deck
     */
    static json getExampleJson(const json &given = json::object()) {
      return applyGiven(given, fields(), {"Output_Criteria"});
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return ;

      readFields(*this, j, fields());

      // An empty path names the working directory.
      if (d_path.empty())
        d_path = "./";

      // Both follow the output interval until the criteria block changes one.
      d_dtOutOld = d_dtOut;
      d_dtOutCriteria = d_dtOut;

      if (j.contains("Output_Criteria")) {
        d_outCriteria = j.at("Output_Criteria").value("Type", std::string(""));
        d_dtOutCriteria = j.at("Output_Criteria").value("New_Interval", size_t(1));
        d_outCriteriaParams = j.at("Output_Criteria").value("Parameters", std::vector<double>());
        if (d_dtOutCriteria < 1)
          throw std::runtime_error(
              "Output_Criteria.New_Interval must be >= 1 (use 1 to write every step).");
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
      oss << tabS << "------- OutputDeck --------" << std::endl << std::endl;
      printFields(*this, oss, fields(), tabS);
      oss << tabS << "Output_Interval before the criteria = " << d_dtOutOld
          << std::endl;
      oss << tabS << "Output_Criteria.Type = " << d_outCriteria << std::endl;
      oss << tabS << "Output_Criteria.New_Interval = " << d_dtOutCriteria
          << std::endl;
      oss << tabS << "Output_Criteria.Parameters = "
          << util::io::printStr<double>(d_outCriteriaParams, 0) << std::endl;
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

#endif // INP_OUTPUTDECK_H
