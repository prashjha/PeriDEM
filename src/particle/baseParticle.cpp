/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "baseParticle.h"
#include "util/methods.h"
#include "fe/baseElem.h"
#include "fe/quadElem.h"
#include "fe/tetElem.h"
#include "fe/triElem.h"
#include "material/materialUtil.h"
#include "util/feElementDefs.h"
#include "util/geom.h"
#include <iostream>

particle::BaseParticle::BaseParticle(std::string particle_type)
:     d_type(particle_type),
      d_typeIndex(-1),
      d_id(0),
      d_typeId(0),
      d_zoneId(0),
      d_dim(0),
      d_particleDescription(""),
      d_isWall(false),
      d_numNodes(0),
      d_h(0.),
      d_horizon(0.),
      d_density(0.),
      d_allDofsConstrained(false),
      d_computeForce(true),
      d_material_p(nullptr),
      d_Rc(0.),
      d_Kn(0.),
      d_globStart(0),
      d_globEnd(0),
      d_globQuadStart(0),
      d_globQuadEnd(0),
      d_modelData_p(nullptr),
      d_rp_p(nullptr),
      d_geom_p(nullptr),
      d_tform(particle::ParticleTransform()),
      d_mesh_p(nullptr) {

  if (particle_type == "particle") {
    d_typeIndex = 0;
    d_isWall = false;
  }
  else if (particle_type == "wall") {
    d_typeIndex = 1;
    d_isWall = true;
  }
}

