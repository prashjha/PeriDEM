/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "vtkWriter.h"
#include <util/feElementDefs.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnsignedIntArray.h>

rw::writer::VtkWriter::VtkWriter(const std::string &filename,
                                 const std::string &compress_type)
    : d_compressType(compress_type) {

  std::string f = filename + ".vtu";

  d_writer_p = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  d_writer_p->SetFileName(const_cast<char *>(f.c_str()));
}

void rw::writer::VtkWriter::appendNodes(const std::vector<util::Point> *nodes,
                                        const std::vector<util::Point> *u) {

  auto points = vtkSmartPointer<vtkPoints>::New();

  for (size_t i = 0; i < nodes->size(); i++) {

    util::Point p = (*nodes)[i];
    if (u)
      p = p + (*u)[i];
    points->InsertNextPoint(p.d_x, p.d_y, p.d_z);
  }

  d_grid_p = vtkSmartPointer<vtkUnstructuredGrid>::New();
  d_grid_p->SetPoints(points);
}

void rw::writer::VtkWriter::appendMesh(
    const std::vector<util::Point> *nodes, const size_t &element_type,
    const std::vector<size_t> *en_con,
    const std::vector<util::Point> *u) {

  // we write following things to the file
  //
  // Node data
  // 1. Coordinates of nodes (current)
  //
  // Element data
  // 1. element node connectivity
  // 2. element type (either triangle or square)

  // add current position of nodes
  this->appendNodes(nodes, u);

  // get the total number of elements
  size_t num_vertex = util::vtk_map_element_to_num_nodes[element_type];
  size_t num_elems = en_con->size() / num_vertex;

  //
  // process elements data
  //
  // element node connectivity
  auto cells = vtkSmartPointer<vtkCellArray>::New();
  cells->Allocate(num_vertex, num_elems);

  // element type
  int cell_types[num_elems];

  vtkIdType ids[num_vertex];
  for (size_t i = 0; i < num_elems; i++) {

    // get ids of vertex of this element
    for (size_t k = 0; k < num_vertex; k++)
      ids[k] = (*en_con)[num_vertex*i + k];

    cells->InsertNextCell(num_vertex, ids);
    cell_types[i] = element_type;
  }

  // element node connectivity
  d_grid_p->SetCells(cell_types, cells);
}

void rw::writer::VtkWriter::appendPointData(const std::string &name,
                                            const std::vector<uint8_t> *data) {

  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(1);
  array->SetName(name.c_str());

  double value[1];
  for (unsigned char i : *data) {
    value[0] = i;
    array->InsertNextTuple(value);
  }

  d_grid_p->GetPointData()->AddArray(array);
}

void rw::writer::VtkWriter::appendPointData(const std::string &name,
                                            const std::vector<size_t> *data) {

  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(1);
  array->SetName(name.c_str());

  double value[1];
  for (unsigned long i : *data) {
    value[0] = i;
    array->InsertNextTuple(value);
  }

  d_grid_p->GetPointData()->AddArray(array);
}

void rw::writer::VtkWriter::appendPointData(const std::string &name,
                                            const std::vector<int> *data) {

  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(1);
  array->SetName(name.c_str());

  double value[1];
  for (int i : *data) {
    value[0] = i;
    array->InsertNextTuple(value);
  }

  d_grid_p->GetPointData()->AddArray(array);
}

void rw::writer::VtkWriter::appendPointData(const std::string &name,
                                            const std::vector<float> *data) {

  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(1);
  array->SetName(name.c_str());

  double value[1];
  for (float i : *data) {
    value[0] = i;
    array->InsertNextTuple(value);
  }

  d_grid_p->GetPointData()->AddArray(array);
}

void rw::writer::VtkWriter::appendPointData(const std::string &name,
                                            const std::vector<double> *data) {

  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(1);
  array->SetName(name.c_str());

  double value[1];
  for (double i : *data) {
    value[0] = i;
    array->InsertNextTuple(value);
  }

  d_grid_p->GetPointData()->AddArray(array);
}

