/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef PD_PDFORCE_H
#define PD_PDFORCE_H

namespace data {
class ModelData;
}

namespace pd {

/*! @brief Assemble nodal peridynamic force (constitutive response stays in material). */
void computeForces(data::ModelData &data);

} // namespace pd

#endif // PD_PDFORCE_H
