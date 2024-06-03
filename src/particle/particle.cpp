/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "particle.h"
#include "util/methods.h"
#include "fe/baseElem.h"
#include "fe/quadElem.h"
#include "fe/tetElem.h"
#include "fe/triElem.h"
#include "material/materialUtil.h"
#include "util/feElementDefs.h"
#include "util/geom.h"
#include <iostream>

// Particle class
particle::Particle::Particle(
    size_t id, inp::ParticleZone &z_deck, size_t particle_zone,
    std::shared_ptr<particle::RefParticle> ref_particle,
    std::shared_ptr<util::geometry::GeomObject> geom,
    const particle::ParticleTransform &transform,
    std::shared_ptr<model::ModelData> model_data,
    bool populate_data)
    : BaseParticle("particle", id, id, particle_zone,
                   ref_particle->getDimension(),
                   ref_particle->getNumNodes(), 0., model_data),
      d_rp_p(ref_particle), d_geom_p(geom), d_tform(transform) {

  if (populate_data) {

    d_globStart = d_modelData_p->d_x.size();
    d_globEnd = d_modelData_p->d_x.size() + d_rp_p->getNumNodes();
    for (size_t i = 0; i < d_rp_p->getNumNodes(); i++) {

      d_modelData_p->d_xRef.push_back(d_tform.apply(d_rp_p->getNode(i)));
      d_modelData_p->d_x.push_back(d_tform.apply(d_rp_p->getNode(i)));
      d_modelData_p->d_u.push_back(util::Point());
      d_modelData_p->d_v.push_back(util::Point());
      d_modelData_p->d_f.push_back(util::Point());
      d_modelData_p->d_vol.push_back(
          d_rp_p->getNodalVolume(i) *
          std::pow(d_tform.d_scale, d_rp_p->getDimension()));
      d_modelData_p->d_fix.push_back(uint8_t(0));
      d_modelData_p->d_forceFixity.push_back(uint8_t(0));
      d_modelData_p->d_thetaX.push_back(0.);
      d_modelData_p->d_mX.push_back(0.);
      d_modelData_p->d_ptId.push_back(id); // id of this particle
    }
  }

  d_h = util::computeMeshSize(d_modelData_p->d_x, d_globStart,
                                        d_globEnd);

  // initialize material class
  inp::MaterialDeck material_deck = z_deck.d_matDeck;
  // double horizon = d_geom_p->inscribedRadius();
  double horizon = material_deck.d_horizon;
  if (material_deck.d_horizonMeshRatio > 0.)
    horizon = material_deck.d_horizonMeshRatio * d_h;

  if (material_deck.d_materialType == "RNPBond")
    d_material_p = std::make_unique<material::RnpMaterial>(
        material_deck, d_rp_p->getDimension(), horizon);
  else if (material_deck.d_materialType == "PMBBond")
    d_material_p = std::make_unique<material::PmbMaterial>(
        material_deck, d_rp_p->getDimension(), horizon);
  else if (material_deck.d_materialType == "PDElasticBond")
    d_material_p = std::make_unique<material::PdElastic>(
        material_deck, d_rp_p->getDimension(), horizon);
  else if (material_deck.d_materialType == "PDState")
    d_material_p = std::make_unique<material::PdState>(
        material_deck, d_rp_p->getDimension(), horizon);

  d_horizon = horizon;
  d_density = d_material_p->getDensity();

  // d_material_p->print();

  // set contact radius for internal contact
  d_Rc = 0.95 * d_h;

  // set contact coefficient for internal contact
  d_Kn = (18. / (M_PI * std::pow(horizon, 5))) *
         d_material_p->computeMaterialProperties(getDimension()).d_K;
}