/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include <PeriDEMConfig.h>

#include "model/dem/demModel.h"
#include "material/materialUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/geomObjects.h"
#include "util/matrix.h"
#include "util/methods.h"
#include "util/point.h"
#include "inp/pdecks/contactDeck.h"
#include "rw/reader.h"
#include "util/function.h"
#include "util/geom.h"
#include "util/methods.h"
#include "util/randomDist.h"
#include "util/parallelUtil.h"
#include "inp/decks/materialDeck.h"
#include "inp/decks/modelDeck.h"
#include "inp/decks/outputDeck.h"
#include "inp/decks/restartDeck.h"
#include "rw/vtkParticleWriter.h"
#include "rw/vtkParticleReader.h"
#include "fe/elemIncludes.h"
#include "fe/meshUtil.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>

#include <taskflow/taskflow/taskflow.hpp>
#include <taskflow/taskflow/algorithm/for_each.hpp>

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

    d_quadOrder = deck->getModelDeck()->d_quadOrder;

    if (d_ppFile.is_open())
      d_ppFile.close();

    std::string filename =
        d_outputDeck_p->d_path + "pp_" + d_outputDeck_p->d_tagPPFile + ".csv";
    d_ppFile = std::ofstream(filename, std::ios_base::out);
    d_ppFile << "t, delta, cont_area_r, s_loc, s_val, max_dist, "
                "cont_area_r_ideal, s_loc_ideal, s_val_ideal" << std::flush;
  }

  void run(inp::Input *deck) override {

    log("twop::Model: Running twop app \n");

    init();

    // check if init() successfully created quadrature data which we need for postprocessing
    {
      size_t totalQuadPoints = 0;
      for (auto &p: d_particlesListTypeAll) {
        const auto &particle_mesh_p = p->d_rp_p->getMeshP();

        // get Quadrature
        fe::BaseElem *elem;
        if (particle_mesh_p->getElementType() == util::vtk_type_line)
          elem = new fe::LineElem(d_modelDeck_p->d_quadOrder);
        else if (particle_mesh_p->getElementType() == util::vtk_type_triangle)
          elem = new fe::TriElem(d_modelDeck_p->d_quadOrder);
        else if (particle_mesh_p->getElementType() == util::vtk_type_quad)
          elem = new fe::QuadElem(d_modelDeck_p->d_quadOrder);
        else if (particle_mesh_p->getElementType() == util::vtk_type_tetra)
          elem = new fe::TetElem(d_modelDeck_p->d_quadOrder);
        else {
          std::cerr << fmt::format("Error: Can not compute strain/stress as the element "
                                   "type = {} is not yet supported in this routine.\n", particle_mesh_p->getElementType());
          exit(EXIT_FAILURE);
        }

        totalQuadPoints += particle_mesh_p->getNumElements() *
                           elem->getNumQuadPoints();
      }

      if (d_xQuadCur.size() != totalQuadPoints
            or d_strain.size() != totalQuadPoints
            or d_stress.size() != totalQuadPoints) {
        std::cerr << "Error: DEMModel::init() did not initialize quadrature data.\n";
        exit(EXIT_FAILURE);
      }
    }

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
        log(fmt::format("twop::Model: time step: {} \n", i));

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

    // compute QoIs
    twoParticleTestPenetrationDist();
    twoParticleTestMaxShearStress();

    // log
    d_ppFile << d_time << ", " << -d_penDist << ", " << d_contactAreaRadius
             << ", " << d_maxStressLocRef << ", " << d_maxStress << ", "
             << d_maxDist << ", " << d_contactAreaRadiusIdeal << ", "
             << d_maxStressLocRefIdeal << ", " << d_maxStressIdeal << std::flush;
  }

  void twoParticleTestPenetrationDist() {

    // get alias for particles
    const auto &p0 = this->d_particles[0];
    const auto &p1 = this->d_particles[1];

    // get penetration distance
    const auto &xc0 = p0->getXCenter();
    const auto &xc1 = p1->getXCenter();
    const double &r = p0->d_geom_p->boundingRadius();

    const auto &contact = d_cDeck_p->getContact(p0->d_zoneId, p1->d_zoneId);
    double r_e = r + contact.d_contactR;

    d_penDist = xc1.dist(xc0) - r_e - r;
    if (util::isLess(d_penDist, 0.))
      d_contactAreaRadius =
              std::sqrt(std::pow(r_e, 2.) - std::pow(r_e + d_penDist, 2.));
    else if (util::isGreater(d_penDist, 0.)) {
      d_penDist = 0.;
      d_contactAreaRadius = 0.;
    }

    // get max distance of second particle (i.e. the y-coord of center + radius)
    d_maxDist = xc1.d_y + p1->d_geom_p->boundingRadius();

    // compute maximum y coordinate of particle 2
    double max_y_loc = p1->getXLocal(0).d_y;
    for (size_t i = 0; i < p1->getNumNodes(); i++)
      if (util::isLess(max_y_loc, p1->getXLocal(i).d_y))
        max_y_loc = p1->getXLocal(i).d_y;

    if (util::isLess(d_maxY, max_y_loc))
      d_maxY = max_y_loc;

    log(fmt::format("max y: {} \n", d_maxY));

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

      d_maxStressLocRefIdeal = r - 0.48 * d_contactAreaRadiusIdeal;

      d_maxStressIdeal =
              0.93 * mass * std::abs(d_pDeck_p->d_gravity[1]) /
              (2. * M_PI * d_contactAreaRadiusIdeal * d_contactAreaRadiusIdeal);

      contact_pp_ideal = 0;
    }
  }

  void twoParticleTestMaxShearStress() {

    // compute maximum shear stress and where it occurs
    double max_stress_t = 0.;
    auto max_stress_loc_cur_t = util::Point();
    auto max_stress_loc_ref_t = util::Point();

    // compute stress and strain
    for (auto &p: d_particles) {
      auto p_mat_data = p->getMaterial()->computeMaterialProperties(
              p->d_rp_p->getMeshP()->getDimension());

      fe::getCurrentQuadPoints(p->d_rp_p->getMeshP(), d_xRef, d_u, d_xQuadCur,
                               p->d_globStart, p->d_globQuadStart,
                               d_quadOrder);
      fe::getStrainStress(p->d_rp_p->getMeshP(), d_xRef, d_u,
                          d_input_p->getMaterialDeck()->d_isPlaneStrain,
                          d_strain, d_stress,
                          p->d_globStart, p->d_globQuadStart,
                          p_mat_data.d_nu, p_mat_data.d_lambda,
                          p_mat_data.d_mu,
                          true, d_quadOrder);

      double p_max_stress = 0.;
      auto p_max_stress_loc_cur = util::Point();
      auto p_max_stress_loc_ref = util::Point();
      fe::getMaxShearStressAndLoc(p->d_rp_p->getMeshP(), d_xRef, d_u, d_stress,
                                  p_max_stress,
                                  p_max_stress_loc_ref,
                                  p_max_stress_loc_cur,
                                  p->d_globStart, p->d_globQuadStart, d_quadOrder);

      if (util::isGreater(p_max_stress, max_stress_t)) {
        max_stress_t = p_max_stress;
        auto p_center_node_id = p->d_globStart + p->d_rp_p->getCenterNodeId();
        max_stress_loc_ref_t = p_max_stress_loc_ref - d_xRef[p_center_node_id];
        max_stress_loc_cur_t = p_max_stress_loc_cur - d_x[p_center_node_id];
      }
    }

    d_maxStress = max_stress_t;
    d_maxStressLocRef = max_stress_loc_ref_t.length();
    d_maxStressLocCur = max_stress_loc_cur_t.length();
  }

