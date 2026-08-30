/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef CONTACT_CONTACT_H
#define CONTACT_CONTACT_H

#include <memory>

namespace model {
class ModelData;
}

namespace contact {

/*!
 * @brief Contact force and contact neighbor search.
 *
 * DEMModel holds a Contact object. An app can supply another implementation
 * (subclass or replacement) without editing src/.
 */
class Contact {
public:
  virtual ~Contact() = default;

  virtual void setup(model::ModelData &data);
  virtual bool updateSearchParameters(model::ModelData &data);
  virtual void updateNeighborlist(model::ModelData &data);
  virtual void computeForces(model::ModelData &data);
};

} // namespace contact

#endif // CONTACT_CONTACT_H
