/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <PeriDEMConfig.h>

#include "inp/input.h"
#include "model/dem/demModel.h"
#include "util/function.h"
#include "util/geomObjects.h"
#include "util/matrix.h"
#include "util/methods.h"
#include "util/point.h"
#include "inp/decks/materialDeck.h"
#include "inp/decks/modelDeck.h"
#include "inp/decks/outputDeck.h"
#include "inp/decks/restartDeck.h"
#include "rw/vtkParticleWriter.h"
#include "fmt/format.h"
#include "inp/pdecks/contactDeck.h"
#include "rw/reader.h"
#include "util/methods.h"
#include "util/function.h"
#include "util/geom.h"
#include "util/io.h"
#include <iostream>

using namespace std::chrono;

namespace {
steady_clock::time_point clock_begin = steady_clock::now();
steady_clock::time_point clock_end = steady_clock::now();

std::ostringstream oss;
}

namespace twop {
using util::io::log;

class Model : public model::DEMModel {

public:
  explicit Model(inp::Input *deck) : model::DEMModel(deck) {

    if (d_ppFile.is_open())
      d_ppFile.close();

    std::string filename =
        d_outputDeck_p->d_path + "pp_" + d_outputDeck_p->d_tagPPFile + ".csv";
    d_ppFile = std::ofstream(filename, std::ios_base::out);
    d_ppFile << "t, delta, cont_area_r, s_loc, s_val, max_dist, "
                "cont_area_r_ideal, s_loc_ideal, s_val_ideal" << std::flush;
  }

  void run(inp::Input *deck) override {
    log("Running twop app \n");

    init();
    integrate();
  }

  void integrate() override {
    // perform output at the beginning
    if (d_n == 0 && d_outputDeck_p->d_performOut) output();

    // apply initial condition
    if (d_n == 0) applyInitialCondition();

    // apply loading
    computeExternalDisplacementBC();
    computeExternalForces();

    for (size_t i = d_n; i < d_modelDeck_p->d_Nt; i++) {

      if (d_n % 100 == 0)
        log(fmt::format("time step: {} \n", i));

      // NOTE: If there is need for different time-stepping scheme, one can
      // define another function similar to these two below
      clock_begin = steady_clock::now();
      integrateStep();

      if (d_n % 100 == 0)
        log(fmt::format(
            "  Integration time: {} \n",
            util::methods::timeDiff(clock_begin, steady_clock::now())));

      if (d_pDeck_p->d_testName == "two_particle") {
        // NOTE: The purpose of this app 'twop' is to show that if we have
        // specific post-processing requirement from the outcome of
        // simulation -- eg in two-particle test, we may be interested in the
        // maximum of y-coordinate of top particle to measure the damping
        // effect or the maximum shear stress -- we wrap the base DEMModel
        // class as we did here and add a specific post-processing function
        // like 'ppTwoParticleTest()'.
        twoParticleTest();
      }

      // handle general output
      if ((d_n % d_outputDeck_p->d_dtOut == 0) &&
          (d_n >= d_outputDeck_p->d_dtOut) && d_outputDeck_p->d_performOut) {
        clock_begin = steady_clock::now();
        output();
        log(fmt::format(
            "   Output time: {} \n",
            util::methods::timeDiff(clock_begin, steady_clock::now())));
      }

      // check for stop (we may want to terminate the simulation early if the
      // results are garbage, or if some other criteria is met)
      // NOTE: this too can be specific to particular application
      checkStop();
    } // loop over time steps
  }

