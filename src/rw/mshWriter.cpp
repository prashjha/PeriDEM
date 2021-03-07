// Copyright (c) 2021 Prashant K. Jha
//
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0.
// (See accompanying file LICENSE.txt)

#include "mshWriter.h"
#include <iostream>
#include <util/feElementDefs.h>

static int ntag = 0;
static int etag = 0;

//extern std::ofstream msh_out;

rw::writer::MshWriter::MshWriter(const std::string &filename,
                                 const std::string &compress_type)
    : d_filename(filename), d_compressType(compress_type), d_file(nullptr) {}

void rw::writer::MshWriter::writeMshDataHeader(const std::string &name, int field_type, size_t
num_data, bool is_node_data) {

  // Write metadata
  if (is_node_data)
    fprintf(d_file, "$NodeData\n");
  else
    fprintf(d_file, "$ElementData\n");

  // number of string the data name has (int)
  fprintf(d_file, "1\n");

  // name of data (string)
  fprintf(d_file, "\"%s\"\n", name.c_str());

  // default (number of real number tags) (int and double)
  fprintf(d_file, "1 \n");
  fprintf(d_file, "1.0 \n");

  // three tags in integer (ints)
  fprintf(d_file, "3 \n");
  if (is_node_data) {
    fprintf(d_file, "%d\n", ntag);
    ntag++;
  } else {
    fprintf(d_file, "%d\n", etag);
    etag++;
  }
  fprintf(d_file, "%d\n", field_type);
  fprintf(d_file, "%d\n", int(num_data));
}

void rw::writer::MshWriter::appendNodes(const std::vector<util::Point> *nodes,
                                        const std::vector<util::Point> *u) {

  // open file stream
  if (!d_file) {
    std::string fname = d_filename + ".msh";
    d_file = fopen(fname.c_str(), "w");
  }
  if (!d_file) {
    std::cerr << "Error: Can not open file = " << d_filename + ".msh" <<".\n";
    exit(1);
  }

  // Write the file header.
  fprintf(d_file, "$MeshFormat\n");
  fprintf(d_file, "2.0 0 %zu\n", sizeof(double));
  fprintf(d_file, "$EndMeshFormat\n");

  // get mesh information
  size_t num_nodes = nodes->size();

  // write the nodes in (n x y z) format
  fprintf(d_file, "$Nodes\n");
  fprintf(d_file, "%zu\n", num_nodes);

  for (size_t i=0; i < num_nodes; i++) {
    auto p = (*nodes)[i];
    if (u)
      p = p + (*u)[i];
    fprintf(d_file, "%zu %lf %lf %lf\n", i + 1, p.d_x, p.d_y, p.d_z);
  }
  fprintf(d_file, "$EndNodes\n");
}

void rw::writer::MshWriter::appendMesh(
    const std::vector<util::Point> *nodes, const size_t &element_type,
    const std::vector<size_t> *en_con,
    const std::vector<util::Point> *u) {

  appendNodes(nodes, u);

  // get mesh information
  size_t num_vertex = util::vtk_map_element_to_num_nodes[element_type];
  size_t num_elems = en_con->size() / num_vertex;
  size_t msh_element_type = util::vtk_to_msh_element_type_map[element_type];

  // write the connectivity
  fprintf(d_file, "$Elements\n");
  fprintf(d_file, "%zu\n", num_elems);

  // loop over the elements
  for (size_t e=0; e < num_elems; e++) {

    // elements ids are 1 based in Gmsh
    fprintf(d_file, "%zu %zu 2 0 6 ", e+1, msh_element_type);

    // write ids of node (numbering starts with 1)
    for (size_t v=0; v<num_vertex; v++)
      fprintf(d_file, "%zu ", (*en_con)[e * num_vertex + v] + 1);

    fprintf(d_file, "\n");
  } // element loop
  fprintf(d_file, "$EndElements\n");
}

