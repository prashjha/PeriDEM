/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "createParticles.h"

#include "baseParticle.h"
#include "refParticle.h"
#include "geom/geomIncludes.h"
#include "geom/geomObjectsUtil.h"
#include "inp/input.h"
#include "mesh_gen/particleMesh.h"
#include "data/modelData.h"
#include "util/io.h"
#include "util/point.h"
#include "util/randomDist.h"

#include <cmath>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

std::shared_ptr<data::ModelData> modelAlias(data::ModelData &data) {
  return std::shared_ptr<data::ModelData>(&data, [](data::ModelData *) {});
}

} // namespace

void particle::createParticleUsingParticleZoneGeomObject(data::ModelData &data) {
  util::io::log(1, data.d_name + ": Creating particle using Particle Zone Geometry Object\n");

  auto p_transform = geom::ParticleTransform();
  auto model_ptr = modelAlias(data);

  for (size_t z = 0; z < data.d_particleDeck_p->d_pMeshVec.size(); z++) {
    std::map<std::string, size_t> p_group(
        {{"geom_id", z}, {"mat_id", 0}, {"contact_id", 0}});

    auto ref_p = data.d_referenceParticles[z];

    auto p = new particle::BaseParticle(
        data.d_particlesListTypeAll.size(), false, ref_p->getDimension(),
        p_group, false, ref_p->getNumNodes(), 0., model_ptr, ref_p,
        ref_p->getGeomP(), p_transform, ref_p->getMeshP(),
        data.d_particleDeck_p->d_pMaterialVec[0], true);

    data.d_particlesListTypeParticle.push_back(p);
    data.d_particlesListTypeAll.push_back(p);
  }
}

void particle::createParticlesFromFile(data::ModelData &data) {
  util::io::log(1, data.d_name + ": Creating particle from file\n");

  auto &pgen_deck = data.d_particleDeck_p->d_pGenDeck;
  auto &pgen_json = pgen_deck.d_pGenJson;

  util::DistributionSample<UniformDistribution> uniform_dist(
      0., 1., data.d_modelDeck_p->d_seed);

  size_t num_particles = pgen_json.value("N", 0);
  if (num_particles == 0)
    throw std::runtime_error("No particles found in particle generation data");

  auto model_ptr = modelAlias(data);

  for (size_t i = 0; i < num_particles; i++) {
    auto p_data = pgen_json.at(std::to_string(i));

    std::map<std::string, size_t> p_group({
        {"geom_id", p_data.at("geom_id").get<size_t>()},
        {"mat_id", p_data.at("mat_id").get<size_t>()},
        {"contact_id", p_data.at("contact_id").get<size_t>()},
    });

    auto site = util::Point(p_data.at("x").get<double>(),
                            p_data.at("y").get<double>(),
                            p_data.at("z").get<double>());

    double angle = 0.;
    double scale = p_data.value("s", double(1.));

    if (p_data.find("theta") != p_data.end()) {
      angle = p_data.at("theta").get<double>();
    } else {
      if (pgen_deck.d_genWithRandomRotation) {
        angle = util::transform_to_uniform_dist(0., 2. * M_PI, uniform_dist());
      }
    }

    auto axis = util::Point(p_data.value("ax", 0.), p_data.value("ay", 0.),
                            p_data.value("az", 1.));
    const bool has_rotationPoint =
        p_data.find("rotx") != p_data.end() &&
        p_data.find("roty") != p_data.end() &&
        p_data.find("rotz") != p_data.end();

    auto &ref_p = data.d_referenceParticles[p_group["geom_id"]];
    const auto &rep_geom_p = ref_p->d_geom_p;

    std::shared_ptr<geom::GeomObject> p_geom(
        geom::createGeomDeepCopy(rep_geom_p.get()));
    const util::Point c0 = p_geom->center();
    const util::Point t = site - c0;
    util::Point rotationPivot =
        has_rotationPoint
            ? util::Point(p_data.value("rotx", 0.), p_data.value("roty", 0.),
                          p_data.value("rotz", 0.))
            : c0;
    p_geom->transform(t, scale, angle, axis, &rotationPivot);

    auto p_transform =
        geom::ParticleTransform(t, axis, angle, scale, rotationPivot);

    const bool is_wall = p_data.value("is_wall", false);

    auto p = new particle::BaseParticle(
        data.d_particlesListTypeAll.size(), is_wall, ref_p->getDimension(),
        p_group, false, ref_p->getNumNodes(), 0., model_ptr, ref_p, p_geom,
        p_transform, ref_p->getMeshP(),
        data.d_particleDeck_p->d_pMaterialVec[p_group["mat_id"]], true);

    if (is_wall)
      data.d_particlesListTypeWall.push_back(p);
    else
      data.d_particlesListTypeParticle.push_back(p);
    data.d_particlesListTypeAll.push_back(p);
  }
}

void particle::createReferenceParticles(data::ModelData &data) {
  data.d_particlesListTypeParticle.resize(0);
  data.d_particlesListTypeAll.resize(0);
  data.d_particlesListTypeWall.resize(0);
  data.d_referenceParticles.clear();

  if (data.d_particleDeck_p->d_pGeomVec.size() == 0)
    throw std::runtime_error(
        "No particle geometry groups found in particle deck");

  if (data.d_particleDeck_p->d_pGeomVec.size() !=
      data.d_particleDeck_p->d_pMeshVec.size())
    throw std::runtime_error(
        "Number of particle geometry groups must be equal to number of "
        "particle mesh groups");

  auto model_ptr = modelAlias(data);

  for (size_t z = 0; z < data.d_particleDeck_p->d_pMeshVec.size(); z++) {
    auto &zmeshDeck = data.d_particleDeck_p->d_pMeshVec[z];
    auto &zgeomDeck = data.d_particleDeck_p->d_pGeomVec[z];

    util::io::log(0, data.d_name +
                   ": Creating mesh for reference particle in mesh group = " +
                   std::to_string(z) + "\n");

    auto mesh = mesh_gen::createParticleMesh(
        zmeshDeck, zgeomDeck, data.d_modelDeck_p.get(), data.d_name);

    util::io::log(0, data.d_name +
                   ": Creating reference particle in mesh group = " +
                   std::to_string(z) + "\n");

    auto &rep_geom_p = zgeomDeck.d_geom_p;

    auto ref_p = std::make_shared<particle::RefParticle>(
        data.d_referenceParticles.size(), model_ptr, rep_geom_p, mesh);

    data.d_referenceParticles.emplace_back(ref_p);
  }
}

void particle::createParticles(data::ModelData &data) {
  createReferenceParticles(data);

  if (data.d_particleDeck_p->d_pGenDeck.d_genMethod == "From_File") {
    createParticlesFromFile(data);
  } else if (data.d_particleDeck_p->d_pGenDeck.d_genMethod ==
             "Use_Particle_Geometry") {
    createParticleUsingParticleZoneGeomObject(data);
  } else {
    throw std::runtime_error(
        "Error: Particle generation method = " +
        data.d_particleDeck_p->d_pGenDeck.d_genMethod + " is invalid.");
  }
}