  // compute location of maximum shear stress and penetration length
  void twoParticleTest() {

    bool continue_dt = false;
    auto check_dt = d_outputDeck_p->d_dtTestOut;
    if ((d_n % check_dt == 0) && (d_n >= check_dt))
      continue_dt = true;

    if (!continue_dt)
      return;

    // get alias for particles
    const auto &p0 = this->d_particles[0];
    const auto &p1 = this->d_particles[1];

    // get penetration distance
    const auto &xc0 = p0->getCurrentCenter();
    const auto &xc1 = p1->getCurrentCenter();
    const double &r = p0->getGeomObjectP()->inscribedRadius();

    const auto &contact = d_cDeck_p->getContact(p0->d_zoneId, p1->d_zoneId);
    double r_e = r + contact.d_contactR;

    d_penDist = xc1.dist(xc0) - r_e - r;
    if (util::definitelyLessThan(d_penDist, 0.))
      d_contactAreaRadius =
          std::sqrt(std::pow(r_e, 2.) - std::pow(r_e + d_penDist, 2.));
    else if (util::definitelyGreaterThan(d_penDist, 0.)) {
      d_penDist = 0.;
      d_contactAreaRadius = 0.;
    }

    // get max distance of second particle (i.e. the y-coord of center + radius)
    double max_dist = xc1.d_y + p1->getGeomObject().inscribedRadius();

    // compute strain
    double max_stress_t = 0.;
    auto max_stress_loc_t = util::Point3();
    p1->getMaxShearStressAndLoc(5, max_stress_t, max_stress_loc_t);

    d_maxStress = max_stress_t;
    d_maxStressLoc = max_stress_loc_t.length();

    // compute ideal values
    static int contact_pp_ideal = -1;
    if (contact_pp_ideal == -1) {
      double mass = p1->getDensity() * M_PI * std::pow(r, 2.);

      const auto &mat_data =
          p1->getMaterial()->computeMaterialProperties(d_modelDeck_p->d_dim);

      d_contactAreaRadiusIdeal = 3. * mass * std::abs(d_pDeck_p->d_gravity[1]) *
          2. * r * (1. - std::pow(mat_data.d_nu, 2.)) /
          (4. * mat_data.d_E);
      d_contactAreaRadiusIdeal = std::pow(d_contactAreaRadiusIdeal, 1. / 3.);

      d_maxStressLocIdeal = r - 0.48 * d_contactAreaRadiusIdeal;

      d_maxStressIdeal =
          0.93 * mass * std::abs(d_pDeck_p->d_gravity[1]) /
              (2. * M_PI * d_contactAreaRadiusIdeal * d_contactAreaRadiusIdeal);

      contact_pp_ideal = 0;
    }

    d_ppFile << d_time << ", " << -d_penDist << ", " << d_contactAreaRadius
             << ", " << d_maxStressLoc << ", " << d_maxStress << ", "
             << max_dist << ", " << d_contactAreaRadiusIdeal << ", "
             << d_maxStressLocIdeal << ", " << d_maxStressIdeal << std::flush;

    // compute maximum y coordinate of particle 2
    double max_y_loc = p1->getCurrentNode(0).d_y;
    for (size_t i = 0; i < p1->getNumNodes(); i++)
      if (util::definitelyLessThan(max_y_loc, p1->getCurrentNode(i).d_y))
        max_y_loc = p1->getCurrentNode(i).d_y;

    if (util::definitelyLessThan(d_maxY, max_y_loc))
      d_maxY = max_y_loc;

    log(fmt::format("max y: {} \n", d_maxY));
  }

public:
  std::ofstream d_ppFile;

  double d_penDist = 0.;
  double d_contactAreaRadius = 0.;
  double d_maxStress = 0.;
  double d_maxStressLoc = 0.;

  double d_maxY = 0.;

  double d_contactAreaRadiusIdeal = 0.;
  double d_maxStressIdeal = 0.;
  double d_maxStressLocIdeal = 0.;
};
} // namespace twop

int main(int argc, char *argv[]) {

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h")) {
    // print help
    std::cout << "Syntax to run the app: ./twop -i <input file> -n <number of threads>";
    std::cout << "Example: ./twop -i input.yaml -n 4";
  }
  
  // print program version
  std::cout << "PeriDEM"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl;

  // current time
  auto begin = steady_clock::now();

  // read input data
  std::string filename = input.getCmdOption("-f");
  auto *deck = new inp::Input(filename);

  // check which model to run
  if (deck->isPeriDEM()) {
    twop::Model dem(deck);
    dem.run(deck);
  }

  // get time elapsed
  auto end = steady_clock::now();

  std::cout << "Total simulation time (s) = " 
            << util::methods::timeDiff(begin, end, "seconds") 
            << std::endl;
}