void rw::writer::MshWriter::appendPointData(const std::string &name,
                                            const std::vector<uint8_t> *data) {

  // Write metadata
  writeMshDataHeader(name, 1, data->size(), true);
  for (size_t i=0; i < data->size(); i++) {
    double d = (*data)[i];
    fprintf(d_file, "%zu %lf\n", i + 1, d);
  }
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendPointData(const std::string &name,
                                            const std::vector<size_t> *data) {

  // Write metadata
  writeMshDataHeader(name, 1, data->size(), true);
  for (size_t i=0; i < data->size(); i++) {
    double d = (*data)[i];
    fprintf(d_file, "%zu %lf\n", i + 1, d);
  }
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendPointData(const std::string &name,
                                            const std::vector<int> *data) {
  // Write metadata
  writeMshDataHeader(name, 1, data->size(), true);
  for (size_t i=0; i < data->size(); i++) {
    double d = (*data)[i];
    fprintf(d_file, "%zu %lf\n", i + 1, d);
  }
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendPointData(const std::string &name,
                                            const std::vector<float> *data) {
  // Write metadata
  writeMshDataHeader(name, 1, data->size(), true);
  for (size_t i=0; i < data->size(); i++) {
    double d = (*data)[i];
    fprintf(d_file, "%zu %lf\n", i + 1, d);
  }
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendPointData(const std::string &name,
                                            const std::vector<double> *data) {
  // Write metadata
  writeMshDataHeader(name, 1, data->size(), true);
  for (size_t i=0; i < data->size(); i++) {
    double d = (*data)[i];
    fprintf(d_file, "%zu %lf\n", i + 1, d);
  }
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendPointData(
    const std::string &name, const std::vector<util::Point> *data) {
  // Write metadata
  writeMshDataHeader(name, 3, data->size(), true);
  for (size_t i=0; i < data->size(); i++) {
    auto d = (*data)[i];
    fprintf(d_file, "%zu %lf %lf %lf\n", i + 1, d.d_x, d.d_y, d.d_z);
  }
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendPointData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {
  // Write metadata
  writeMshDataHeader(name, 6, data->size(), true);
  for (size_t i=0; i < data->size(); i++) {
    auto d = (*data)[i];
    fprintf(d_file, "%zu %lf %lf %lf %lf %lf %lf\n", i + 1, d(0,0), d(1,1),
            d(2,2), d(1,2), d(0,2), d(0,1));
  }
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendCellData(const std::string &name,
                                           const std::vector<float> *data) {
  // Write metadata
  writeMshDataHeader(name, 1, data->size(), false);
  for (size_t i=0; i < data->size(); i++) {
    double d = (*data)[i];
    fprintf(d_file, "%zu %lf\n", i + 1, d);
  }
  fprintf(d_file, "$EndElementData\n");
}

void rw::writer::MshWriter::appendCellData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {
  // Write metadata
  writeMshDataHeader(name, 6, data->size(), false);
  for (size_t i=0; i < data->size(); i++) {
    auto d = (*data)[i];
    fprintf(d_file, "%zu %lf %lf %lf %lf %lf %lf\n", i + 1, d(0,0), d(1,1),
            d(2,2), d(1,2), d(0,2), d(0,1));
  }
  fprintf(d_file, "$EndElementData\n");
}

void rw::writer::MshWriter::addTimeStep(const double &timestep) {
  // we add field data as simply node data

  // Write metadata
  writeMshDataHeader("time", 1, 1, true);
  fprintf(d_file, "1 %lf\n", timestep);
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::close() {
  ntag = 0;
  etag = 0;
  d_filename.clear();
  fclose(d_file);
}

void rw::writer::MshWriter::appendFieldData(const std::string &name,
                                            const double &data) {
  // we add field data as simply node data

  // Write metadata
  writeMshDataHeader(name, 1, 1, true);
  fprintf(d_file, "1 %lf\n", data);
  fprintf(d_file, "$EndNodeData\n");
}

void rw::writer::MshWriter::appendFieldData(const std::string &name,
                                            const float &data) {
  // we add field data as simply node data

  // Write metadata
  writeMshDataHeader(name, 1, 1, true);
  fprintf(d_file, "1 %lf\n", data);
  fprintf(d_file, "$EndNodeData\n");
}