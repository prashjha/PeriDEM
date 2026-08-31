/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef TWOPARTICLE_DEMO_APPPOSTPROCESS_H
#define TWOPARTICLE_DEMO_APPPOSTPROCESS_H

#include "inp/materialDeck.h"
#include "postprocess/postprocess.h"

#include <vector>

namespace twoparticle_demo {

/*!
 * Library two-particle metrics plus shear-stress / Hertz CSV.
 * Set on PeriDEMModel with setPostprocess; do not subclass the driver.
 */
class AppPostprocess : public postprocess::Postprocess {
public:
  std::string twoParticle(data::ModelData &data) override;

private:
  void writeCsv(data::ModelData &data);
  void maxShearStress(data::ModelData &data);
  void hertzIdeals(data::ModelData &data);

  std::vector<inp::MatData> d_matData;
  double d_maxStress = 0.;
  double d_maxStressLocRef = 0.;
  double d_contactAreaRadiusIdeal = 0.;
  double d_maxStressIdeal = 0.;
  double d_maxStressLocRefIdeal = 0.;
  bool d_idealsSet = false;
};

} // namespace twoparticle_demo

#endif // TWOPARTICLE_DEMO_APPPOSTPROCESS_H
