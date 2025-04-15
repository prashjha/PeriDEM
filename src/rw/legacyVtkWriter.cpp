// Copyright (c) 2021 Prashant K. Jha
//
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0.
// (See accompanying file LICENSE.txt)

#include "legacyVtkWriter.h"

rw::writer::LegacyVtkWriter::LegacyVtkWriter(const std::string &filename,
                                 const std::string &compress_type)
    : d_filename(filename), d_compressType(compress_type) {}

void rw::writer::LegacyVtkWriter::appendNodes(
    const std::vector<util::Point> *nodes) {}

void rw::writer::LegacyVtkWriter::appendNodes(const std::vector<util::Point> *nodes,
                                        const std::vector<util::Point> *u) {}

void rw::writer::LegacyVtkWriter::appendMesh(
    const std::vector<util::Point> *nodes, const size_t &element_type,
    const std::vector<size_t> *en_con,
    const std::vector<util::Point> *u) {}

void rw::writer::LegacyVtkWriter::appendPointData(const std::string &name,
                                            const std::vector<uint8_t> *data) {}

void rw::writer::LegacyVtkWriter::appendPointData(const std::string &name,
                                            const std::vector<size_t> *data) {}

void rw::writer::LegacyVtkWriter::appendPointData(const std::string &name,
                                            const std::vector<int> *data) {}

void rw::writer::LegacyVtkWriter::appendPointData(const std::string &name,
                                            const std::vector<float> *data) {}

void rw::writer::LegacyVtkWriter::appendPointData(const std::string &name,
                                            const std::vector<double> *data) {}

void rw::writer::LegacyVtkWriter::appendPointData(
    const std::string &name, const std::vector<util::Point> *data) {}

void rw::writer::LegacyVtkWriter::appendPointData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {}

void rw::writer::LegacyVtkWriter::appendCellData(const std::string &name,
                                           const std::vector<float> *data) {}

void rw::writer::LegacyVtkWriter::appendCellData(
    const std::string &name, const std::vector<util::SymMatrix3> *data) {}

void rw::writer::LegacyVtkWriter::addTimeStep(const double &timestep) {}

void rw::writer::LegacyVtkWriter::close() {}

void rw::writer::LegacyVtkWriter::appendFieldData(const std::string &name,
                                            const double &data) {}

void rw::writer::LegacyVtkWriter::appendFieldData(const std::string &name,
                                            const float &data) {}