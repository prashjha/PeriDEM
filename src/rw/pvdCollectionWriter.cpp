/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "pvdCollectionWriter.h"
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace rw {

void writePvdCollectionFile(
    const std::string &pvd_path,
    const std::vector<std::pair<double, std::string>> &time_and_vtu_relative_path) {

  std::ofstream os(pvd_path);
  if (!os)
    throw std::runtime_error("writePvdCollectionFile: could not open " + pvd_path);

  os << std::setprecision(17);
  os << "<?xml version=\"1.0\"?>\n";
  os << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
  os << "  <Collection>\n";
  for (const auto &e : time_and_vtu_relative_path) {
    os << "    <DataSet timestep=\"" << e.first << "\" file=\"" << e.second << "\"/>\n";
  }
  os << "  </Collection>\n";
  os << "</VTKFile>\n";
}

void writePvtuCollectionFile(
    const std::string &pvtu_path,
    const std::vector<std::string> &piece_vtu_relative_paths) {

  std::ofstream os(pvtu_path);
  if (!os)
    throw std::runtime_error("writePvtuCollectionFile: could not open " +
                             pvtu_path);

  os << "<?xml version=\"1.0\"?>\n";
  os << "<VTKFile type=\"PUnstructuredGrid\" version=\"0.1\" "
        "byte_order=\"LittleEndian\">\n";
  os << "  <PUnstructuredGrid GhostLevel=\"0\">\n";
  // PointData / CellData omitted: ParaView reads arrays from pieces.
  os << "    <PPoints>\n";
  os << "      <PDataArray type=\"Float32\" NumberOfComponents=\"3\"/>\n";
  os << "    </PPoints>\n";
  for (const auto &piece : piece_vtu_relative_paths) {
    os << "    <Piece Source=\"" << piece << "\"/>\n";
  }
  os << "  </PUnstructuredGrid>\n";
  os << "</VTKFile>\n";
}

} // namespace rw
