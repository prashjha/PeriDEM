/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "refParticle.h"
#include "inp/pdecks/particleDeck.h"
#include "fe/baseElem.h"
#include "util/geom.h"
#include <iostream>

// Reference particle class
particle::RefParticle::RefParticle(inp::ParticleZone *z_deck, fe::Mesh *mesh)
    : d_mesh_p(mesh), d_centerNode(0), d_geom_p(z_deck->d_rParticle_p),
      d_pRadius(z_deck->d_rParticle_p->boundingRadius()) {

  if (d_pRadius < 1.0E-10) {
    std::cerr << "Error: Reference particle radius is too small.\n";
    exit(1);
  }

  // find the node which is closest to the particle center
  // also add node near boundary to list
  auto center = d_geom_p->center();
  auto dx = util::Point();
  double dist = d_geom_p->boundingRadius();
  for (size_t i = 0; i < mesh->getNumNodes(); i++) {
    dx = center - mesh->getNode(i);
    if (util::isLess(dx.length(), dist)) {
      dist = dx.length();
      d_centerNode = i;
    }
  }
}

std::string particle::RefParticle::printStr(int nt, int lvl) const {

  auto tabS = util::io::getTabS(nt);
  std::ostringstream oss;
  oss << tabS << "------- Reference particle --------" << std::endl
      << std::endl;

  oss << tabS << "Mesh pointer = " << d_mesh_p.get() << std::endl;
  oss << tabS << "Mesh info: " << std::endl;
  oss << d_mesh_p->printStr(nt + 1, lvl);
  oss << tabS << "Center node = " << d_centerNode << std::endl;
  oss << tabS << "Center node location = " << getNode(d_centerNode).printStr()
      << std::endl;
  oss << tabS << "Geometry info: " << std::endl;
  oss << d_geom_p->printStr(nt + 1, lvl);
  oss << tabS << "Radius = " << d_pRadius << std::endl;
  oss << tabS << "Num interior flag data = " << d_intFlags.size() << std::endl;

  oss << tabS << std::endl;

  return oss.str();
}