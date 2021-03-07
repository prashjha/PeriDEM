/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "vtkParticleWriter.h"
#include <util/feElementDefs.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnsignedIntArray.h>

#include "fe/mesh.h"
#include <cstdint>
#include "model/modelData.h"
#include "particle/baseParticle.h"
#include "particle/particle.h"
#include "particle/wall.h"
#include "particle/refParticle.h"

#include "util/methods.h"

rw::writer::VtkParticleWriter::VtkParticleWriter(const std::string &filename,
                                 const std::string &compress_type)
    : d_compressType(compress_type) {

  std::string f = filename + ".vtu";

  d_writer_p = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  d_writer_p->SetFileName(const_cast<char *>(f.c_str()));
}

void rw::writer::VtkParticleWriter::appendNodes(
    const model::ModelData *model,
    const std::vector<std::string> &tags) {

  if (model->d_x.size() == 0)
    return;

  // write point data
  auto points = vtkSmartPointer<vtkPoints>::New();

  // get all the nodes first
  for (const auto &x : model->d_x)
    points->InsertNextPoint(x.d_x, x.d_y, x.d_z);

  // write point data
  d_grid_p = vtkSmartPointer<vtkUnstructuredGrid>::New();
  d_grid_p->SetPoints(points);

  // now write data associated to nodes in both particle and wall
  double value[3];
  value[0] = 0;
  value[1] = 0;
  value[2] = 0;
  double p_tag[1];
  p_tag[0] = 0;

  // handle displacement
  if (util::methods::isTagInList("Displacement", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(3);
    array->SetName("Displacement");

    array->SetComponentName(0, "x");
    array->SetComponentName(1, "y");
    array->SetComponentName(2, "z");

    for (const auto &ui : model->d_u) {
      value[0] = ui.d_x;
      value[1] = ui.d_y;
      value[2] = ui.d_z;
      array->InsertNextTuple(value);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // displacement

  // handle velocity
  if (util::methods::isTagInList("Velocity", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(3);
    array->SetName("Velocity");

    array->SetComponentName(0, "x");
    array->SetComponentName(1, "y");
    array->SetComponentName(2, "z");

    for (const auto &ui : model->d_v) {
      value[0] = ui.d_x;
      value[1] = ui.d_y;
      value[2] = ui.d_z;
      array->InsertNextTuple(value);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // velocity

  // handle force
  if (util::methods::isTagInList("Force_Density", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(3);
    array->SetName("Force_Density");

    array->SetComponentName(0, "x");
    array->SetComponentName(1, "y");
    array->SetComponentName(2, "z");

    for (const auto &ui : model->d_f) {
      value[0] = ui.d_x;
      value[1] = ui.d_y;
      value[2] = ui.d_z;
      array->InsertNextTuple(value);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // force

  // handle force
  if (util::methods::isTagInList("Force", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(3);
    array->SetName("Force");

    array->SetComponentName(0, "x");
    array->SetComponentName(1, "y");
    array->SetComponentName(2, "z");

    size_t i_count = 0;
    for (const auto &ui : model->d_f) {
      const auto &voli = model->d_vol[i_count];
      value[0] = ui.d_x * voli;
      value[1] = ui.d_y * voli;
      value[2] = ui.d_z * voli;
      array->InsertNextTuple(value);

      i_count++;
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // force

  // handle fixity
  if (util::methods::isTagInList("Fixity", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Fixity");

    for (const auto &n : model->d_fix) {
      p_tag[0] = double(n);
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // fixity

  // handle Particle ID
  if (util::methods::isTagInList("Particle_ID", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Particle_ID");

    for (size_t i = 0; i<model->d_x.size(); i++) {
      auto pi = model->getPtId(i);
      p_tag[0] = double(model->getBaseParticle(pi)->getTypeId());
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // Particle ID

  // handle Zone ID
  if (util::methods::isTagInList("Zone_ID", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Zone_ID");

    for (size_t i = 0; i<model->d_x.size(); i++) {
      auto pi = model->getPtId(i);
      p_tag[0] = double(model->getBaseParticle(pi)->d_zoneId);
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // Zone ID

  // handle force fixity
  if (util::methods::isTagInList("Force_Fixity", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Force_Fixity");

    for (const auto &n : model->d_forceFixity) {
      p_tag[0] = double(n);
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // force fixity

  // handle nodal volume
  if (util::methods::isTagInList("Nodal_Volume", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Nodal_Volume");

    for (const auto &n : model->d_vol) {
      p_tag[0] = double(n);
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // nodal volume

  // handle damage_Z
  if (util::methods::isTagInList("Damage_Z", tags)) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Damage_Z");

    for (const auto &n : model->d_Z) {
      p_tag[0] = double(n);
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // damage_Z

  // handle theta
  if (util::methods::isTagInList("Theta", tags)) {

    if (model->getBaseParticle(0)->d_material_p->isStateActive()) {

      auto array = vtkSmartPointer<vtkDoubleArray>::New();
      array->SetNumberOfComponents(1);
      array->SetName("Theta");

      for (const auto &n : model->d_thetaX) {
        p_tag[0] = double(n);
        array->InsertNextTuple(p_tag);
      }

      // write
      d_grid_p->GetPointData()->AddArray(array);
    }
  } // Theta
}

void rw::writer::VtkParticleWriter::appendMesh(
    const model::ModelData *model,
    const std::vector<std::string> &tags) {

  if (model->d_x.size() == 0)
    return;

  // write point data
  appendNodes(model, tags);

  //
  // process elements data
  //

  // get total number of elements and maximum number of vertex in any element
  size_t num_elems = 0;
  size_t num_vertex = 0;

  // count number of elements in all particles
  for (const auto &p : model->d_particles) {
    const auto &rp = p->d_rp_p;
    num_elems += rp->getMeshP()->getNumElements();
    auto n =
        util::vtk_map_element_to_num_nodes[rp->getMeshP()->getElementType()];
    if (num_vertex < n)
      num_vertex = n;
  }

  // count number of elements in all walls
  for (const auto &p: model->d_walls) {
    num_elems += p->d_mesh_p->getNumElements();
    auto n =
        util::vtk_map_element_to_num_nodes[p->d_mesh_p->getElementType()];
    if (num_vertex < n)
      num_vertex = n;
  }

  // element node connectivity
  auto cells = vtkSmartPointer<vtkCellArray>::New();
  cells->Allocate(num_vertex, num_elems);

  // element type
  int cell_types[num_elems];

  // loop over particles
  size_t global_elem_counter = 0;
  size_t node_number_start = 0;
  for (const auto &p : model->d_particles) {
    // get mesh of reference particle in this zone
    const auto &mesh = p->d_rp_p->getMeshP();

    // get element type
    size_t element_type = mesh->getElementType();

    // loop over elements of this particle
    size_t num_vertex_p = util::vtk_map_element_to_num_nodes[element_type];
    vtkIdType ids[num_vertex_p];
    for (size_t e =0; e < mesh->getNumElements(); e++) {
      auto elem = mesh->getElementConnectivity(e);

      // assign global ids to the nodes
      for (size_t n=0; n<elem.size(); n++)
        ids[n] = elem[n] + node_number_start;

      cells->InsertNextCell(num_vertex_p, ids);
      cell_types[global_elem_counter] = element_type;

      // increment global element counter
      global_elem_counter++;
    }

    // adjust global node ids
    node_number_start += mesh->getNumNodes();
  }

  // loop over walls
  for (const auto &p : model->d_walls) {
    // get mesh of reference particle in this zone
    const auto &mesh = p->d_mesh_p;

    // get element type
    size_t element_type = mesh->getElementType();

    // loop over elements of this particle
    size_t num_vertex_p = util::vtk_map_element_to_num_nodes[element_type];
    vtkIdType ids[num_vertex_p];
    for (size_t e =0; e < mesh->getNumElements(); e++) {
      auto elem = mesh->getElementConnectivity(e);

      // assign global ids to the nodes
      for (size_t n=0; n<elem.size(); n++)
        ids[n] = elem[n] + node_number_start;

      cells->InsertNextCell(num_vertex_p, ids);
      cell_types[global_elem_counter] = element_type;

      // increment global element counter
      global_elem_counter++;
    }

    // adjust global node ids
    node_number_start += mesh->getNumNodes();
  }

  // element node connectivity
  d_grid_p->SetCells(cell_types, cells);
}

void rw::writer::VtkParticleWriter::addTimeStep(const double &timestep) {

  auto t = vtkDoubleArray::New();
  t->SetName("TIME");
  t->SetNumberOfTuples(1);
  t->SetTuple1(0, timestep);
  d_grid_p->GetFieldData()->AddArray(t);
}

void rw::writer::VtkParticleWriter::close() {
  d_writer_p->SetInputData(d_grid_p);
  d_writer_p->SetDataModeToAppended();
  d_writer_p->EncodeAppendedDataOn();
  if (d_compressType == "zlib")
    d_writer_p->SetCompressorTypeToZLib();
  else
    d_writer_p->SetCompressor(0);
  d_writer_p->Write();
}

void rw::writer::VtkParticleWriter::appendContactData(
    const model::ModelData *model,
    const std::vector<size_t> *processed_nodes,
    const std::vector <
        std::pair<size_t, size_t>> *processed_elems) {

  if (processed_nodes->size() == 0)
    return;

  // write point data
  auto points = vtkSmartPointer<vtkPoints>::New();


  const size_t num_nodes = processed_nodes->size();
  const size_t num_elems = processed_elems->size();

  // get all the nodes first
  for (const auto &i : *processed_nodes) {
    const auto &x = model->d_x[i];
    points->InsertNextPoint(x.d_x, x.d_y, x.d_z);
  }

  // write point data
  d_grid_p = vtkSmartPointer<vtkUnstructuredGrid>::New();
  d_grid_p->SetPoints(points);

  // now wrtie element data
  const size_t vtk_element_type = 3; // line element
  const size_t num_vertex = 2;
  // element node connectivity
  auto cells = vtkSmartPointer<vtkCellArray>::New();
  cells->Allocate(num_vertex, num_elems);

  // element type
  int cell_types[num_elems];

  vtkIdType ids[num_vertex];
  for (size_t i = 0; i < num_elems; i++) {

    ids[0] = (*processed_elems)[i].first;
    ids[1] = (*processed_elems)[i].second;

    cells->InsertNextCell(num_vertex, ids);
    cell_types[i] = vtk_element_type;
  }

  // element node connectivity
  d_grid_p->SetCells(cell_types, cells);

  // write cell data (normal direction)
  {
    auto array = vtkSmartPointer < vtkDoubleArray > ::New();
    array->SetNumberOfComponents(3);
    array->SetName("Normal");
    array->SetComponentName(0, "x");
    array->SetComponentName(1, "y");
    array->SetComponentName(2, "z");

    double value[3];
    for (size_t i = 0; i < num_elems; i++) {

      ids[0] = (*processed_elems)[i].first;
      ids[1] = (*processed_elems)[i].second;

      auto glob_id1 = (*processed_nodes)[ids[0]];
      auto glob_id2 = (*processed_nodes)[ids[1]];

      const auto &x1 = model->d_x[glob_id1];
      const auto &x2 = model->d_x[glob_id2];

      auto xd = (x1 - x2)/((x2 - x1).length());

      value[0] = xd[0];
      value[1] = xd[1];
      value[2] = xd[2];
      array->InsertNextTuple(value);
    }

    d_grid_p->GetCellData()->AddArray(array);
  }
}

