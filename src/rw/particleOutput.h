/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef RW_PARTICLE_OUTPUT_H
#define RW_PARTICLE_OUTPUT_H

namespace data {
class ModelData;
}

namespace rw {

/*! @brief Write VTU/PVD (and optional strain VTU / particle-location CSV). */
void writeOutput(data::ModelData &data);

} // namespace rw

#endif // RW_PARTICLE_OUTPUT_H