public:
  std::ofstream d_ppFile;

  double d_penDist = 0.;
  double d_contactAreaRadius = 0.;
  double d_maxDist = 0.;
  double d_maxStress = 0.;
  double d_maxStressLocRef = 0.;
  double d_maxStressLocCur = 0.;

  double d_maxY = 0.;

  double d_contactAreaRadiusIdeal = 0.;
  double d_maxStressIdeal = 0.;
  double d_maxStressLocRefIdeal = 0.;

  size_t d_quadOrder = 1;
};
} // namespace twop

int main(int argc, char *argv[]) {

  // print program version
  std::cout << "twop (PeriDEM)"
            << " (Version " << MAJOR_VERSION << "." << MINOR_VERSION << "."
            << UPDATE_VERSION << ")" << std::endl << std::flush;

  util::io::InputParser input(argc, argv);

  if (input.cmdOptionExists("-h")) {
    // print help
    std::cout << "Syntax to run the app: ./twop -i <input file> -nThreads <number of threads>";
    std::cout << "Example: ./twop -i input.yaml -nThreads 4";
  }

  // read input arguments
  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads")) nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = 2;
    util::io::print(fmt::format("Running twop with number of threads = {}\n", nThreads));
  }
  // set number of threads
  util::parallel::initNThreads(nThreads);
  util::io::print(fmt::format("Number of threads = {}\n", util::parallel::getNThreads()));

  std::string filename;
  if (input.cmdOptionExists("-i"))
    filename = input.getCmdOption("-i");
  else {
    filename = "./example/input_0.yaml";
    util::io::print(fmt::format("Running twop with example input file = {}\n", filename));
  }

  // current time
  auto begin = steady_clock::now();

  // create deck
  auto *deck = new inp::Input(filename);

  // check which model to run
  if (deck->isPeriDEM()) {
    // ensure two variables in the deck are set
    deck->getModelDeck()->d_populateElementNodeConnectivity = true;

    // simulate model
    twop::Model dem(deck);
    dem.run(deck);
  }

  // get time elapsed
  auto end = steady_clock::now();

  std::cout << "Total simulation time (s) = " 
            << util::methods::timeDiff(begin, end, "seconds") 
            << std::endl;
}