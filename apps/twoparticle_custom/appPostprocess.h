/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef TWOPARTICLE_CUSTOM_APPPOSTPROCESS_H
#define TWOPARTICLE_CUSTOM_APPPOSTPROCESS_H

#include "postprocess/postprocess.h"

#include <fstream>
#include <string>

namespace twoparticle_custom {

/*!
 * @brief Library two-particle postprocess plus kinetic-energy CSV.
 *
 * Lives in this app, not in src/. Copy this class when writing extra output.
 */
class AppPostprocess : public postprocess::Postprocess {
public:
  void close(data::ModelData &data) override;
  std::string twoParticle(data::ModelData &data) override;
  void checkStop(data::ModelData &data) override;

private:
  void writeExtra(data::ModelData &data);
  std::ofstream d_extraFile;
};

} // namespace twoparticle_custom

#endif // TWOPARTICLE_CUSTOM_APPPOSTPROCESS_H
