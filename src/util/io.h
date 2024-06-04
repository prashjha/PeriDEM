/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTILS_UTIL_IO_H
#define UTILS_UTIL_IO_H

#include "point.h"
#include "parallelUtil.h" // to make prints MPI aware
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace util {

/*! @brief Provides geometrical methods such as point inside rectangle */
namespace io {

/*! @brief Default value of tab used in outputting formatted information */
const int print_default_tab = 0;

/*! @brief Default mpi rank in printing information */
const int print_default_mpi_rank = 0;

/*! @brief Default debug level for logger */
const int logger_default_debug_lvl = 5;

/*!
 * @brief Returns tab spaces of given size
 * @param nt Number of tabs
 * @return string Tab spaces
 */
inline std::string getTabS(int nt) {
  std::string tabS = "";
  for (int i = 0; i < nt; i++)
    tabS += "\t";

  return tabS;
};

/*!
 * @brief Returns formatted string for output
 * @param msg Message
 * @param nt Number of tabs to prefix
 * @return string Formatted string
 */
template <class T> inline std::string printStr(const T &msg, int nt = print_default_tab) {

  std::ostringstream oss;
  oss << getTabS(nt) << msg;
  return oss.str();
};

/*!
 * @brief Returns formatted string for output
 * @param list List of objects
 * @param nt Number of tabs to prefix
 * @return string Formatted string
 */
template <class T> inline std::string printStr(const std::vector<T> &list,
    int nt = print_default_tab) {

  auto tabS = getTabS(nt);
  std::ostringstream oss;
  oss << tabS;
  size_t i = 0;
  for (auto l : list) {
    oss << l;
    i++;
    if (i != list.size())
      oss << ", ";
  }

  return oss.str();
};

/*! @copydoc printStr(const std::vector<T> &list, int nt = print_default_tab) */
template <> inline std::string printStr(const std::vector<util::Point> &list,
                                               int nt) {

  auto tabS = getTabS(nt);
  std::ostringstream oss;
  oss << tabS;
  size_t i = 0;
  for (auto l : list) {
    oss << "(" << l[0] << ", " << l[1] << ", " << l[2] << ")";
    i++;
    if (i != list.size())
      oss << ", ";
  }

  return oss.str();
};

/*!
 * @brief Prints formatted information
 * @param msg Message
 * @param nt Number of tabs
 * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
 */
template <class T> inline void print(const T &msg, int nt = print_default_tab,
        int printMpiRank = print_default_mpi_rank) {
  if (printMpiRank < 0 or util::parallel::mpiRank() == printMpiRank)
    std::cout << printStr(msg, nt);
};

/*!
 * @brief Prints formatted information
 * @param list List of objects
 * @param nt Number of tabs
 * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
 */
template <class T> inline void print(const std::vector<T> &list, int nt = print_default_tab,
        int printMpiRank = print_default_mpi_rank) {
  if (printMpiRank < 0 or util::parallel::mpiRank() == printMpiRank)
      std::cout << printStr(list, nt);
};

/*! @copydoc printStr(const std::vector<T> &list, int nt = print_default_tab) */
template <class T>
inline std::string printStr(const std::vector<std::vector<T>> &list,
                            int nt = print_default_tab) {

  auto tabS = getTabS(nt);
  std::ostringstream oss;
  oss << tabS;
  size_t i = 0;
  for (auto l : list) {
    oss << "(";
    for (size_t k = 0; k < l.size(); k++) {
      oss << l[k];
      if (k < l.size() - 1)
        oss << ", ";
    }
    oss << ")";

    i++;
    if (i != list.size())
      oss << ", ";
  }

  return oss.str();
}

/*! @copydoc print(const std::vector<T> &list, int nt = print_default_tab,
        int printMpiRank = print_default_mpi_rank) */
template <class T> inline void print(const std::vector<std::vector<T>> &list,
        int nt = print_default_tab,
        int printMpiRank = print_default_mpi_rank) {

  if (printMpiRank < 0 or util::parallel::mpiRank() == printMpiRank)
    std::cout << printStr(list, nt);
};

/*!
 * @brief Returns formatted string for output
 * @param box Pair of corner points of box
 * @param nt Number of tabs to prefix
 * @return string Formatted string
 */
inline std::string printBoxStr(const std::pair<util::Point, util::Point>
    &box, int nt = print_default_tab) {
  auto tabS = getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "Corner point 1 = " << box.first.printStr(nt, 0) << std::endl;
  oss << tabS << "Corner point 2 = " << box.second.printStr(nt, 0) << std::endl;

  return oss.str();
};

/*!
 * @brief Prints formatted string for output
 * @param box Pair of corner points of box
 * @param nt Number of tabs to prefix
 * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
 */
inline void printBox(const std::pair<util::Point, util::Point> &box,
                     int nt = print_default_tab,
                     int printMpiRank = print_default_mpi_rank) {
    if (printMpiRank < 0 or util::parallel::mpiRank() == printMpiRank)
      std::cout << printBoxStr(box, nt);
};

/*! @copydoc printBoxStr(const std::pair<util::Point, util::Point>
    &box, int nt = print_default_tab) */
inline std::string printBoxStr(const std::pair<std::vector<double>, std::vector<double>> &box,
                               int nt = print_default_tab) {
  auto tabS = getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "Corner point 1 = (" << printStr<double>(box.first, 0)
              << ")" << std::endl;
  oss << tabS << "Corner point 2 = (" << printStr<double>(box.second, 0)
              << ")" << std::endl;

  return oss.str();
};

/*! @copydoc printBox(const std::pair<util::Point, util::Point> &box,
                     int nt = print_default_tab,
                     int printMpiRank = print_default_mpi_rank) */
inline void printBox(const std::pair<std::vector<double>, std::vector<double>> &box,
                     int nt = print_default_tab, int printMpiRank = print_default_mpi_rank) {
  if (printMpiRank < 0 or util::parallel::mpiRank() == printMpiRank)
    std::cout << printBoxStr(box, nt);
};

/*!
 * @brief Deck to store log parameters
 */
struct LoggerDeck {

  /*! @brief Debug level */
  int d_debugLevel;

  /*! @brief Filename to print log */
  std::string d_filename;

  /*! @brief Print to std::cout? */
  bool d_printScreen;

  /*! @brief Print to file? */
  bool d_printFile;

  /*!
   * @brief Constructor
   */
  LoggerDeck() : d_debugLevel(5), d_printScreen(true), d_printFile(false) {}

  /*!
   * @brief Constructor
   *
   * @param debug_level Specify debug level/verbosity
   * @param filename Specify log filename
   */
  LoggerDeck(int debug_level, std::string filename)
      : d_debugLevel(debug_level), d_filename(filename), d_printScreen
      (d_debugLevel > 0), d_printFile(!d_filename.empty()) {}
};


/*!
 * @brief Prints log to std::cout and also write to the file
 */
class Logger {

public:

  /*!
   * @brief Constructor
   *
   * @param deck Logger deck
   */
  Logger(LoggerDeck *deck = nullptr) : d_deck_p(deck) {

    if (d_deck_p == nullptr)
      d_deck_p = new LoggerDeck();
  }

  /*!
   * @brief Destructor
   */
  ~Logger() {

    if (d_deck_p->d_printFile)
      d_logFile.close();
  }

  /*!
   * @brief Log the message
   *
   * @param oss Message
   * @param screen_out Specify if it goes to std::cout as well
   * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
   */
  void log(std::ostringstream &oss, bool screen_out = false,
           int printMpiRank = print_default_mpi_rank) {

    log(oss.str(), screen_out, printMpiRank);

    // reset oss
    oss.str("");
    oss.clear();
  };

  /*!
   * @brief Log the message
   *
   * @param str Message
   * @param screen_out Specify if it goes to std::cout as well
   * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
   */
  void log(const std::string &str, bool screen_out = false,
           int printMpiRank = print_default_mpi_rank) {

    if (printMpiRank < 0 or util::parallel::mpiRank() == printMpiRank) {
      if (d_deck_p->d_printScreen || screen_out)
        std::cout << str << std::flush;

      // log
      if (d_deck_p->d_printFile) {
        d_logFile.open(d_deck_p->d_filename, std::ios_base::app);
        d_logFile << str;
        d_logFile.close();
      }
    }
  };

  /*! @brief Pointer to logger deck */
  LoggerDeck *d_deck_p;

  /*! @brief Filestream for logging */
  std::ofstream d_logFile;
};

/*!
 * @brief Initializes the logger
 * @param debug_level Specify debug level/verbosity
 * @param filename Specify filename for logs
 */
void initLogger(int debug_level = logger_default_debug_lvl, std::string filename = "");

/*!
 * @brief Global method to log the message
 *
 * @param oss Message
 * @param screen_out Specify if it goes to std::cout as well
 * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
 */
void log(std::ostringstream &oss, bool screen_out = false,
         int printMpiRank = print_default_mpi_rank);

/*!
 * @brief Global method to log the message
 *
 * @param str Message
 * @param screen_out Specify if it goes to std::cout as well
 * @param printMpiRank MPI rank to do log/print. Negative value means all ranks log.
 */
void log(const std::string &str, bool screen_out = false,
         int printMpiRank = print_default_mpi_rank);

/*!
 * @brief Input command line argument parser
 *
 * source - https://stackoverflow.com/a/868894
 * @author iain
 */
class InputParser {
public:
  /*!
   * @brief Constructor
   * @param argc Number of arguments
   * @param argv Strings
   */
  InputParser(int &argc, char **argv) {
    for (int i = 1; i < argc; ++i)
    this->tokens.push_back(std::string(argv[i]));
  }

  /*!
   * @brief Get value of argument specified by key
   * @param option Argument name/key
   * @return string Value of argument
   */
  const std::string &getCmdOption(const std::string &option) const {
    std::vector<std::string>::const_iterator itr;
    itr = std::find(this->tokens.begin(), this->tokens.end(), option);
    if (itr != this->tokens.end() && ++itr != this->tokens.end())
      return *itr;

    static const std::string empty_string("");
    return empty_string;
  }

  /*!
   * @brief Check if argument exists
   * @param option Argument name/key
   * @return bool True if argument exists
   */
  bool cmdOptionExists(const std::string &option) const {
    return std::find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
  }

private:
  /*! @brief Tokens */
  std::vector<std::string> tokens;
};

/*!
 * @brief Get filename removing path from the string
 * Source - https://stackoverflow.com/a/24386991
 *
 * @param path Filename with path
 * @param delims Delimiter to separate the filename from the path
 * @return string Filename
 */
inline std::string getFilenameFromPath(std::string const & path, std::string const & delims = "/\\")
{
  return path.substr(path.find_last_of(delims) + 1);
}

/*!
 * @brief Remove extension from the filename
 * Source - https://stackoverflow.com/a/24386991
 *
 * @param filename Filename with extension
 * @return string Filename without extension
 */
inline std::string removeExtensionFromFile(std::string const & filename)
{
  typename std::string::size_type const p(filename.find_last_of('.'));
  return p > 0 && p != std::string::npos ? filename.substr(0, p) : filename;
}

/*!
 * @brief Get extension from the filename
 *
 * @param filename Filename with extension
 * @return string Extension
 */
inline std::string getExtensionFromFile(std::string const & filename)
{
  typename std::string::size_type const p(filename.find_last_of('.'));

  //return filename.substr(p + 1);
  return p > 0 && p != std::string::npos ? filename.substr(p+1) : "";
}

/*!
 * @brief Check for extension and if possible create new filename from a given filename and a given extension
 *
 * @param filename Filename with/without extension
 * @param filename_ext Extension to check for and append if needed
 * @return string New filename if successful
 */
inline std::string checkAndCreateNewFilename(std::string const & filename, std::string filename_ext)
{
  auto f_ext = util::io::getExtensionFromFile(filename);
  std::string f = filename;
  if (f_ext.empty()) {
    f = f + "." + filename_ext;
  }
  else {
    if (f_ext != filename_ext) {
      std::cerr << "checkAndCreateNewFilename(): Argument filename = "
                << filename << " has extension = "
                << f_ext << " which does not match expected extension = "
                << filename_ext << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  return f;
}

} // namespace io

} // namespace util

#endif // UTILS_UTIL_IO_H