void rw::writer::VtkWriter::appendPointData(
    const std::string &name, const std::vector<util::Point> *data) {

  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(3);
  array->SetName(name.c_str());

  array->SetComponentName(0, "x");
  array->SetComponentName(1, "y");
  array->SetComponentName(2, "z");

  double value[3];
  for (const auto &i : *data) {
    value[0] = i.d_x;
    value[1] = i.d_y;
    value[2] = i.d_z;
    array->InsertNextTuple(value);
  }

  d_grid_p->GetPointData()->AddArray(array);
}

void rw::writer::VtkWriter::appendPointData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {

  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(6);
  array->SetName(name.c_str());

  array->SetComponentName(0, "xx");
  array->SetComponentName(1, "yy");
  array->SetComponentName(2, "zz");
  array->SetComponentName(3, "yz");
  array->SetComponentName(4, "xz");
  array->SetComponentName(5, "xy");

  double value[6];
  for (const auto &i : *data) {
    value[0] = i(0,0);
    value[1] = i(1,1);
    value[2] = i(2,2);
    value[3] = i(1,2);
    value[4] = i(0,2);
    value[5] = i(0,1);
    array->InsertNextTuple(value);
  }

  d_grid_p->GetPointData()->AddArray(array);
}

void rw::writer::VtkWriter::appendCellData(const std::string &name,
                                           const std::vector<float> *data) {
  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetNumberOfComponents(1);
  array->SetName(name.c_str());

  double value[1];
  for (float i : *data) {
    value[0] = i;
    array->InsertNextTuple(value);
  }

  d_grid_p->GetCellData()->AddArray(array);
}

void rw::writer::VtkWriter::appendCellData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {

  auto array = vtkSmartPointer < vtkDoubleArray > ::New();
  array->SetNumberOfComponents(6);
  array->SetName(name.c_str());

  array->SetComponentName(0, "xx");
  array->SetComponentName(1, "yy");
  array->SetComponentName(2, "zz");
  array->SetComponentName(3, "yz");
  array->SetComponentName(4, "xz");
  array->SetComponentName(5, "xy");

  double value[6];
  for (const auto &i : *data) {
    value[0] = i(0,0);
    value[1] = i(1,1);
    value[2] = i(2,2);
    value[3] = i(1,2);
    value[4] = i(0,2);
    value[5] = i(0,1);
    array->InsertNextTuple(value);
  }

  d_grid_p->GetCellData()->AddArray(array);
}

void rw::writer::VtkWriter::addTimeStep(const double &timestep) {

  auto t = vtkDoubleArray::New();
  t->SetName("TIME");
  t->SetNumberOfTuples(1);
  t->SetTuple1(0, timestep);
  d_grid_p->GetFieldData()->AddArray(t);
}

void rw::writer::VtkWriter::close() {
  d_writer_p->SetInputData(d_grid_p);
  d_writer_p->SetDataModeToAppended();
  d_writer_p->EncodeAppendedDataOn();
  if (d_compressType == "zlib")
    d_writer_p->SetCompressorTypeToZLib();
  else
    d_writer_p->SetCompressor(0);
  d_writer_p->Write();
}

void rw::writer::VtkWriter::appendFieldData(const std::string &name,
                                            const double &data) {

  auto t = vtkDoubleArray::New();
  t->SetName(name.c_str());
  t->SetNumberOfTuples(1);
  t->SetTuple1(0, data);
  d_grid_p->GetFieldData()->AddArray(t);
}

void rw::writer::VtkWriter::appendFieldData(const std::string &name,
                                            const float &data) {

  auto t = vtkDoubleArray::New();
  t->SetName(name.c_str());
  t->SetNumberOfTuples(1);
  t->SetTuple1(0, data);
  d_grid_p->GetFieldData()->AddArray(t);
}
