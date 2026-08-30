/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "appDamping.h"

#include "data/modelData.h"
#include "particle/baseParticle.h"

void twoparticle_custom::AppDamping::apply(data::ModelData &data) {
  contact::Damping::apply(data);

  if (data.d_particlesListTypeAll.size() < 2)
    return;

  auto *p0 = data.d_particlesListTypeAll[0];
  auto *p1 = data.d_particlesListTypeAll[1];
  const auto vrel = p1->getVCenter() - p0->getVCenter();
  const double c = 1.0e-3;
  const auto fpair = vrel * c;

  const auto i0 = p0->getNodeId(p0->getCenterNodeId());
  const auto i1 = p1->getNodeId(p1->getCenterNodeId());
  if (p0->d_computeForce)
    data.d_f[i0] += fpair;
  if (p1->d_computeForce)
    data.d_f[i1] -= fpair;

  data.setKeyData("app_contact_fpair", fpair.length());
}
