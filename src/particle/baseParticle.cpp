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
#include "util/vecMethods.h"
#include "fe/elemIncludes.h"
#include "material/materialUtil.h"
#include "util/feElementDefs.h"
#include "geom/geomIncludes.h"
#include <iostream>

particle::BaseParticle::BaseParticle(size_t id)
:     d_isWall(false),
      d_id(id),
      d_dim(0),
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
      d_tform(geom::ParticleTransform()),
      d_mesh_p(nullptr) {}

particle::BaseParticle::BaseParticle(size_t id,
                                     bool isWall,
                                     size_t dim,
                                     std::map<std::string, size_t> groups,
                                     bool are_all_dofs_constrained,
                                     size_t num_nodes,
                                     double h,
                                     std::shared_ptr<model::ModelData> model_data,
                                     std::shared_ptr<particle::RefParticle> ref_particle,
                                     std::shared_ptr<geom::GeomObject> geom,
                                     geom::ParticleTransform &transform,
                                     std::shared_ptr<mesh::Mesh> mesh,
                                     inp::MaterialDeck &material_deck,
                                     bool populate_data)
        : d_id(id),
          d_dim(dim),
          d_isWall(isWall),
          d_groups(groups),
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

  d_computeForce = !d_allDofsConstrained;

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

  d_h = geom::computeMeshSize(d_modelData_p->d_x, d_globStart, d_globEnd);

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

  oss << tabS << "d_isWall = " << d_isWall << std::endl;
  oss << tabS << "d_id = " << d_id << std::endl;
  oss << tabS << "d_dim = " << d_dim << std::endl;
  //oss << tabS << "d_groups = " << util::io::printStr<std::string, size_t>(d_groups) << std::endl;
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