/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "input.h"
#include "geom/geomIncludes.h"
#include <iostream>
#include <cmath>

inp::Input::Input(const json &j) {
  d_modelDeck_p = j.find("Model") != j.end() ? std::make_unique<ModelDeck>(j["Model"]) : std::make_shared<ModelDeck>();
  d_outputDeck_p = j.find("Output") != j.end() ? std::make_unique<OutputDeck>(j["Output"]) : std::make_shared<OutputDeck>();
  d_restartDeck_p = j.find("Restart") != j.end() ? std::make_unique<RestartDeck>(j["Restart"]) : std::make_shared<RestartDeck>();
  d_testDeck_p =  j.find("Test") != j.end() ? std::make_unique<TestDeck>(j["Test"]) : std::make_shared<TestDeck>();
  d_bcDeck_p = std::make_shared<inp::BCDeck>(j);

  // particle deck
  d_particleDeck_p = std::make_shared<inp::ParticleDeck>(j, d_modelDeck_p->d_particleSimType);
};
