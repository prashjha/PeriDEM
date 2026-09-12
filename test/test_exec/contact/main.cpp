/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "contact/pairForce.h"
#include "contact/policy.h"
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

  auto law_j = contact::makePairForce("volume_j");
  auto law_p = contact::makePairForce("volume_product");

  // Both pair kernels use Vj only; volume_product applies Vi after the loop.
  const auto fj = law_j->springForce(p);
  const auto fp = law_p->springForce(p);
  if (!near(fj.d_x, fp.d_x) || !near(fj.d_x, -150.)) {
    std::cerr << "spring kernels should match and equal Kn*(R-Rc)*Vj: fj.x="
              << fj.d_x << " fp.x=" << fp.d_x << "\n";
    return 1;
  }

  const auto assembled = p.voli * fj; // after-loop × Vi
  if (!near(assembled.d_x, -300.)) {
    std::cerr << "after-loop Vi*Vj product failed: " << assembled.d_x << "\n";
    return 1;
  }

  std::cout << "TestContact volume_product OK\n";
  return 0;
}
