/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
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
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>

#include "mesh/mesh.h"
#include <cstdint>
#include "data/modelData.h"
#include "particle/baseParticle.h"
#include "particle/particleMpi.h"
#include "util/parallelUtil.h"

#include "util/vecMethods.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

rw::writer::VtkParticleWriter::VtkParticleWriter(const std::string &filename,
                                 const std::string &compress_type)
    : d_compressType(compress_type) {

  std::string f = filename + ".vtu";

  d_writer_p = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  d_writer_p->SetFileName(const_cast<char *>(f.c_str()));
}

void rw::writer::VtkParticleWriter::appendNodes(
    const data::ModelData *model,
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
      p_tag[0] = double(model->getParticleFromAllList(pi)->getId());
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
      p_tag[0] = double(model->getParticleFromAllList(pi)->d_groups.at("geom_id"));
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

  // handle damage function phi = 1 - (intact bond volume)/(horizon volume)
  // (Silling 2000/2003, Trask, Bhattacharya "fraction of broken bonds")
  if (util::methods::isTagInList("Damage", tags) && !model->d_phi.empty()) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Damage");

    for (const auto &n : model->d_phi) {
      p_tag[0] = double(n);
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // damage phi

  // handle broken-bond count fraction (Bhattacharya & Lipton damage)
  if (util::methods::isTagInList("Damage_Bond", tags) &&
      !model->d_phiBond.empty()) {

    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName("Damage_Bond");

    for (const auto &n : model->d_phiBond) {
      p_tag[0] = double(n);
      array->InsertNextTuple(p_tag);
    }

    // write
    d_grid_p->GetPointData()->AddArray(array);
  } // damage bond fraction

  // handle theta
  if (util::methods::isTagInList("Theta", tags)) {

    if (model->getParticleFromAllList(0)->d_material_p->isStateActive()) {

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
    const data::ModelData *model,
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
  for (const auto &p : model->d_particlesListTypeAll) {
    //const auto &rp = p->d_rp_p;
    num_elems += p->getMeshP()->getNumElements();
    auto n =
        util::vtk_map_element_to_num_nodes[p->getMeshP()->getElementType()];
    if (num_vertex < n)
      num_vertex = n;
  }

  if (num_elems == 0)
    return;

  // element node connectivity
  auto cells = vtkSmartPointer<vtkCellArray>::New();
  // VTK 9+: legacy Allocate(sz,ext) maps to AllocateExact(sz,sz); use AllocateEstimate.
  cells->AllocateEstimate(static_cast<vtkIdType>(num_elems), static_cast<vtkIdType>(num_vertex));

  // VTK 9+: prefer vtkUnsignedCharArray for cell types (XML writer path).
  auto cellTypeArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  cellTypeArray->SetNumberOfValues(static_cast<vtkIdType>(num_elems));

  // loop over particles
  size_t global_elem_counter = 0;
  for (const auto &p : model->d_particlesListTypeAll) {
    // get mesh of reference particle in this zone
    const auto &mesh = p->getMeshP();

    // get element type
    size_t element_type = mesh->getElementType();

    // loop over elements of this particle
    size_t num_vertex_p = util::vtk_map_element_to_num_nodes[element_type];
    vtkIdType ids[8];
    for (size_t e = 0; e < mesh->getNumElements(); e++) {
      auto elem = mesh->getElementConnectivity(e);

      // assign global ids to the nodes
      for (size_t n = 0; n < elem.size(); n++)
        ids[n] = elem[n] + p->d_globStart;

      cells->InsertNextCell(static_cast<int>(num_vertex_p), ids);
      cellTypeArray->SetValue(static_cast<vtkIdType>(global_elem_counter),
                              static_cast<unsigned char>(element_type));

      // increment global element counter
      global_elem_counter++;
    }
  }

  d_grid_p->SetCells(cellTypeArray, cells);
}

namespace {

void appendPointArraysForNodes(vtkUnstructuredGrid *grid,
                               const data::ModelData *model,
                               const std::vector<size_t> &gids,
                               const std::vector<std::string> &tags) {
  double value[3] = {0., 0., 0.};
  auto add_vec3 = [&](const char *name, const std::vector<util::Point> &field) {
    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(3);
    array->SetName(name);
    array->SetComponentName(0, "x");
    array->SetComponentName(1, "y");
    array->SetComponentName(2, "z");
    for (size_t g : gids) {
      const auto &ui = field[g];
      value[0] = ui.d_x;
      value[1] = ui.d_y;
      value[2] = ui.d_z;
      array->InsertNextTuple(value);
    }
    grid->GetPointData()->AddArray(array);
  };
  auto add_scalar = [&](const char *name, auto getter) {
    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(1);
    array->SetName(name);
    for (size_t g : gids) {
      value[0] = static_cast<double>(getter(g));
      array->InsertNextTuple(value);
    }
    grid->GetPointData()->AddArray(array);
  };

  if (util::methods::isTagInList("Displacement", tags))
    add_vec3("Displacement", model->d_u);
  if (util::methods::isTagInList("Velocity", tags))
    add_vec3("Velocity", model->d_v);
  if (util::methods::isTagInList("Force_Density", tags))
    add_vec3("Force_Density", model->d_f);
  if (util::methods::isTagInList("Force", tags)) {
    auto array = vtkSmartPointer<vtkDoubleArray>::New();
    array->SetNumberOfComponents(3);
    array->SetName("Force");
    array->SetComponentName(0, "x");
    array->SetComponentName(1, "y");
    array->SetComponentName(2, "z");
    for (size_t g : gids) {
      const auto &fi = model->d_f[g];
      const double vol = model->d_vol[g];
      value[0] = fi.d_x * vol;
      value[1] = fi.d_y * vol;
      value[2] = fi.d_z * vol;
      array->InsertNextTuple(value);
    }
    grid->GetPointData()->AddArray(array);
  }
  if (util::methods::isTagInList("Damage_Z", tags) && !model->d_Z.empty())
    add_scalar("Damage_Z", [&](size_t g) { return model->d_Z[g]; });
  if (util::methods::isTagInList("Damage", tags) && !model->d_phi.empty())
    add_scalar("Damage", [&](size_t g) { return model->d_phi[g]; });
  if (util::methods::isTagInList("Damage_Bond", tags) && !model->d_phiBond.empty())
    add_scalar("Damage_Bond", [&](size_t g) { return model->d_phiBond[g]; });
  if (util::methods::isTagInList("Particle_ID", tags))
    add_scalar("Particle_ID", [&](size_t g) {
      return static_cast<double>(
          model->getParticleFromAllList(model->d_ptId[g])->getId());
    });
}

} // namespace

void rw::writer::VtkParticleWriter::appendMeshParallelPiece(
    const data::ModelData *model, const std::vector<std::string> &tags) {

  if (model->d_x.empty())
    return;

  const int mpi_size = util::parallel::mpiSize();
  const int mpi_rank = util::parallel::mpiRank();
  if (mpi_size <= 1) {
    appendMesh(model, tags);
    return;
  }

  std::unordered_set<size_t> node_set;
  struct LocalElem {
    size_t type{};
    std::vector<size_t> gids;
  };
  std::vector<LocalElem> elems;

  if (model->d_pdDofMpi &&
      model->d_pdNodePartition.size() == model->d_x.size()) {
    // Cell owner = min node-owner among vertices; piece includes all nodes of
    // those cells (may include halo nodes for connectivity).
    for (const auto &p : model->d_particlesListTypeAll) {
      const auto &mesh = p->getMeshP();
      const size_t element_type = mesh->getElementType();
      for (size_t e = 0; e < mesh->getNumElements(); ++e) {
        auto conn = mesh->getElementConnectivity(e);
        size_t cell_owner = model->d_pdNodePartition[conn[0] + p->d_globStart];
        for (size_t n = 1; n < conn.size(); ++n)
          cell_owner = std::min(
              cell_owner,
              model->d_pdNodePartition[conn[n] + p->d_globStart]);
        if (static_cast<int>(cell_owner) != mpi_rank)
          continue;
        LocalElem le;
        le.type = element_type;
        le.gids.reserve(conn.size());
        for (size_t n : conn) {
          const size_t g = n + p->d_globStart;
          le.gids.push_back(g);
          node_set.insert(g);
        }
        elems.push_back(std::move(le));
      }
    }
  } else {
    // Particle-MPI: whole owned grains; walls only on rank 0.
    for (const auto &p : model->d_particlesListTypeAll) {
      if (p->isWall()) {
        if (mpi_rank != 0)
          continue;
      } else if (!particle::isLocallyOwned(*p)) {
        continue;
      }
      for (size_t i = 0; i < p->getNumNodes(); ++i)
        node_set.insert(p->d_globStart + i);
      const auto &mesh = p->getMeshP();
      const size_t element_type = mesh->getElementType();
      for (size_t e = 0; e < mesh->getNumElements(); ++e) {
        auto conn = mesh->getElementConnectivity(e);
        LocalElem le;
        le.type = element_type;
        for (size_t n : conn)
          le.gids.push_back(n + p->d_globStart);
        elems.push_back(std::move(le));
      }
    }
  }

  if (node_set.empty()) {
    // Empty piece still valid for PVTU.
    d_grid_p = vtkSmartPointer<vtkUnstructuredGrid>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    d_grid_p->SetPoints(points);
    return;
  }

  std::vector<size_t> gids(node_set.begin(), node_set.end());
  std::sort(gids.begin(), gids.end());
  std::unordered_map<size_t, vtkIdType> g2l;
  g2l.reserve(gids.size());
  auto points = vtkSmartPointer<vtkPoints>::New();
  for (size_t i = 0; i < gids.size(); ++i) {
    const auto &x = model->d_x[gids[i]];
    points->InsertNextPoint(x.d_x, x.d_y, x.d_z);
    g2l[gids[i]] = static_cast<vtkIdType>(i);
  }

  d_grid_p = vtkSmartPointer<vtkUnstructuredGrid>::New();
  d_grid_p->SetPoints(points);
  appendPointArraysForNodes(d_grid_p, model, gids, tags);

  if (elems.empty())
    return;

  size_t num_vertex = 0;
  for (const auto &le : elems)
    num_vertex = std::max(num_vertex, le.gids.size());

  auto cells = vtkSmartPointer<vtkCellArray>::New();
  cells->AllocateEstimate(static_cast<vtkIdType>(elems.size()),
                          static_cast<vtkIdType>(num_vertex));
  auto cellTypeArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  cellTypeArray->SetNumberOfValues(static_cast<vtkIdType>(elems.size()));

  vtkIdType ids[8];
  for (size_t ei = 0; ei < elems.size(); ++ei) {
    const auto &le = elems[ei];
    for (size_t n = 0; n < le.gids.size(); ++n)
      ids[n] = g2l.at(le.gids[n]);
    cells->InsertNextCell(static_cast<int>(le.gids.size()), ids);
    cellTypeArray->SetValue(static_cast<vtkIdType>(ei),
                            static_cast<unsigned char>(le.type));
  }
  d_grid_p->SetCells(cellTypeArray, cells);
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
  // VTK 9.1 (linked by PeriDEM) cannot parse AppendedData VTUs from this
  // writer as Restart.File (base64 → "junk after document element"; raw →
  // "invalid token"). Ascii matches working restart files and reloads cleanly.
  d_writer_p->SetDataModeToAscii();
  d_writer_p->SetCompressor(0);
  d_writer_p->Write();
}

void rw::writer::VtkParticleWriter::appendContactData(
    const data::ModelData *model,
    const std::vector<size_t> *processed_nodes,
    const std::vector <
        std::pair<size_t, size_t>> *processed_elems) {

  if (processed_nodes->size() == 0)
    return;

  // write point data
  auto points = vtkSmartPointer<vtkPoints>::New();


  const size_t num_nodes = processed_nodes->size();
  const size_t num_elems = processed_elems->size();

  if (num_elems == 0)
    return;

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
  cells->AllocateEstimate(static_cast<vtkIdType>(num_elems), static_cast<vtkIdType>(num_vertex));

  auto cellTypeArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  cellTypeArray->SetNumberOfValues(static_cast<vtkIdType>(num_elems));

  vtkIdType ids[num_vertex];
  for (size_t i = 0; i < num_elems; i++) {

    ids[0] = (*processed_elems)[i].first;
    ids[1] = (*processed_elems)[i].second;

    cells->InsertNextCell(static_cast<int>(num_vertex), ids);
    cellTypeArray->SetValue(static_cast<vtkIdType>(i), static_cast<unsigned char>(vtk_element_type));
  }

  d_grid_p->SetCells(cellTypeArray, cells);

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


void rw::writer::VtkParticleWriter::appendStrainStress(
        const data::ModelData *model) {

  if (model->d_xQuadCur.size() == 0) {
    std::cout << "VtkParticleWriter::appendStrainStress: Nothing to write.\n";
    return;
  }

  // write point data
  auto points = vtkSmartPointer<vtkPoints>::New();

  // get all the quadrature points first
  for (const auto &x : model->d_xQuadCur)
    points->InsertNextPoint(x.d_x, x.d_y, x.d_z);

  // write point data
  d_grid_p = vtkSmartPointer<vtkUnstructuredGrid>::New();
  d_grid_p->SetPoints(points);

  // now write data associated to nodes (in this case, quad points are nodes)
  double value[3] = {0., 0., 0.};
  double value_s[6] = {0., 0., 0., 0., 0., 0.};
  double p_tag[1] = {0.};

  auto array_strain = vtkSmartPointer<vtkDoubleArray>::New();
  array_strain->SetNumberOfComponents(6);
  array_strain->SetName("Strain");

  auto array_stress = vtkSmartPointer<vtkDoubleArray>::New();
  array_stress->SetNumberOfComponents(6);
  array_stress->SetName("Stress");

  std::vector<std::string> coord_strings = {"xx", "yy", "zz", "yz", "xz", "xy"};
  for (size_t i =0; i<6; i++) {
    array_strain->SetComponentName(i, coord_strings[i].c_str());
    array_stress->SetComponentName(i, coord_strings[i].c_str());
  }

  for (size_t i=0; i<model->d_strain.size(); i++) {

    model->d_strain[i].copy(value_s);
    array_strain->InsertNextTuple(value_s);

    model->d_stress[i].copy(value_s);
    array_stress->InsertNextTuple(value_s);
  }


  // write
  d_grid_p->GetPointData()->AddArray(array_strain);
  d_grid_p->GetPointData()->AddArray(array_stress);
}
