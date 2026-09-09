/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "pdMpi.h"

#include "data/modelData.h"
#include "mesh/meshPartitioning.h"
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
  data.d_pdNodePartition.clear();
  data.d_pdGhostNeedFrom.clear();
  data.d_pdGhostServeTo.clear();

  if (!util::parallel::isMpiEnabled())
    return;
  if (data.d_input_p && data.d_input_p->isMultiParticle())
    return;
  const int size = util::parallel::mpiSize();
  if (size <= 1)
    return;
  if (data.d_neighPd.empty() || data.d_neighPd.size() != data.d_x.size())
    return;

  data.d_pdDofMpi = true;
  const int rank = util::parallel::mpiRank();
  const size_t n_nodes = data.d_x.size();

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
      "PD DOF-MPI: rank {}/{} owns {}/{} nodes (metis_kway)\n", rank, size,
      n_owned, n_nodes));
}

void pd::exchangeGhostDisplacement(data::ModelData &data) {
  if (!data.d_pdDofMpi)
    return;
  using clock = std::chrono::steady_clock;
  const auto t0 = clock::now();
  exchangePoints(data, data.d_u);
  // Keep current configuration consistent for any x-based reads.
  for (int r = 0; r < util::parallel::mpiSize(); ++r) {
    for (int id : data.d_pdGhostNeedFrom[static_cast<size_t>(r)]) {
      const size_t i = static_cast<size_t>(id);
      data.d_x[i] = data.d_xRef[i] + data.d_u[i];
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
  exchangeDoubles(data, data.d_thetaX);
  data.appendKeyData("pd_mpi_theta_exchange_time",
                     util::methods::timeDiff(t0, clock::now()));
}
