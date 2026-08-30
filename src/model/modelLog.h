/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MODEL_MODELLOG_H
#define MODEL_MODELLOG_H

#include "modelData.h"
#include "util/io.h"
#include <sstream>
#include <string>

namespace model {

/*! @brief Same debug gate as DEMModel::log, for library kernels that take ModelData. */
inline void log(ModelData &data, const std::string &str, int priority = 0,
                bool check_condition = true, int override_priority = -1,
                bool screen_out = false) {
  int op = override_priority == -1 ? priority : override_priority;
  if ((check_condition && data.d_outputDeck_p->d_debug > priority) ||
      data.d_outputDeck_p->d_debug > op)
    util::io::log(str, screen_out);
}

inline void log(ModelData &data, std::ostringstream &oss, int priority = 0,
                bool check_condition = true, int override_priority = -1,
                bool screen_out = false) {
  int op = override_priority == -1 ? priority : override_priority;
  if ((check_condition && data.d_outputDeck_p->d_debug > priority) ||
      data.d_outputDeck_p->d_debug > op)
    util::io::log(oss, screen_out);
}

} // namespace model

#endif // MODEL_MODELLOG_H
