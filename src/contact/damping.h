/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
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
 *
 * Selected by Contact.Damping_Law:
 * - com_and_node (default): this COM path + node damping in the pair law
 * - com: this COM path only
 * - node: node damping only (no COM object)
 * - off: neither
 */
class Damping {
public:
  virtual ~Damping() = default;

  virtual void apply(data::ModelData &data);
};

} // namespace contact

#endif // CONTACT_DAMPING_H
