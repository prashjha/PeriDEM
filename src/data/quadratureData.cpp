/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "modelData.h"

#include "fe/elemIncludes.h"
#include "particle/baseParticle.h"
#include "particle/refParticle.h"
#include "util/vecMethods.h"

#include <cstdlib>
#include <format>
#include <iostream>

namespace {

size_t numQuadPoints(size_t elemType, size_t quadOrder) {
  return fe::elem(elemType, quadOrder)->getNumQuadPoints();
}

} // namespace

void data::setupQuadratureData(ModelData &data) {
  if (!util::methods::isTagInList("Strain_Stress", data.d_outputDeck_p->d_outTags)
      and !data.d_modelDeck_p->d_populateElementNodeConnectivity)
    return;

  for (auto &p : data.d_referenceParticles) {
    auto &particle_mesh_p = p->getMeshP();
    if (!particle_mesh_p->d_encDataPopulated && particle_mesh_p->d_enc.empty())
      particle_mesh_p->readElementData(particle_mesh_p->d_filename);
  }

  size_t totalQuadPoints = 0;
  const auto quadOrder = data.d_modelDeck_p->d_quadOrder;
  for (auto &p : data.d_particlesListTypeAll) {
    const auto &particle_mesh_p = p->getMeshP();
    const auto nq = particle_mesh_p->getNumElements() *
                    numQuadPoints(particle_mesh_p->getElementType(), quadOrder);

    p->d_globQuadStart = totalQuadPoints;
    totalQuadPoints += nq;
    p->d_globQuadEnd = totalQuadPoints;

    std::cout << std::format("p->id() = {}, "
                             "p->d_globQuadStart = {}, "
                             "totalQuadPoints = {}, "
                             "p->d_globQuadEnd = {}",
                             p->getId(), p->d_globQuadStart, nq, p->d_globQuadEnd)
              << std::endl;
  }

  data.d_xQuadCur.resize(totalQuadPoints);
  data.d_strain.resize(totalQuadPoints);
  data.d_stress.resize(totalQuadPoints);
}
