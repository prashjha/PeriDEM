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

/*!
 * @brief Write a ParaView parallel VTU (.pvtu) that lists per-rank .vtu pieces.
 *
 * Each string in @p piece_vtu_relative_paths is a path relative to the .pvtu
 * (typically `output_N_r0.vtu`, …). Open the .pvtu (or a .pvd that points at
 * .pvtu files) in ParaView for the full mesh across ranks.
 */
void writePvtuCollectionFile(
    const std::string &pvtu_path,
    const std::vector<std::string> &piece_vtu_relative_paths);

} // namespace rw

#endif
