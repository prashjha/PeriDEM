/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "vtkParticleReader.h"
#include <util/feElementDefs.h>
#include <vtkAbstractArray.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>

#include "fe/mesh.h"
#include <cstdint>
#include "model/modelData.h"
#include "particle/baseParticle.h"
#include "particle/particle.h"
#include "particle/wall.h"
#include "particle/refParticle.h"

#include "util/methods.h"

rw::reader::VtkParticleReader::VtkParticleReader(const std::string &filename) {

  std::string f = filename + ".vtu";

  d_reader_p = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  d_reader_p->SetFileName(const_cast<char *>(f.c_str()));
  d_reader_p->Update();
}

void rw::reader::VtkParticleReader::readNodes(
    model::ModelData *model) {

  if (model->d_x.size() == 0)
    return;

  //
  // For nodes, we read following data
  // 1. nodes current configuration
  // 2. nodes displacement (if not then compute from current and reference
  // config)
  // 3. nodes velocity (if not then produce error)
  //
  d_grid_p = d_reader_p->GetOutput();
  vtkIdType num_nodes = d_grid_p->GetNumberOfPoints();

  if (num_nodes != model->d_x.size()) {
    std::cerr << "Error: Number of points data = " << num_nodes <<
                 " in file does not match the number of points = "
              << model->d_x.size() << ". Aborting reading.\n";
    exit(1);
  }

  // read current configuration
  for (size_t i = 0; i < num_nodes; i++) {
    vtkIdType id = i;

    double x[3];
    d_grid_p->GetPoint(id, x);

    util::Point i_node = util::Point(x[0], x[1], x[2]);
    model->setX(i, i_node);
  }

  // read point field data
  vtkPointData *p_field = d_grid_p->GetPointData();

  std::vector<std::string> tags = {"Displacement", "Velocity"};
  for (auto tag: tags) {

    if (p_field->HasArray(tag.c_str())) {
      vtkDataArray *array = p_field->GetArray(tag.c_str());

      auto data_a = vtkSmartPointer<vtkDoubleArray>::New();
      data_a->SetNumberOfComponents(3);
      data_a->Allocate(3, 1); // allocate memory

      for (size_t i = 0; i < array->GetNumberOfTuples(); i++) {
        array->GetTuples(i, i, data_a);
        auto xi = util::Point(data_a->GetValue(0), data_a->GetValue(1),
                               data_a->GetValue(2));
        if (tag == "Displacement")
          model->setU(i, xi);
        else if (tag == "Velocity")
          model->setV(i, xi);
      }
    } else {

      if (tag == "Velocity") {
        std::cerr << "Error: Restart file does not have <Velocity> data. "
                     "Aborting reading.\n";
        exit(1);
      } else if (tag == "Displacement") {
        // compute from current and reference configuration
        for (size_t i=0; i<model->d_x.size(); i++)
          model->setU(i, model->getX(i) - model->getXRef(i));
      }
    }
  }
}


void rw::reader::VtkParticleReader::close() {
  // delete d_reader_p;
  // delete d_grid_p;
}