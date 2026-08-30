/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "appPostprocess.h"

#include "data/modelData.h"
#include "inp/input.h"

#include <format>

void twoparticle_custom::AppPostprocess::close(data::ModelData &data) {
  postprocess::Postprocess::close(data);
  if (d_extraFile.is_open())
    d_extraFile.close();
}

void twoparticle_custom::AppPostprocess::writeExtra(data::ModelData &data) {
  if (!d_extraFile.is_open()) {
    const std::string filename = data.d_outputDeck_p->d_path + "app_pp.csv";
    d_extraFile.open(filename);
    d_extraFile << "t, ke, contact_fpair\n";
  }

  double ke = 0.;
  for (size_t i = 0; i < data.d_v.size(); i++)
    ke += 0.5 * data.getDensity(i) * data.getVol(i) *
          data.d_v[i].length() * data.d_v[i].length();

  d_extraFile << std::format("{:.6e}, {:.6e}, {:.6e}\n", data.d_time, ke,
                             data.getKeyData("app_contact_fpair"));
}

std::string twoparticle_custom::AppPostprocess::twoParticle(
    data::ModelData &data) {
  auto msg = postprocess::Postprocess::twoParticle(data);
  writeExtra(data);
  return msg;
}

void twoparticle_custom::AppPostprocess::checkStop(data::ModelData &data) {
  postprocess::Postprocess::checkStop(data);
  if (data.d_testDeck_p->d_testName != "two_particle")
    writeExtra(data);
}
