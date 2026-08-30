/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef POSTPROCESS_POSTPROCESS_H
#define POSTPROCESS_POSTPROCESS_H

#include <memory>
#include <string>

namespace model {
class ModelData;
}

namespace postprocess {

/*!
 * @brief Extra postprocessing and stop criteria (VTU writing is in rw::).
 *
 * DEMModel holds a Postprocess object. An app can supply another
 * implementation without editing src/.
 */
class Postprocess {
public:
  virtual ~Postprocess() = default;

  virtual void close(model::ModelData &data);
  virtual std::string twoParticle(model::ModelData &data);
  virtual std::string compressive(model::ModelData &data);
  virtual void checkStop(model::ModelData &data);
};

} // namespace postprocess

#endif // POSTPROCESS_POSTPROCESS_H
