/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PERIDEM_MODEL_H
#define PERIDEM_MODEL_H

#include "data/modelData.h"
#include "contact/contact.h"
#include "postprocess/postprocess.h"
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/**
 * \defgroup Explicit Explicit
 */
/**@{*/

/*!
 * Default driver. Hands the run to time_int::Integrator at integrate().
 * Physics lives here: forces, BCs, output, particles.
 */
class PeriDEMModel : public data::ModelData {

public:
  explicit PeriDEMModel(std::shared_ptr<inp::Input> & deck, std::string modelName = "PeriDEMModel");

  void log(std::ostringstream &oss, int priority = 0, bool check_condition = true, int override_priority = -1, bool screen_out = false);

  void log(const std::string &str, int priority = 0, bool check_condition = true, int override_priority = -1, bool screen_out = false);

  void run(std::shared_ptr<inp::Input> & deck);

  void restart(std::shared_ptr<inp::Input> & deck);

  void init();

  void close();

  void integrate();

  void integrateStep();

  void computeForces();

  void computeExternalForces();

  void applyDisplacementBC();

  void applyInitialCondition();

  void output();

  std::string ppTwoParticleTest();

  std::string ppCompressiveTest();

  void checkStop();

  void setContact(std::unique_ptr<contact::Contact> c) { d_contact_p = std::move(c); }

  void setPostprocess(std::unique_ptr<postprocess::Postprocess> p) {
    d_postprocess_p = std::move(p);
  }

  std::unique_ptr<contact::Contact> d_contact_p;
  std::unique_ptr<postprocess::Postprocess> d_postprocess_p;
};

/** @}*/

#endif // PERIDEM_MODEL_H
