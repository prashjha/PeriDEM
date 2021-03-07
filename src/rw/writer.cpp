/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "writer.h"
#include "vtkWriter.h"
#include "legacyVtkWriter.h"
#include "mshWriter.h"

rw::writer::Writer::Writer()
    : d_vtkWriter_p(nullptr), d_legacyVtkWriter_p(nullptr),
      d_mshWriter_p(nullptr), d_format("vtu") {}

rw::writer::Writer::Writer(const std::string &filename,
                           const std::string &format,
                           const std::string &compress_type)
    : d_vtkWriter_p(nullptr), d_legacyVtkWriter_p(nullptr),
      d_mshWriter_p(nullptr), d_format("vtu") {
  open(filename, format, compress_type);
}

void rw::writer::Writer::open(const std::string &filename,
                              const std::string &format,
                              const std::string &compress_type) {
  d_format = format;
  if (d_format == "vtu")
    d_vtkWriter_p = new rw::writer::VtkWriter(filename, compress_type);
  else if (d_format == "msh")
    d_mshWriter_p = new rw::writer::MshWriter(filename, compress_type);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p =
        new rw::writer::LegacyVtkWriter(filename, compress_type);
}

rw::writer::Writer::~Writer() {
  delete (d_vtkWriter_p);
}

void rw::writer::Writer::appendNodes(
    const std::vector<util::Point> *nodes,
    const std::vector<util::Point> *u) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendNodes(nodes, u);
  else if (d_format == "msh")
    d_mshWriter_p->appendNodes(nodes, u);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendNodes(nodes, u);
}

void rw::writer::Writer::appendMesh(
    const std::vector<util::Point> *nodes, const size_t &element_type,
    const std::vector<size_t> *en_con, const std::vector<util::Point> *u) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendMesh(nodes, element_type, en_con, u);
  else if (d_format == "msh")
    d_mshWriter_p->appendMesh(nodes, element_type, en_con, u);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendMesh(nodes, element_type, en_con, u);
}

void rw::writer::Writer::appendPointData(
    const std::string &name, const std::vector<uint8_t> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendPointData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendPointData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendPointData(name, data);
}

void rw::writer::Writer::appendPointData(
    const std::string &name, const std::vector<size_t> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendPointData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendPointData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendPointData(name, data);
}

void rw::writer::Writer::appendPointData(
    const std::string &name, const std::vector<int> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendPointData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendPointData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendPointData(name, data);
}

void rw::writer::Writer::appendPointData(
    const std::string &name, const std::vector<float> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendPointData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendPointData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendPointData(name, data);
}

void rw::writer::Writer::appendPointData(
    const std::string &name, const std::vector<double> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendPointData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendPointData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendPointData(name, data);
}

void rw::writer::Writer::appendPointData(
    const std::string &name, const std::vector<util::Point> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendPointData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendPointData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendPointData(name, data);
}

void rw::writer::Writer::appendPointData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendPointData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendPointData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendPointData(name, data);
}

void rw::writer::Writer::appendCellData(
    const std::string &name, const std::vector<float> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendCellData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendCellData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendCellData(name, data);
}

void rw::writer::Writer::appendCellData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendCellData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendCellData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendCellData(name, data);
}

void rw::writer::Writer::addTimeStep(const double &timestep) {

  if (d_format == "vtu")
    d_vtkWriter_p->addTimeStep(timestep);
  else if (d_format == "msh")
    d_mshWriter_p->addTimeStep(timestep);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->addTimeStep(timestep);
}

void rw::writer::Writer::appendFieldData(const std::string &name,
                                         const double &data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendFieldData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendFieldData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendFieldData(name, data);
}

void rw::writer::Writer::appendFieldData(const std::string &name,
                                         const float &data) {

  if (d_format == "vtu")
    d_vtkWriter_p->appendFieldData(name, data);
  else if (d_format == "msh")
    d_mshWriter_p->appendFieldData(name, data);
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->appendFieldData(name, data);
}

void rw::writer::Writer::close() {

  if (d_format == "vtu")
    d_vtkWriter_p->close();
  else if (d_format == "msh")
    d_mshWriter_p->close();
  else if (d_format == "legacy_vtk")
    d_legacyVtkWriter_p->close();
}
