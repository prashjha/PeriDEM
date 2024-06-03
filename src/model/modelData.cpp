/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "modelData.h"
#include "particle/baseParticle.h"
#include "particle/particle.h"
#include "particle/wall.h"
#include "particle/refParticle.h"

double model::ModelData::getDensity(size_t i) {
  return d_allParticles[d_ptId[i]]->getDensity();
};
double model::ModelData::getHorizon(size_t i) {
  return d_allParticles[d_ptId[i]]->getHorizon();
};