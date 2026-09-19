/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "pdMpi.h"

#include "data/modelData.h"
#include "inp/input.h"
#include "mesh/meshPartitioning.h"
#include "particle/baseParticle.h"
#include "particle/particleMpi.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "util/point.h"
#include "util/vecMethods.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <stdexcept>
#include <vector>

bool pd::dofMpiEnabled(const data::ModelData &data) {
  return data.d_pdDofMpi;
}

namespace {

void uniqueSorted(std::vector<int> &v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
}

void buildGhostPlan(data::ModelData &data) {
  MPI_Comm comm = util::parallel::mpiComm();
  const int size = util::parallel::mpiSize();
  const int rank = util::parallel::mpiRank();
  const size_t n_nodes = data.d_pdNodePartition.size();

  std::vector<std::vector<int>> need_from(static_cast<size_t>(size));
  for (size_t i = 0; i < n_nodes; ++i) {
    if (static_cast<int>(data.d_pdNodePartition[i]) != rank)
      continue;
    for (size_t j : data.d_neighPd[i]) {
      const int own = static_cast<int>(data.d_pdNodePartition[j]);
      if (own != rank)
        need_from[static_cast<size_t>(own)].push_back(static_cast<int>(j));
    }
  }
  for (auto &v : need_from)
    uniqueSorted(v);

  std::vector<int> sendcounts(static_cast<size_t>(size), 0);
  std::vector<int> recvcounts(static_cast<size_t>(size), 0);
  for (int r = 0; r < size; ++r)
    sendcounts[static_cast<size_t>(r)] =
        static_cast<int>(need_from[static_cast<size_t>(r)].size());
  MPI_Alltoall(sendcounts.data(), 1, MPI_INT, recvcounts.data(), 1, MPI_INT,
               comm);

  std::vector<int> sdispls(static_cast<size_t>(size), 0);
  std::vector<int> rdispls(static_cast<size_t>(size), 0);
  int send_total = 0;
  int recv_total = 0;
  for (int r = 0; r < size; ++r) {
    sdispls[static_cast<size_t>(r)] = send_total;
    rdispls[static_cast<size_t>(r)] = recv_total;
    send_total += sendcounts[static_cast<size_t>(r)];
    recv_total += recvcounts[static_cast<size_t>(r)];
  }

  std::vector<int> sendbuf(static_cast<size_t>(send_total));
  for (int r = 0; r < size; ++r) {
    const auto &v = need_from[static_cast<size_t>(r)];
    std::copy(v.begin(), v.end(),
              sendbuf.begin() + sdispls[static_cast<size_t>(r)]);
  }
  std::vector<int> recvbuf(static_cast<size_t>(recv_total));
  MPI_Alltoallv(sendbuf.data(), sendcounts.data(), sdispls.data(), MPI_INT,
                recvbuf.data(), recvcounts.data(), rdispls.data(), MPI_INT,
                comm);

  data.d_pdGhostNeedFrom = std::move(need_from);
  data.d_pdGhostServeTo.assign(static_cast<size_t>(size), {});
  for (int r = 0; r < size; ++r) {
    auto &dst = data.d_pdGhostServeTo[static_cast<size_t>(r)];
    dst.resize(static_cast<size_t>(recvcounts[static_cast<size_t>(r)]));
    for (int k = 0; k < recvcounts[static_cast<size_t>(r)]; ++k) {
      const int g = recvbuf[static_cast<size_t>(rdispls[static_cast<size_t>(r)] + k)];
      if (g < 0 || static_cast<size_t>(g) >= n_nodes)
        throw std::runtime_error("pdMpi: bad node id in ghost plan");
      if (static_cast<int>(data.d_pdNodePartition[static_cast<size_t>(g)]) !=
          rank)
        throw std::runtime_error("pdMpi: serve node not owned here");
      dst[static_cast<size_t>(k)] = g;
    }
  }

  size_t n_ghost = 0;
  for (const auto &v : data.d_pdGhostNeedFrom)
    n_ghost += v.size();
  data.setKeyData("pd_mpi_ghost_nodes", static_cast<double>(n_ghost));
}

/*! Gather owned nodal Point fields to every rank (unique Metis owners).
 * Needed for Multi_Particle DOF where contact reaches outside the PD halo. */
void syncAllOwnedPoints(data::ModelData &data, std::vector<util::Point> &field) {
  const int rank = util::parallel::mpiRank();
  const size_t n = field.size();
  std::vector<double> buf(3 * n, 0.);
  for (size_t i = 0; i < n; ++i) {
    if (static_cast<int>(data.d_pdNodePartition[i]) != rank)
      continue;
    buf[3 * i + 0] = field[i].d_x;
    buf[3 * i + 1] = field[i].d_y;
    buf[3 * i + 2] = field[i].d_z;
  }
  MPI_Allreduce(MPI_IN_PLACE, buf.data(), static_cast<int>(3 * n), MPI_DOUBLE,
                MPI_SUM, util::parallel::mpiComm());
  for (size_t i = 0; i < n; ++i)
    field[i] = util::Point(buf[3 * i], buf[3 * i + 1], buf[3 * i + 2]);
}

void exchangePoints(data::ModelData &data, std::vector<util::Point> &field) {
  MPI_Comm comm = util::parallel::mpiComm();
  const int size = util::parallel::mpiSize();

  std::vector<std::vector<double>> send(static_cast<size_t>(size));
  for (int r = 0; r < size; ++r) {
    for (int id : data.d_pdGhostServeTo[static_cast<size_t>(r)]) {
      const auto &p = field[static_cast<size_t>(id)];
      send[static_cast<size_t>(r)].push_back(p.d_x);
      send[static_cast<size_t>(r)].push_back(p.d_y);
      send[static_cast<size_t>(r)].push_back(p.d_z);
    }
  }

  std::vector<int> sendcounts(static_cast<size_t>(size), 0);
  std::vector<int> recvcounts(static_cast<size_t>(size), 0);
  for (int r = 0; r < size; ++r)
    sendcounts[static_cast<size_t>(r)] =
        static_cast<int>(send[static_cast<size_t>(r)].size());
  MPI_Alltoall(sendcounts.data(), 1, MPI_INT, recvcounts.data(), 1, MPI_INT,
               comm);

  std::vector<int> sdispls(static_cast<size_t>(size), 0);
  std::vector<int> rdispls(static_cast<size_t>(size), 0);
  int send_total = 0;
  int recv_total = 0;
  for (int r = 0; r < size; ++r) {
    sdispls[static_cast<size_t>(r)] = send_total;
    rdispls[static_cast<size_t>(r)] = recv_total;
    send_total += sendcounts[static_cast<size_t>(r)];
    recv_total += recvcounts[static_cast<size_t>(r)];
  }

  std::vector<double> sendbuf(static_cast<size_t>(send_total));
  for (int r = 0; r < size; ++r) {
    const auto &v = send[static_cast<size_t>(r)];
    std::copy(v.begin(), v.end(),
              sendbuf.begin() + sdispls[static_cast<size_t>(r)]);
  }
  std::vector<double> recvbuf(static_cast<size_t>(recv_total));
  MPI_Alltoallv(sendbuf.data(), sendcounts.data(), sdispls.data(), MPI_DOUBLE,
                recvbuf.data(), recvcounts.data(), rdispls.data(), MPI_DOUBLE,
                comm);

  for (int r = 0; r < size; ++r) {
    size_t off = static_cast<size_t>(rdispls[static_cast<size_t>(r)]);
    for (int id : data.d_pdGhostNeedFrom[static_cast<size_t>(r)]) {
      field[static_cast<size_t>(id)] =
          util::Point(recvbuf[off], recvbuf[off + 1], recvbuf[off + 2]);
      off += 3;
    }
  }
}

void exchangeDoubles(data::ModelData &data, std::vector<double> &field) {
  MPI_Comm comm = util::parallel::mpiComm();
  const int size = util::parallel::mpiSize();

  std::vector<std::vector<double>> send(static_cast<size_t>(size));
  for (int r = 0; r < size; ++r) {
    for (int id : data.d_pdGhostServeTo[static_cast<size_t>(r)])
      send[static_cast<size_t>(r)].push_back(field[static_cast<size_t>(id)]);
  }

  std::vector<int> sendcounts(static_cast<size_t>(size), 0);
  std::vector<int> recvcounts(static_cast<size_t>(size), 0);
  for (int r = 0; r < size; ++r)
    sendcounts[static_cast<size_t>(r)] =
        static_cast<int>(send[static_cast<size_t>(r)].size());
  MPI_Alltoall(sendcounts.data(), 1, MPI_INT, recvcounts.data(), 1, MPI_INT,
               comm);

  std::vector<int> sdispls(static_cast<size_t>(size), 0);
  std::vector<int> rdispls(static_cast<size_t>(size), 0);
  int send_total = 0;
  int recv_total = 0;
  for (int r = 0; r < size; ++r) {
    sdispls[static_cast<size_t>(r)] = send_total;
    rdispls[static_cast<size_t>(r)] = recv_total;
    send_total += sendcounts[static_cast<size_t>(r)];
    recv_total += recvcounts[static_cast<size_t>(r)];
  }

  std::vector<double> sendbuf(static_cast<size_t>(send_total));
  for (int r = 0; r < size; ++r) {
    const auto &v = send[static_cast<size_t>(r)];
    std::copy(v.begin(), v.end(),
              sendbuf.begin() + sdispls[static_cast<size_t>(r)]);
  }
  std::vector<double> recvbuf(static_cast<size_t>(recv_total));
  MPI_Alltoallv(sendbuf.data(), sendcounts.data(), sdispls.data(), MPI_DOUBLE,
                recvbuf.data(), recvcounts.data(), rdispls.data(), MPI_DOUBLE,
                comm);

  for (int r = 0; r < size; ++r) {
    size_t off = static_cast<size_t>(rdispls[static_cast<size_t>(r)]);
    for (int id : data.d_pdGhostNeedFrom[static_cast<size_t>(r)])
      field[static_cast<size_t>(id)] = recvbuf[off++];
  }
}

} // namespace

