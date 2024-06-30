////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "io.h"
#include <cassert>

namespace {
    util::io::Logger *logger_p = nullptr;
}

void util::io::initLogger(int debug_level, std::string filename) {

  if (logger_p != nullptr)
    return;

  auto deck = new LoggerDeck(debug_level, filename);
  logger_p = new Logger(deck);
}

void util::io::log(const std::string & str, bool screen_out, int printMpiRank) {

  // for now, we do not call assert and rather create a logger if it does not exist
  /*
  //assert((logger_p != nullptr) && "logger_p "
  //                                   "(pointer of type util::io::Logger) is not initialized. "
  //                                   "Call util::io::initLogger(debug_level, log_filename) once at the beginning.");
  */
  if (logger_p == nullptr)
    logger_p = new Logger();

  logger_p->log(str, screen_out, printMpiRank);
}

void util::io::log(std::ostringstream &oss, bool screen_out, int printMpiRank) {

  // for now, we do not call assert and rather create a logger if it does not exist
  /*
  //assert((logger_p != nullptr) && "logger_p "
  //                                   "(pointer of type util::io::Logger) is not initialized. "
  //                                   "Call util::io::initLogger(debug_level, log_filename) once at the beginning.");
  */
  if (logger_p == nullptr)
    logger_p = new Logger();

  logger_p->log(oss, screen_out, printMpiRank);
}