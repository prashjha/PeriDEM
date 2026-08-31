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
#include "mesh/meshUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/point.h"

#include <cmath>

std::string twoparticle_demo::AppPostprocess::twoParticle(
    data::ModelData &data) {
  auto msg = postprocess::Postprocess::twoParticle(data);
  if (msg.empty())
    return msg;

  hertzIdeals(data);
  maxShearStress(data);
  writeCsv(data);
  return msg;
}

void twoparticle_demo::AppPostprocess::writeCsv(data::ModelData &data) {
  if (!data.d_ppFile.is_open()) {
    const auto tag = data.d_outputDeck_p->d_tagPPFile.empty()
                         ? "0"
                         : data.d_outputDeck_p->d_tagPPFile;
    const auto filename = data.d_outputDeck_p->d_path + "pp_" + tag + ".csv";
    data.d_ppFile.open(filename, std::ios_base::out);
    data.d_ppFile << "t, delta, cont_area_r, s_loc, s_val, max_dist, "
                     "cont_area_r_ideal, s_loc_ideal, s_val_ideal\n";
  }

  data.d_ppFile << data.d_time << ", " << -data.getKeyData("pen_dist") << ", "
                << data.getKeyData("contact_area_radius") << ", "
                << d_maxStressLocRef << ", " << d_maxStress << ", "
                << data.getKeyData("max_dist") << ", "
                << d_contactAreaRadiusIdeal << ", " << d_maxStressLocRefIdeal
                << ", " << d_maxStressIdeal << std::endl;
}

void twoparticle_demo::AppPostprocess::hertzIdeals(data::ModelData &data) {
  if (d_idealsSet)
    return;

  const auto &p1 = data.d_particlesListTypeAll[1];
  const double r = data.d_particlesListTypeAll[0]->d_geom_p->boundingRadius();
  const double mass = p1->getDensity() * M_PI * std::pow(r, 2.);
  const auto mat = p1->getMaterial()->computeMaterialProperties(
      data.d_modelDeck_p->d_dim);

  d_contactAreaRadiusIdeal = 3. * mass * std::abs(data.d_bcDeck_p->d_gravity[1]) *
                             2. * r * (1. - std::pow(mat.d_nu, 2.)) /
                             (4. * mat.d_E);
  d_contactAreaRadiusIdeal = std::pow(d_contactAreaRadiusIdeal, 1. / 3.);
  d_maxStressLocRefIdeal = r - 0.48 * d_contactAreaRadiusIdeal;
  d_maxStressIdeal = 0.93 * mass * std::abs(data.d_bcDeck_p->d_gravity[1]) /
                     (2. * M_PI * d_contactAreaRadiusIdeal *
                      d_contactAreaRadiusIdeal);
  d_idealsSet = true;
}

void twoparticle_demo::AppPostprocess::maxShearStress(data::ModelData &data) {
  if (d_matData.empty()) {
    for (auto &p : data.d_particlesListTypeAll) {
      if (d_matData.size() <= p->getId())
        d_matData.resize(p->getId() + 1);
      d_matData[p->getId()] = p->getMaterial()->computeMaterialProperties(
          p->getMeshP()->getDimension());
    }
  }

  double max_stress_t = 0.;
  auto max_stress_loc_ref_t = util::Point();

  for (auto &p : data.d_particlesListTypeAll) {
    const auto particle_mesh_p = p->getMeshP();
    mesh::getCurrentQuadPoints(particle_mesh_p.get(), data.d_xRef, data.d_u,
                               data.d_xQuadCur, p->d_globStart,
                               p->d_globQuadStart,
                               data.d_modelDeck_p->d_quadOrder);

    mesh::getStrainStress(particle_mesh_p.get(), data.d_xRef, data.d_u,
                          p->getMaterial()->isPlaneStrain(), data.d_strain,
                          data.d_stress, p->d_globStart, p->d_globQuadStart,
                          d_matData[p->getId()].d_nu,
                          d_matData[p->getId()].d_lambda,
                          d_matData[p->getId()].d_mu, true,
                          data.d_modelDeck_p->d_quadOrder);

    double p_max_stress = 0.;
    auto p_max_stress_loc_cur = util::Point();
    auto p_max_stress_loc_ref = util::Point();
    mesh::getMaxShearStressAndLoc(
        p->getMeshP().get(), data.d_xRef, data.d_u, data.d_stress, p_max_stress,
        p_max_stress_loc_ref, p_max_stress_loc_cur, p->d_globStart,
        p->d_globQuadStart, data.d_modelDeck_p->d_quadOrder);

    if (util::isGreater(p_max_stress, max_stress_t)) {
      max_stress_t = p_max_stress;
      auto p_center_node_id = p->d_globStart + p->d_rp_p->getCenterNodeId();
      max_stress_loc_ref_t = p_max_stress_loc_ref - data.d_xRef[p_center_node_id];
    }
  }

  d_maxStress = max_stress_t;
  d_maxStressLocRef = max_stress_loc_ref_t.length();
}