void pd::setupDofPartition(data::ModelData &data) {
  data.d_pdDofMpi = false;
  data.d_pdGrainAligned = false;
  data.d_pdNodePartition.clear();
  data.d_pdGhostNeedFrom.clear();
  data.d_pdGhostServeTo.clear();

  if (!util::parallel::isMpiEnabled())
    return;
  const std::string strategy = particle::resolvedMpiStrategy(data);
  if (strategy != "dof")
    return;
  const int size = util::parallel::mpiSize();
  if (size <= 1)
    return;
  if (data.d_neighPd.empty() || data.d_neighPd.size() != data.d_x.size())
    return;

  data.d_pdDofMpi = true;
  data.d_pdGrainAligned = false; // DOF-MPI = node owners, never whole-grain brick
  const int rank = util::parallel::mpiRank();
  const size_t n_nodes = data.d_x.size();

  // Partition nodes across ranks (graph partition). Walls / Multi_Particle do
  // not change the mode: DOF-MPI always distributes nodes.
  if (rank == 0) {
    mesh::metisGraphPartition("metis_kway", data.d_neighPd,
                              data.d_pdNodePartition,
                              static_cast<size_t>(size));
  } else {
    data.d_pdNodePartition.assign(n_nodes, 0);
  }
  MPI_Bcast(data.d_pdNodePartition.data(), static_cast<int>(n_nodes),
            MPI_UNSIGNED_LONG, 0, util::parallel::mpiComm());

  size_t n_owned = 0;
  for (size_t i = 0; i < n_nodes; ++i)
    if (static_cast<int>(data.d_pdNodePartition[i]) == rank)
      ++n_owned;

  buildGhostPlan(data);

  util::io::print(std::format(
      "DOF-MPI: rank {}/{} owns {}/{} nodes\n", rank, size, n_owned, n_nodes));
}

