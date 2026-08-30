/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef TWOPARTICLE_CUSTOM_APPDAMPING_H
#define TWOPARTICLE_CUSTOM_APPDAMPING_H

#include "contact/damping.h"

namespace twoparticle_custom {

/*!
 * Library damping plus extra center-to-center viscous force.
 *
 * Lives in this app. Set on Contact with setDamping; do not copy the
 * neighbor loop in contact.cpp.
 */
class AppDamping : public contact::Damping {
public:
  void apply(data::ModelData &data) override;
};

} // namespace twoparticle_custom

#endif // TWOPARTICLE_CUSTOM_APPDAMPING_H
