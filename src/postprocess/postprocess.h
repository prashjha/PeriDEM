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

namespace data {
class ModelData;
}

namespace postprocess {

/*! @brief Extra postprocessing and stop criteria (VTU writing is in rw::). */
class Postprocess {
public:
  virtual ~Postprocess() = default;

  virtual void close(data::ModelData &data);
  virtual std::string twoParticle(data::ModelData &data);
  virtual std::string compressive(data::ModelData &data);
  virtual void checkStop(data::ModelData &data);
};

} // namespace postprocess

#endif // POSTPROCESS_POSTPROCESS_H
