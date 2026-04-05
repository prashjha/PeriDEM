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

} // namespace rw
