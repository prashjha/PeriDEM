/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "wall.h"
#include "fe/mesh.h"
#include "geometry/fracture.h"
#include "inp/pdecks/particleDeck.h"
#include "util/geom.h"
#include "util/io.h"
#include "util/methods.h"
#include <material/materialUtil.h>

// hpx lib
#include <hpx/include/parallel_algorithm.hpp>

// Wall class
particle::Wall::Wall(size_t id, size_t wall_id, inp::WallZone &z_deck,
                     size_t z_id, fe::Mesh *mesh,
                     std::shared_ptr<model::ModelData> model_data,
                     bool populate_data)
    : BaseParticle("wall", id, wall_id, z_id, mesh->getDimension(),
                   mesh->getNumNodes(),
                   0.,
                   model_data),
      d_wallType(z_deck.d_type), d_mesh_p(mesh) {

  if (populate_data) {

    d_globStart = d_modelData_p->d_x.size();
    d_globEnd = d_modelData_p->d_x.size() + d_mesh_p->getNumNodes();
    for (size_t i = 0; i < d_mesh_p->getNumNodes(); i++) {

      d_modelData_p->d_xRef.push_back(d_mesh_p->getNode(i));
      d_modelData_p->d_x.push_back(d_mesh_p->getNode(i));
      d_modelData_p->d_u.push_back(util::Point());
      d_modelData_p->d_v.push_back(util::Point());
      d_modelData_p->d_f.push_back(util::Point());
      d_modelData_p->d_vol.push_back(d_mesh_p->getNodalVolume(i));
      d_modelData_p->d_fix.push_back(uint8_t(0));
      d_modelData_p->d_forceFixity.push_back(uint8_t(0));
      d_modelData_p->d_thetaX.push_back(0.);
      d_modelData_p->d_mX.push_back(0.);
      d_modelData_p->d_ptId.push_back(id); // id of this wall
    }
  }

  d_h = d_mesh_p->getMeshSize();

  // material class
  // initialize material class
  auto &material_deck = z_deck.d_matDeck;
  double horizon = material_deck.d_horizon;

  if (material_deck.d_materialType == "RNPBond")
    d_material_p = std::make_unique<material::RnpMaterial>(
        material_deck, d_mesh_p->getDimension(), horizon);
  else if (material_deck.d_materialType == "PMBBond")
    d_material_p = std::make_unique<material::PmbMaterial>(
        material_deck, d_mesh_p->getDimension(), horizon);
  else if (material_deck.d_materialType == "PDElasticBond")
    d_material_p = std::make_unique<material::PdElastic>(
        material_deck, d_mesh_p->getDimension(), horizon);
  else if (material_deck.d_materialType == "PDState")
    d_material_p = std::make_unique<material::PdState>(
        material_deck, d_mesh_p->getDimension(), horizon);

  d_horizon = horizon;
  d_density = d_material_p->getDensity();

  if (z_deck.d_allDofsConstrained)
    d_computeForce = false;

  // set contact radius for internal contact
  d_Rc = 0.9 * d_h;

  // set contact coefficient for internal contact
  d_Kn = (18. / (M_PI * std::pow(horizon, 5))) *
         d_material_p->computeMaterialProperties(getDimension()).d_K;
}