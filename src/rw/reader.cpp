/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "reader.h"
#include "mshReader.h"
#include "vtkReader.h"
#include <csv/csv.h>

void rw::reader::readCsvFile(const std::string &filename, size_t dim,
                             std::vector<util::Point> *nodes,
                             std::vector<double> *volumes) {
  nodes->clear();
  volumes->clear();
  if (dim == 1) {
    io::CSVReader<3> in(filename);
    in.read_header(io::ignore_extra_column, "id", "x", "volume");

    double x;
    double volume;
    int id;
    while (in.read_row(id, x, volume)) {
      volumes->emplace_back(volume);
      nodes->emplace_back(util::Point(x, 0., 0.));
    }
  }

  if (dim == 2) {
    io::CSVReader<4> in(filename);
    in.read_header(io::ignore_extra_column, "id", "x", "y", "volume");

    double x, y, volume;
    int id;
    while (in.read_row(id, x, y, volume)) {
      volumes->emplace_back(volume);
      nodes->emplace_back(util::Point(x, y, 0.));
    }
  }

  if (dim == 3) {
    io::CSVReader<5> in(filename);
    in.read_header(io::ignore_extra_column, "id", "x", "y", "z", "volume");

    double x, y, z, volume;
    int id;
    while (in.read_row(id, x, y, z, volume)) {
      volumes->emplace_back(volume);
      nodes->emplace_back(util::Point(x, y, z));
    }
  }
}

void rw::reader::readParticleCsvFile(const std::string &filename, size_t dim,
                                     std::vector<util::Point> *nodes,
                                     std::vector<double> *rads,
                                     std::vector<size_t> *zones) {

  nodes->clear();
  zones->clear();
  rads->clear();

  io::CSVReader<5> in(filename);
  in.read_header(io::ignore_extra_column, "i", "x", "y", "z", "r");

  double x, y, z, r;
  int id;
  while (in.read_row(id, x, y, z, r)) {
    rads->emplace_back(r);
    nodes->emplace_back(util::Point(x, y, z));
    zones->emplace_back(id);
  }
}

void rw::reader::readParticleCsvFile(const std::string &filename, size_t dim,
                                     std::vector<util::Point> *nodes,
                                     std::vector<double> *rads,
                                     const size_t &zone) {

  nodes->clear();
  rads->clear();

  io::CSVReader<5> in(filename);
  in.read_header(io::ignore_extra_column, "i", "x", "y", "z", "r");

  double x, y, z, r;
  int id;
  while (in.read_row(id, x, y, z, r)) {
    if (id == zone) {
      rads->emplace_back(r);
      nodes->emplace_back(util::Point(x, y, z));
    }
  }
}

void rw::reader::readParticleWithOrientCsvFile(const std::string &filename, size_t dim,
                                     std::vector<util::Point> *nodes,
                                     std::vector<double> *rads,
                                     std::vector<double> *orients,
                                     const size_t &zone) {

  nodes->clear();
  rads->clear();
  orients->clear();

  io::CSVReader<6> in(filename);
  in.read_header(io::ignore_extra_column, "i", "x", "y", "z", "r", "o");

  double x, y, z, r, o;
  int id;
  while (in.read_row(id, x, y, z, r, o)) {
    if (id == zone) {
      rads->emplace_back(r);
      nodes->emplace_back(util::Point(x, y, z));
      orients->emplace_back(o);
    }
  }
}

void rw::reader::readVtuFile(const std::string &filename, size_t dim,
                             std::vector<util::Point> *nodes,
                             size_t &element_type, size_t &num_elem,
                             std::vector<size_t> *enc,
                             std::vector<std::vector<size_t>> *nec,
                             std::vector<double> *volumes, bool is_fd) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  rdr.readMesh(dim, nodes, element_type, num_elem, enc, nec, volumes, is_fd);
  rdr.close();
}

