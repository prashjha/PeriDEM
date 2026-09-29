/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "rw/particleOutput.h"

#include "data/modelData.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "particle/baseParticle.h"
#include "rw/vtkParticleWriter.h"
#include "rw/pvdCollectionWriter.h"
#include "mesh/meshUtil.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "inp/input.h"

#include <mpi.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <vector>

void rw::writeOutput(data::ModelData &data) {


  // write out % completion of simulation at 10% interval
  {
    float p = float(data.d_n) * 100. / data.d_modelDeck_p->d_Nt;
    int m = std::max(1, int(data.d_modelDeck_p->d_Nt / 10));
    if (data.d_n % m == 0 && int(p) > 0)
      util::io::log(0, std::format("{}: Simulation {}% complete\n",
                      data.d_name, int(p)));
    ;
  }

  util::io::log(2, std::format("{}: Output step = {}, time = {:.6f} \n",
                  data.d_name, data.d_n, data.d_time));

  if (data.d_outputDeck_p->d_debug > 0 and data.getKeyData("debug_once") < 0) {

    data.setKeyData("debug_once", 1);

    size_t nt = 1;
    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "*******************************************\n";
    oss << tabS << "Debug various input decks\n\n\n";
    oss << data.d_modelDeck_p->printStr(nt + 1);
    oss << data.d_particleDeck_p->printStr(nt + 1);
    oss << data.d_outputDeck_p->printStr(nt + 1);
    oss << data.d_restartDeck_p->printStr(nt + 1);
    oss << data.d_testDeck_p->printStr(nt + 1);
    oss << data.d_bcDeck_p->printStr(nt + 1);
    oss << tabS << "\n\n*******************************************\n";
    oss << tabS << "Debug particle data\n\n\n";
    oss << tabS << "Number of particles = " << data.d_particlesListTypeAll.size() << std::endl;
    oss << tabS << "Number of particle zones = " << data.d_zInfo.size() << std::endl;
    for (auto zone : data.d_zInfo) {
      oss << tabS << "zone of d_zInfo: " << util::io::printStr(zone)
          << std::endl;
    }

    // wall info
    oss << tabS << "Number of walls = " << data.d_particlesListTypeWall.size() << std::endl;
    for (auto &d_wall : data.d_particlesListTypeWall)
      oss << tabS << "Number of nodes in wall " << d_wall->d_id
          << " is " << d_wall->getNumNodes() << std::endl;

    oss << tabS << "h_min = " << data.d_hMin << ", h_max = " << data.d_hMax << std::endl;

    util::io::log(2, oss);
  } // end of debug

  size_t dt_out = data.d_outputDeck_p->d_dtOutCriteria;
  const size_t frame = data.d_n / dt_out;
  const std::string tag =
      data.d_outputDeck_p->d_tagPPFile.empty()
          ? std::to_string(frame)
          : data.d_outputDeck_p->d_tagPPFile + "_" + std::to_string(frame);
  const std::string path = data.d_outputDeck_p->d_path;
  const int mpi_size = util::parallel::mpiSize();
  const int mpi_rank = util::parallel::mpiRank();
  const bool parallel_pieces = mpi_size > 1;

  // Per-rank piece (or single file). No solution gather for I/O.
  std::string piece_stem = path + "output_" + tag;
  if (parallel_pieces)
    piece_stem += "_r" + std::to_string(mpi_rank);

  {
    auto writer = rw::writer::VtkParticleWriter(piece_stem);
    writer.appendMeshParallelPiece(&data, data.d_outputDeck_p->d_outTags);
    writer.addTimeStep(data.d_time);
    writer.close();
  }

  if (data.d_outputDeck_p->d_outFormat == "vtu" &&
      data.d_outputDeck_p->d_pvdCollection) {
    if (parallel_pieces) {
      // Ensure all piece files are on disk before rank 0 writes the .pvtu.
      MPI_Barrier(util::parallel::mpiComm());
      if (mpi_rank == 0) {
        std::vector<std::string> pieces;
        pieces.reserve(static_cast<size_t>(mpi_size));
        for (int r = 0; r < mpi_size; ++r)
          pieces.push_back("output_" + tag + "_r" + std::to_string(r) + ".vtu");
        // Must match appendPointArraysForNodes: ParaView only lists arrays
        // declared in PPointData of the .pvtu (not discovered from pieces).
        const auto &tags = data.d_outputDeck_p->d_outTags;
        std::vector<rw::PvtuPointArray> point_arrays;
        auto add_arr = [&](const char *name, int ncomp) {
          point_arrays.push_back({name, "Float64", ncomp});
        };
        if (util::methods::isTagInList("Displacement", tags))
          add_arr("Displacement", 3);
        if (util::methods::isTagInList("Velocity", tags))
          add_arr("Velocity", 3);
        if (util::methods::isTagInList("Force_Density", tags))
          add_arr("Force_Density", 3);
        if (util::methods::isTagInList("Force", tags))
          add_arr("Force", 3);
        if (util::methods::isTagInList("Damage_Z", tags) && !data.d_Z.empty())
          add_arr("Damage_Z", 1);
        if (util::methods::isTagInList("Damage", tags) && !data.d_phi.empty())
          add_arr("Damage", 1);
        if (util::methods::isTagInList("Damage_Bond", tags) && !data.d_phiBond.empty())
          add_arr("Damage_Bond", 1);
        if (util::methods::isTagInList("Particle_ID", tags))
          add_arr("Particle_ID", 1);
        const std::string pvtu_name = "output_" + tag + ".pvtu";
        rw::writePvtuCollectionFile(path + pvtu_name, pieces, point_arrays);
        data.d_pvdParticleEntries.push_back({data.d_time, pvtu_name});
        rw::writePvdCollectionFile(path + "output.pvd",
                                   data.d_pvdParticleEntries);
      }
    } else {
      data.d_pvdParticleEntries.push_back(
          {data.d_time, "output_" + tag + ".vtu"});
      rw::writePvdCollectionFile(path + "output.pvd",
                                 data.d_pvdParticleEntries);
    }
  }

  if (util::methods::isTagInList("Strain_Stress", data.d_outputDeck_p->d_outTags)) {

    // compute current position of quadrature points and strain/stress data
    {
      // if particle mat data is not computed, compute them
      if (data.d_particlesMatDataList.empty()) {
        for (auto &p: data.d_particlesListTypeAll) {
          data.d_particlesMatDataList.push_back(p->getMaterial()->computeMaterialProperties(
                  p->getMeshP()->getDimension()));
        }
      }

      for (auto &p: data.d_particlesListTypeAll) {

        const auto particle_mesh_p = p->getMeshP();

        mesh::getCurrentQuadPoints(particle_mesh_p.get(), data.d_xRef, data.d_u, data.d_xQuadCur,
                                 p->d_globStart,
                                 p->d_globQuadStart,
                                 data.d_modelDeck_p->d_quadOrder);

        auto isPlaneStrain = p->d_material_p->isPlaneStrain();
        mesh::getStrainStress(particle_mesh_p.get(), data.d_xRef, data.d_u,
                            isPlaneStrain,
                            data.d_strain, data.d_stress,
                            p->d_globStart,
                            p->d_globQuadStart,
                            data.d_particlesMatDataList[p->getId()].d_nu,
                            data.d_particlesMatDataList[p->getId()].d_lambda,
                            data.d_particlesMatDataList[p->getId()].d_mu,
                            true,
                            data.d_modelDeck_p->d_quadOrder);
      } // for loop over particles
    } // compute strain/stress block

    // Strain field is global; only rank 0 writes (legacy path). Prefer primary
    // particle VTU/PVTU for parallel visualization.
    if (mpi_rank == 0) {
      std::string out_filename = path + "output_strain_" + tag;
      auto writer1 = rw::writer::VtkParticleWriter(out_filename);
      writer1.appendStrainStress(&data);
      writer1.addTimeStep(data.d_time);
      writer1.close();

      if (data.d_outputDeck_p->d_outFormat == "vtu" &&
          data.d_outputDeck_p->d_pvdCollection) {
        data.d_pvdStrainEntries.push_back(
            {data.d_time, "output_strain_" + tag + ".vtu"});
        rw::writePvdCollectionFile(path + "output_strain.pvd",
                                   data.d_pvdStrainEntries);
      }
    }
  }

  // output particle locations to csv file (rank 0)
  if (util::methods::isTagInList("Particle_Locations",
                                 data.d_outputDeck_p->d_outTags) &&
      mpi_rank == 0) {

    std::string out_filename = path + "particle_locations_" + tag + ".csv";
    std::ofstream oss(out_filename);
    oss << "i, x, y, z, r\n";
    for (const auto &p : data.d_particlesListTypeAll) {
      auto xc = p->getXCenter();
      oss << p->d_id << ", " << xc.d_x << ", " << xc.d_y << ", " << xc.d_z
          << ", " << p->d_geom_p->boundingRadius() << "\n";
    }
    oss.close();
  }

}
