////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "io.h"

namespace {

  static util::io::Logger *logger = nullptr;
}

void util::io::initLogger(int debug_level, std::string filename) {

  if (logger != nullptr)
    return;

  auto deck = new LoggerDeck(debug_level, filename);
  logger = new Logger(deck);
}

void util::io::log(const std::string & str, bool screen_out) {

  if (logger == nullptr)
    logger = new Logger();

  logger->log(str, screen_out);
}

void util::io::log(std::ostringstream &oss, bool screen_out) {

  if (logger == nullptr)
    logger = new Logger();

  logger->log(oss, screen_out);
}