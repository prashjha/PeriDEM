/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef CONTACT_DAMPING_H
#define CONTACT_DAMPING_H

namespace data {
class ModelData;
}

namespace contact {

/*!
 * Center-center and particle-wall damping. Not part of the node-node pair
 * law. Contact applies this after pair assembly if d_damping is set.
 * Subclass to change or add damping; pass nullptr to skip.
 */
class Damping {
public:
  virtual ~Damping() = default;

  virtual void apply(data::ModelData &data);
};

} // namespace contact

#endif // CONTACT_DAMPING_H
