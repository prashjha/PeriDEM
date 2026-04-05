/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef RW_PVD_COLLECTION_WRITER_H
#define RW_PVD_COLLECTION_WRITER_H

#include <string>
#include <utility>
#include <vector>

namespace rw {

/*!
 * @brief Write a ParaView VTK collection (.pvd) that lists VTU files with timesteps.
 *
 * Open the .pvd in ParaView to animate all listed .vtu snapshots. Each entry's
 * second string must be the VTU filename only (or a path relative to the .pvd file).
 */
void writePvdCollectionFile(
    const std::string &pvd_path,
    const std::vector<std::pair<double, std::string>> &time_and_vtu_relative_path);

} // namespace rw

#endif
