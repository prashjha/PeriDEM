/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "model.h"

model::Model::Model()
    : d_n(0), d_time(0.), d_te(0.), d_tw(0.), d_tk(0.), d_teF(0.), d_teFB(0.) {}

size_t model::Model::currentStep() { return d_n; }

float model::Model::getEnergy() { return d_te - d_tw + d_tk; }
