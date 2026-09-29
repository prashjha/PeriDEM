/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef NSEARCH_NEIGHBOR_POLICY_H
#define NSEARCH_NEIGHBOR_POLICY_H

namespace data {
class ModelData;
}

namespace nsearch {

/*! @brief Horizon search restricted to the same particle. */
void updatePeridynamicNeighborlist(data::ModelData &data);

} // namespace nsearch

#endif // NSEARCH_NEIGHBOR_POLICY_H
