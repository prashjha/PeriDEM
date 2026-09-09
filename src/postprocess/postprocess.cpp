/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "postprocess.h"

#include "data/modelData.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "particle/baseParticle.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "inp/input.h"

#include <cstdlib>
#include <format>
#include <mpi.h>

void postprocess::Postprocess::close(data::ModelData &data) {

  if (data.d_ppFile.is_open())
    data.d_ppFile.close();

}

std::string postprocess::Postprocess::twoParticle(data::ModelData &data) {


  bool continue_dt = false;
  auto check_dt = data.d_outputDeck_p->d_dtTestOut;
  if ((data.d_n % check_dt == 0) && (data.d_n >= check_dt))
    continue_dt = true;

  if (!continue_dt)
    return "";

  // get alias for particles
  const auto &p0 = data.d_particlesListTypeAll[0];
  const auto &p1 = data.d_particlesListTypeAll[1];

  // get penetration distance
  const auto &xc0 = p0->getXCenter();
  const auto &xc1 = p1->getXCenter();
  const double &r = p0->d_geom_p->boundingRadius();

  const auto &contact = data.d_particleDeck_p->d_contactDeck.getContact(p0->getGroupId("contact_id"), p1->getGroupId("contact_id"));
  double r_e = r + contact.d_contactR;

  double pen_dist = xc1.dist(xc0) - r_e - r;
  double contact_area_radius = 0.;
  if (util::isLess(pen_dist, 0.))
    contact_area_radius =
        std::sqrt(std::pow(r_e, 2.) - std::pow(r_e + pen_dist, 2.));
  else if (util::isGreater(pen_dist, 0.)) {
    pen_dist = 0.;
    contact_area_radius = 0.;
  }

  // get max distance of second particle (i.e. the y-coord of center + radius)
  double max_dist = xc1.d_y + p1->d_geom_p->boundingRadius();

  // compute maximum y coordinate of particle 2
  double max_y_loc = p1->getXLocal(0).d_y;
  double max_y = 0.;
  for (size_t i = 0; i < p1->getNumNodes(); i++)
    if (util::isLess(max_y_loc, p1->getXLocal(i).d_y))
      max_y_loc = p1->getXLocal(i).d_y;

  if (util::isLess(max_y, max_y_loc))
    max_y = max_y_loc;

  data.setKeyData("pen_dist", pen_dist);
  data.setKeyData("contact_area_radius", contact_area_radius);
  data.setKeyData("max_y", max_y);
  data.setKeyData("max_dist", max_dist);
  data.setKeyData("max_y_loc", max_y_loc);


  return std::format("  Post-processing: max y = {:.6f} \n", max_y);

}

std::string postprocess::Postprocess::compressive(data::ModelData &data) {

  bool continue_dt = false;
  auto check_dt = data.d_outputDeck_p->d_dtTestOut;
  if ((data.d_n % check_dt == 0) && (data.d_n >= check_dt))
    continue_dt = true;

  if (!continue_dt)
    return "";

  // get wall
  auto w_id = data.d_testDeck_p->d_particleIdCompressiveTest;
  auto f_dir = data.d_testDeck_p->d_particleForceDirectionCompressiveTest - 1;
  const auto &wall = data.d_particlesListTypeAll[w_id];

  // find the penetration of the wall from it's original location
  auto dx = wall->getXLocal(0) - wall->getXRefLocal(0);
  double wall_penetration = dx[f_dir];

  // Local wall reaction; under particle-MPI each rank only sees contact from
  // owned grains, so sum across ranks before recording.
  double tot_reaction_force = 0.;
  for (size_t i = 0; i < wall->getNumNodes(); i++) {
    tot_reaction_force += wall->getFLocal(i)[f_dir] * wall->getVolLocal(i);
  }
  if (util::parallel::mpiSize() > 1) {
    double reduced = 0.;
    MPI_Allreduce(&tot_reaction_force, &reduced, 1, MPI_DOUBLE, MPI_SUM,
                  util::parallel::mpiComm());
    tot_reaction_force = reduced;
  }

  // Rank 0 only: avoid interleaved multi-rank appends to the same CSV.
  if (util::parallel::mpiRank() == 0) {
    if (!data.d_ppFile.is_open()) {
      std::string tag_pp_file = data.d_outputDeck_p->d_tagPPFile.empty()
                                    ? "0"
                                    : data.d_outputDeck_p->d_tagPPFile;
      std::string filename = data.d_outputDeck_p->d_path + "pp_" +
                             data.d_testDeck_p->d_testName + "_" +
                             tag_pp_file + ".csv";
      data.d_ppFile.open(filename.c_str(), std::ofstream::out | std::ofstream::app);
      data.d_ppFile << "t, delta, force \n";
    }
    data.d_ppFile << std::format("{:.6e}, {:.6e}, {:.6e}\n", data.d_time,
                                 wall_penetration, tot_reaction_force);
  }

  data.setKeyData("wall_penetration", wall_penetration);
  data.setKeyData("tot_reaction_force", tot_reaction_force);

  return std::format("  Post-processing: wall penetration = {:"
                     ".6f}, "
                     "reaction force = {:5.3e} \n",
                     wall_penetration, tot_reaction_force);

}

void postprocess::Postprocess::checkStop(data::ModelData &data) {


  if (data.d_outputDeck_p->d_outCriteria == "max_particle_dist" &&
      data.d_testDeck_p->d_testName == "two_particle") {

    // compute max distance between two particles
    // current center position
    const auto &xci = data.d_particlesListTypeAll[0]->getXCenter();
    const auto &xcj = data.d_particlesListTypeAll[1]->getXCenter();

    // check
    if (util::isGreater(xci.dist(xcj),
                        data.d_outputDeck_p->d_outCriteriaParams[0])) {

      if(data.d_ppFile.is_open())
        data.d_ppFile.close();
      exit(1);
    }
  }
  else if (data.d_outputDeck_p->d_outCriteria == "max_node_dist") {

    //    static int msg_printed = 0;
    //    if (msg_printed == 0) {
    //      std::cout << "Check = " << data.d_outputDeck_p->d_outCriteria
    //              << " is no longer supported. In future, this test will be implemented when function util::methods::maxLength() is defined." << std::endl;
    //      msg_printed = 1;
    //    }
    //exit(EXIT_FAILURE);
    auto max_pt_and_index = util::methods::maxLengthAndMaxLengthIndex(data.d_x);
    auto max_x = data.d_x[max_pt_and_index.second];

    // check
    if (util::isGreater(max_pt_and_index.first,
                        data.d_outputDeck_p->d_outCriteriaParams[0])) {

      // close open file
      if(data.d_ppFile.is_open())
        data.d_ppFile.close();

      util::io::log(0, std::format("{}: Terminating simulation as one of the failing"
                      " criteria is met. Point ({:.6f}, {:.6f}, {:.6f}) is at "
                      "distance {:.6f} "
                      "more than"
                      " allowed distance {:.6f}\n",
                      data.d_name, max_x.d_x, max_x.d_y, max_x.d_z, max_x.length(),
                      data.d_outputDeck_p->d_outCriteriaParams[0]));
      exit(1);
    }
  }

}