static void refreshVMagFromV(data::ModelData &data) {
  if (data.d_vMag.size() != data.d_v.size())
    data.d_vMag.resize(data.d_v.size(), 0.);
  for (size_t i = 0; i < data.d_v.size(); ++i)
    data.d_vMag[i] = data.d_v[i].length();
}

void pd::exchangeGhostDisplacement(data::ModelData &data) {
  if (!data.d_pdDofMpi)
    return;
  using clock = std::chrono::steady_clock;
  const auto t0 = clock::now();

  // DOF-MPI: each rank owns a subset of nodes. Before contact (and for PD
  // neighbors outside the local halo), every rank needs a consistent full
  // copy of u and v on Multi_Particle. Single_Particle has no inter-body
  // contact — PD halo exchange is enough.
  // Always refresh d_vMag after rewriting d_v (contact search uses vMag).
  if (data.d_input_p && data.d_input_p->isMultiParticle()) {
    syncAllOwnedPoints(data, data.d_u);
    syncAllOwnedPoints(data, data.d_v);
    for (size_t i = 0; i < data.d_x.size(); ++i)
      data.d_x[i] = data.d_xRef[i] + data.d_u[i];
    refreshVMagFromV(data);
  } else {
    exchangePoints(data, data.d_u);
    exchangePoints(data, data.d_v);
    for (int r = 0; r < util::parallel::mpiSize(); ++r) {
      for (int id : data.d_pdGhostNeedFrom[static_cast<size_t>(r)]) {
        const size_t i = static_cast<size_t>(id);
        data.d_x[i] = data.d_xRef[i] + data.d_u[i];
      }
    }
  }
  data.appendKeyData("pd_mpi_disp_exchange_time",
                     util::methods::timeDiff(t0, clock::now()));
}

void pd::exchangeGhostTheta(data::ModelData &data) {
  if (!data.d_pdDofMpi)
    return;
  if (data.d_thetaX.empty())
    return;
  using clock = std::chrono::steady_clock;
  const auto t0 = clock::now();
  if (data.d_input_p && data.d_input_p->isMultiParticle()) {
    const int rank = util::parallel::mpiRank();
    const size_t n = data.d_thetaX.size();
    std::vector<double> buf(n, 0.);
    for (size_t i = 0; i < n; ++i) {
      if (static_cast<int>(data.d_pdNodePartition[i]) == rank)
        buf[i] = data.d_thetaX[i];
    }
    MPI_Allreduce(MPI_IN_PLACE, buf.data(), static_cast<int>(n), MPI_DOUBLE,
                  MPI_SUM, util::parallel::mpiComm());
    data.d_thetaX.swap(buf);
  } else {
    exchangeDoubles(data, data.d_thetaX);
  }
  data.appendKeyData("pd_mpi_theta_exchange_time",
                     util::methods::timeDiff(t0, clock::now()));
}