void rw::reader::readVtuFileNodes(const std::string &filename, size_t dim,
                                  std::vector<util::Point> *nodes,
                                  bool ref_config) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);

  // below will read the current position of nodes
  rdr.readNodes(nodes);

  // need to subtract the displacement to get reference configuration of nodes
  if (ref_config) {
    std::vector<util::Point> u;
    if (rdr.readPointData("Displacement", &u)) {
      std::cerr << "Error: Did not find displacement in the vtu file."
                << std::endl;
      exit(1);
    }

    if (u.size() != nodes->size()) {
      std::cerr << "Error: Displacement data and node data size do not match."
                << std::endl;
      exit(1);
    }

    for (size_t i = 0; i < u.size(); i++)
      (*nodes)[i] -= u[i];
  }

  rdr.close();
}

void rw::reader::readVtuFileCells(const std::string &filename, size_t dim,
                                  size_t &element_type, size_t &num_elem,
                                  std::vector<size_t> *enc,
                                  std::vector<std::vector<size_t>> *nec) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);

  // below will read the current position of nodes
  rdr.readCells(dim, element_type, num_elem, enc, nec);

  rdr.close();
}

void rw::reader::readVtuFileRestart(const std::string &filename,
                                    std::vector<util::Point> *u,
                                    std::vector<util::Point> *v,
                                    const std::vector<util::Point> *X) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // if displacement is not in input file, use reference coordinate to get
  // displacement
  if (!rdr.readPointData("Displacement", u)) {
    std::vector<util::Point> y;
    rdr.readNodes(&y);
    if (y.size() != X->size()) {
      std::cerr << "Error: Number of nodes in input file = " << filename
                << " and number nodes in data X are not same.\n";
      exit(1);
    }

    u->resize(y.size());
    for (size_t i = 0; i < y.size(); i++)
      (*u)[i] = y[i] - (*X)[i];
  }

  // get velocity
  rdr.readPointData("Velocity", v);
  rdr.close();
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<uint8_t> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<size_t> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<int> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<float> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<double> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<util::Point> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<util::SymMatrix3> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<util::Matrix3> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFileCellData(const std::string &filename,
                                     const std::string &tag,
                                     std::vector<float> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readCellData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFileCellData(const std::string &filename,
                                     const std::string &tag,
                                     std::vector<double> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readCellData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFileCellData(const std::string &filename,
                                     const std::string &tag,
                                     std::vector<util::Point> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readCellData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFileCellData(const std::string &filename,
                                     const std::string &tag,
                                     std::vector<util::SymMatrix3> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readCellData(tag, data);
  rdr.close();
  return st;
}

bool rw::reader::readVtuFileCellData(const std::string &filename,
                                     const std::string &tag,
                                     std::vector<util::Matrix3> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // read data
  auto st = rdr.readCellData(tag, data);
  rdr.close();
  return st;
}

void rw::reader::readMshFile(const std::string &filename, size_t dim,
                             std::vector<util::Point> *nodes,
                             size_t &element_type, size_t &num_elem,
                             std::vector<size_t> *enc,
                             std::vector<std::vector<size_t>> *nec,
                             std::vector<double> *volumes, bool is_fd) {
  // call vtk reader
  auto rdr = rw::reader::MshReader(filename);
  rdr.readMesh(dim, nodes, element_type, num_elem, enc, nec, volumes, is_fd);
  rdr.close();
}

void rw::reader::readMshFileRestart(const std::string &filename,
                                    std::vector<util::Point> *u,
                                    std::vector<util::Point> *v,
                                    const std::vector<util::Point> *X) {
  // call vtk reader
  auto rdr = rw::reader::MshReader(filename);
  // if displacement is not in input file, use reference coordinate to get
  // displacement
  if (!rdr.readPointData("Displacement", u)) {
    std::vector<util::Point> y;
    rdr.readNodes(&y);
    if (y.size() != X->size()) {
      std::cerr << "Error: Number of nodes in input file = " << filename
                << " and number nodes in data X are not same.\n";
      exit(1);
    }

    u->resize(y.size());
    for (size_t i = 0; i < y.size(); i++)
      (*u)[i] = y[i] - (*X)[i];
  }

  // get velocity
  rdr.readPointData("Velocity", v);
  rdr.close();
}

bool rw::reader::readMshFilePointData(const std::string &filename,
                                      const std::string &tag,
                                      std::vector<double> *data) {
  // call vtk reader
  auto rdr = rw::reader::VtkReader(filename);
  // get velocity
  auto st = rdr.readPointData(tag, data);
  rdr.close();
  return st;
}
