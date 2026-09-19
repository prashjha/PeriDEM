/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "contact/pairForce.h"
#include "contact/policy.h"
#include "contact/wallContact.h"
#include "geom/geomObjects.h"
#include "pd/selfContact.h"
#include "util/io.h"
#include "inp/contactPairDeck.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

bool near(double a, double b, double tol = 1.e-12) {
  return std::abs(a - b) <= tol * (1. + std::abs(a) + std::abs(b));
}

} // namespace

int main() {
  // Partial volume near contact radius: full at R = Rc - h/2, zero at Rc + h/2.
  {
    const double vol = 4.0;
    const double Rc = 1.0;
    const double h = 0.2;
    const double full = contact::correctedContactVolume(vol, 0.5, Rc, h);
    const double mid = contact::correctedContactVolume(vol, Rc, Rc, h);
    const double edge = contact::correctedContactVolume(vol, Rc + 0.5 * h, Rc, h);
    if (!near(full, vol) || !near(mid, 0.5 * vol) || !near(edge, 0.)) {
      std::cerr << "correctedContactVolume failed: full=" << full
                << " mid=" << mid << " edge=" << edge << "\n";
      return 1;
    }
  }

  inp::ContactPairDeck deck(/*contactR*/ 1.0, /*computeContactR*/ false,
                            /*dampingOn*/ false, /*frictionOn*/ false,
                            /*Kn*/ 100.0);

  contact::Pair p{deck,
                  util::Point(0., 0., 0.),
                  util::Point(0.5, 0., 0.),
                  util::Point(),
                  util::Point(),
                  0,
                  1,
                  0,
                  1,
                  /*voli*/ 2.0,
                  /*volj*/ 3.0,
                  1.0,
                  1.0,
                  1.e-6,
                  false,
                  false};

  auto law_j = contact::makePairForce("coulomb_simple");

  // Force density spring ∝ Kn*(R-Rc)*Vj (partial volume applied by assembly).
  const auto fj = law_j->springForce(p);
  if (!near(fj.d_x, -150.)) {
    std::cerr << "spring should equal Kn*(R-Rc)*Vj: fj.x=" << fj.d_x << "\n";
    return 1;
  }

  // Nodal force would be fj*Vi; density-form integrator uses fj directly.
  const auto nodal = p.voli * fj;
  if (!near(nodal.d_x, -300.)) {
    std::cerr << "nodal force Vi*fj failed: " << nodal.d_x << "\n";
    return 1;
  }

  // Node damping fires only when Damping_On; law flags gate COM vs node.
  {
    inp::ContactPairDeck dd(/*contactR*/ 1.0, /*computeContactR*/ false,
                            /*dampingOn*/ true, /*frictionOn*/ false,
                            /*Kn*/ 100.0);
    dd.d_K = 1.e6;
    dd.d_betan = 0.2;
    contact::Pair pd{dd,
                     util::Point(0., 0., 0.),
                     util::Point(0.5, 0., 0.),
                     util::Point(1., 0., 0.),
                     util::Point(0., 0., 0.), // approaching
                     0,
                     1,
                     0,
                     1,
                     2.0,
                     3.0,
                     1.0,
                     1.0,
                     1.e-6,
                     false,
                     false};
    const auto fd = law_j->nodeDampingForce(pd);
    if (!(fd.length() > 0.)) {
      std::cerr << "node damping should be nonzero for approaching pair\n";
      return 1;
    }
    if (contact::usesNodeDamping("off") || !contact::usesNodeDamping("node") ||
        !contact::usesNodeDamping("com_and_node")) {
      std::cerr << "Damping_Law node flags incorrect\n";
      return 1;
    }
  }

  // Stick-slip: tangential spring capped by mu * |Fn|.
  {
    inp::ContactPairDeck fd(/*contactR*/ 1.0, /*computeContactR*/ false,
                            /*dampingOn*/ false, /*frictionOn*/ true,
                            /*Kn*/ 100.0, /*eps*/ 1., /*mu*/ 0.2);
    contact::Pair q{fd,
                    util::Point(0., 0., 0.),
                    util::Point(0.5, 0., 0.),
                    util::Point(0., 0., 0.),
                    util::Point(0., 10., 0.), // large tangential approach
                    0,
                    1,
                    0,
                    1,
                    1.0,
                    1.0,
                    1.0,
                    1.0,
                    1.e-3,
                    false,
                    false};
    auto ss = contact::makePairForce("stick_slip");
    ss->beginStep();
    // Accumulate enough tangential slip to exceed the Coulomb limit.
    util::Point fss;
    for (int k = 0; k < 20; ++k)
      fss = ss->springForce(q);
    ss->endStep();

    const double fn = 100.0 * (1.0 - 0.5) * 1.0;
    const double ft_max = 0.2 * fn;
    const double ft = std::abs(fss.d_y);
    if (ft > ft_max + 1.e-9) {
      std::cerr << "stick-slip exceeded Coulomb cap: ft=" << ft
                << " ft_max=" << ft_max << "\n";
      return 1;
    }
    if (ft < 0.5 * ft_max) {
      std::cerr << "stick-slip did not mobilize friction: ft=" << ft << "\n";
      return 1;
    }
  }

  std::cout << "TestContact spring / stick_slip OK\n";

  // Damping_Law selection: COM object vs node-damping flag.
  {
    if (contact::makeDamping("off") != nullptr ||
        contact::makeDamping("node") != nullptr) {
      std::cerr << "off/node should not create COM damping\n";
      return 1;
    }
    if (contact::makeDamping("com") == nullptr ||
        contact::makeDamping("com_and_node") == nullptr) {
      std::cerr << "com/com_and_node should create COM damping\n";
      return 1;
    }
    if (!contact::usesNodeDamping("com_and_node") ||
        !contact::usesNodeDamping("node") ||
        contact::usesNodeDamping("com") || contact::usesNodeDamping("off") ||
        !contact::usesComDamping("com") ||
        contact::usesComDamping("node")) {
      std::cerr << "damping law flags incorrect\n";
      return 1;
    }
  }

  std::cout << "TestContact damping laws OK\n";

  // Reference-aware self-contact formula vs broken_bond_kn (natural R = Rc).
  // Force: Kn * volj * capped_gap / R * yji, gap = R - natural_R, cap = -0.25*natural_R.
  {
    auto bb = pd::makeSelfContact("broken_bond_kn");
    auto rg = pd::makeSelfContact("reference_gap");
    const util::Point yji(0.4, 0., 0.); // R = 0.4
    const double volj = 1.0;
    const double Kn = 10.0;
    const double Rc = 1.0;
    const double r0 = 0.5;
    const auto f_bb = bb->force(yji, volj, Kn, Rc, r0);
    const auto f_rg = rg->force(yji, volj, Kn, Rc, r0);
    if (!near(f_bb.d_x, -2.5)) {
      std::cerr << "broken_bond_kn unexpected: " << f_bb.d_x << "\n";
      return 1;
    }
    if (!near(f_rg.d_x, -1.0)) {
      std::cerr << "reference_gap unexpected: " << f_rg.d_x << "\n";
      return 1;
    }
    // No force when stretched beyond r0 for reference_gap.
    const auto f_rg2 = rg->force(util::Point(0.8, 0., 0.), volj, Kn, Rc, r0);
    if (!near(f_rg2.d_x, 0.) || !near(f_rg2.length(), 0.)) {
      std::cerr << "reference_gap should be zero when R > r0\n";
      return 1;
    }
  }

  std::cout << "TestContact self-contact OK\n";

  // Wall_Contact factory + geom wallContactQuery (plane / rectangle).
  {
    auto meshed = contact::makeWallContact("meshed");
    auto anal = contact::makeWallContact("analytical_plane");
    if (!meshed || meshed->skipsMeshedGrainWall() || !anal ||
        !anal->skipsMeshedGrainWall()) {
      std::cerr << "Wall_Contact factory flags incorrect\n";
      return 1;
    }

    geom::Plane plane(util::Point(0., 1., 0.), util::Point(0., 0., 0.));
    geom::WallContactHit hit;
    if (!plane.wallContactQuery(util::Point(0., 0.3, 0.), hit) ||
        !near(hit.signed_gap, 0.3) || !near(hit.outward_n.d_y, 1.)) {
      std::cerr << "Plane wallContactQuery free-space failed\n";
      return 1;
    }
    if (!plane.wallContactQuery(util::Point(0., -0.2, 0.), hit) ||
        !near(hit.signed_gap, -0.2) || !near(hit.outward_n.d_y, 1.)) {
      std::cerr << "Plane wallContactQuery penetration failed\n";
      return 1;
    }

    geom::Rectangle rect(2.0, 1.0, util::Point(0., 0., 0.));
    if (!rect.wallContactQuery(util::Point(0., 0.8, 0.), hit) ||
        !near(hit.signed_gap, 0.3) || !near(hit.outward_n.d_y, 1.)) {
      std::cerr << "Rectangle wallContactQuery failed: gap=" << hit.signed_gap
                << " ny=" << hit.outward_n.d_y << "\n";
      return 1;
    }

    // Analytical wall spring density matches Kn*(gap-Rc)*voli * outward.
    const double Kn = 100., Rc = 1., vol = 2.;
    plane.wallContactQuery(util::Point(0., 0.3, 0.), hit);
    const double gap = hit.signed_gap;
    auto scalar = Kn * (gap - Rc) * vol;
    if (scalar > 0.)
      scalar = 0.;
    const util::Point f = scalar * (-1. * hit.outward_n);
    if (!near(f.d_y, 140.) || !near(f.d_x, 0.)) {
      std::cerr << "analytical wall force failed: f=(" << f.d_x << ","
                << f.d_y << ")\n";
      return 1;
    }
  }

  std::cout << "TestContact wall contact OK\n";

  // coulomb_simple vs stick_slip differ in tangential response.
  {
    inp::ContactPairDeck fd(/*contactR*/ 1.0, /*computeContactR*/ false,
                            /*dampingOn*/ false, /*frictionOn*/ true,
                            /*Kn*/ 100.0, /*eps*/ 1., /*mu*/ 0.2);
    contact::Pair q{fd,
                    util::Point(0., 0., 0.),
                    util::Point(0.5, 0., 0.),
                    util::Point(0., 0., 0.),
                    util::Point(0., 5., 0.),
                    0,
                    1,
                    0,
                    1,
                    1.0,
                    1.0,
                    1.0,
                    1.0,
                    1.e-3,
                    false,
                    false};
    auto cs = contact::makePairForce("coulomb_simple");
    auto ss = contact::makePairForce("stick_slip");
    const auto f_cs = cs->springForce(q);
    ss->beginStep();
    util::Point f_ss;
    for (int k = 0; k < 5; ++k)
      f_ss = ss->springForce(q);
    ss->endStep();
    // coulomb_simple: instantaneous mu*|Fn| along et; stick_slip builds spring.
    if (near(f_cs.d_y, f_ss.d_y, 1.e-6) && near(f_cs.d_x, f_ss.d_x, 1.e-6)) {
      std::cerr << "coulomb_simple and stick_slip should differ in Ft\n";
      return 1;
    }
  }

  std::cout << "TestContact friction laws differ OK\n";
  return 0;
}
