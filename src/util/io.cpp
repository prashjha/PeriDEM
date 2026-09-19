/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

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

bool util::io::logEnabled(int priority, bool check_condition,
                          int override_priority) {
  int dbg = 0;
  if (logger_p != nullptr && logger_p->d_deck_p != nullptr)
    dbg = logger_p->d_deck_p->d_debugLevel;
  int op = override_priority == -1 ? priority : override_priority;
  return (check_condition && dbg > priority) || dbg > op;
}

void util::io::log(int priority, const std::string &str, bool check_condition,
                   int override_priority, bool screen_out) {
  if (logEnabled(priority, check_condition, override_priority))
    log(str, screen_out);
}

void util::io::log(int priority, std::ostringstream &oss, bool check_condition,
                   int override_priority, bool screen_out) {
  if (logEnabled(priority, check_condition, override_priority))
    log(oss, screen_out);
  else {
    oss.str("");
    oss.clear();
  }
}