particle::BaseParticle::BaseParticle(std::string particle_type,
                                     size_t id,
                                     size_t particle_type_id,
                                     size_t zone_id,
                                     size_t dim,
                                     std::string particle_description,
                                     bool is_particle_a_wall,
                                     bool are_all_dofs_constrained,
                                     size_t num_nodes,
                                     double h,
                                     std::shared_ptr<model::ModelData> model_data,
                                     std::shared_ptr<particle::RefParticle> ref_particle,
                                     std::shared_ptr<util::geometry::GeomObject> geom,
                                     particle::ParticleTransform &transform,
                                     std::shared_ptr<fe::Mesh> mesh,
                                     inp::MaterialDeck &material_deck,
                                     bool populate_data)
        : d_type(particle_type),
          d_typeIndex(-1),
          d_id(id),
          d_typeId(particle_type_id),
          d_zoneId(zone_id),
          d_dim(dim),
          d_particleDescription(particle_description),
          d_isWall(is_particle_a_wall),
          d_numNodes(num_nodes),
          d_h(h),
          d_horizon(0),
          d_density(0),
          d_allDofsConstrained(are_all_dofs_constrained),
          d_computeForce(true),
          d_material_p(nullptr),
          d_Rc(0.),
          d_Kn(0.),
          d_globStart(0),
          d_globEnd(0),
          d_globQuadStart(0),
          d_globQuadEnd(0),
          d_modelData_p(model_data),
          d_rp_p(ref_particle),
          d_geom_p(geom),
          d_tform(transform),
          d_mesh_p(mesh),
          d_pRadius(geom->boundingRadius()) {

  if (d_type == "particle")
    d_typeIndex = 0;
  else if (d_type == "wall")
    d_typeIndex = 1;

  d_computeForce = !d_allDofsConstrained;

  // check
  if (d_type == "particle" and d_isWall) {
    std::cerr << "Error: Can not have d_type = 'particle' and d_isWall = true.\n";
    exit(EXIT_FAILURE);
  }
  if (d_type == "wall" and !d_isWall) {
    std::cerr << "Error: Can not have d_type = 'wall' and d_isWall = false.\n";
    exit(EXIT_FAILURE);
  }

  if (populate_data) {

    d_globStart = d_modelData_p->d_x.size();
    d_globEnd = d_modelData_p->d_x.size() + d_rp_p->getNumNodes();

    for (size_t i = 0; i < d_rp_p->getNumNodes(); i++) {

      d_modelData_p->d_xRef.push_back(d_tform.apply(d_rp_p->getNode(i)));
      d_modelData_p->d_x.push_back(d_tform.apply(d_rp_p->getNode(i)));
      d_modelData_p->d_u.push_back(util::Point());
      d_modelData_p->d_v.push_back(util::Point());
      d_modelData_p->d_vMag.push_back(0.);
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
  inp::MaterialDeck p_material_deck = material_deck;
  // double horizon = d_geom_p->inscribedRadius();
  double horizon = p_material_deck.d_horizon;
  if (p_material_deck.d_horizonMeshRatio > 0.)
    horizon = p_material_deck.d_horizonMeshRatio * d_h;

  if (p_material_deck.d_materialType == "RNPBond")
    d_material_p = std::make_unique<material::RnpMaterial>(
            p_material_deck, d_rp_p->getDimension(), horizon);
  else if (p_material_deck.d_materialType == "PMBBond")
    d_material_p = std::make_unique<material::PmbMaterial>(
            p_material_deck, d_rp_p->getDimension(), horizon);
  else if (p_material_deck.d_materialType == "PDElasticBond")
    d_material_p = std::make_unique<material::PdElastic>(
            p_material_deck, d_rp_p->getDimension(), horizon);
  else if (p_material_deck.d_materialType == "PDState")
    d_material_p = std::make_unique<material::PdState>(
            p_material_deck, d_rp_p->getDimension(), horizon);

  d_horizon = horizon;
  d_density = d_material_p->getDensity();

  // d_material_p->print();

  // set contact radius for internal contact
  d_Rc = 0.95 * d_h;

  // set contact coefficient for internal contact
  d_Kn = (18. / (M_PI * std::pow(horizon, 5))) *
         d_material_p->computeMaterialProperties(getDimension()).d_K;

  if (!d_computeForce) {
    std::cout << "Warning: Compute force is OFF in particle with id = "
              << d_id << "\n";
  }
  if (d_allDofsConstrained) {
    std::cout << "Warning: All DoFs are OFF in particle with id = "
              << d_id << "\n";
  }
}

std::string particle::BaseParticle::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "------- BaseParticle --------" << std::endl
      << std::endl;

  oss << tabS << "d_type = " << d_type << std::endl;
  oss << tabS << "d_particleDescription = " << d_particleDescription << std::endl;
  oss << tabS << "d_typeIndex = " << d_typeIndex << std::endl;
  oss << tabS << "d_isWall = " << d_isWall << std::endl;
  oss << tabS << "d_id = " << d_id << std::endl;
  oss << tabS << "d_typeId = " << d_typeId << std::endl;
  oss << tabS << "d_zoneId = " << d_zoneId << std::endl;
  oss << tabS << "d_dim = " << d_dim << std::endl;
  oss << tabS << "d_numNodes = " << d_numNodes << std::endl;
  oss << tabS << "d_pRadius = " << d_pRadius << std::endl;
  oss << tabS << "d_h = " << d_h << std::endl;
  oss << tabS << "d_allDofsConstrained = " << d_allDofsConstrained << std::endl;
  oss << tabS << "d_computeForce = " << d_computeForce << std::endl;
  oss << tabS << "d_horizon = " << d_horizon << std::endl;
  oss << tabS << "d_density = " << d_density << std::endl;
  oss << tabS << "d_Rc = " << d_Rc << std::endl;
  oss << tabS << "d_Kn = " << d_Kn << std::endl;
  oss << tabS << "d_globStart = " << d_globStart << std::endl;
  oss << tabS << "d_globEnd = " << d_globEnd << std::endl;
  oss << tabS << "d_globQuadStart = " << d_globQuadStart << std::endl;
  oss << tabS << "d_globQuadEnd = " << d_globQuadEnd << std::endl;
  oss << tabS << std::endl;
  oss << tabS << std::endl;
  oss << tabS << "Ref particle info = " << std::endl;
  oss << d_rp_p->printStr(nt + 1, lvl);
  oss << tabS << "Geometry info: " << std::endl;
  oss << d_geom_p->printStr(nt + 1, lvl);

  oss << tabS << std::endl;

  return oss.str();
}