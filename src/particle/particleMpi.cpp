/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "particleMpi.h"

#include "baseParticle.h"
#include "data/modelData.h"
#include "inp/input.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "util/point.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <limits>
#include <stdexcept>
#include <vector>

bool particle::isLocallyOwned(const BaseParticle &p) {
  if (!util::parallel::isMpiEnabled())
    return true;
  if (p.isWall())
    return true;
  return p.d_mpiOwner == util::parallel::mpiRank();
}

std::string particle::resolvedMpiStrategy(const data::ModelData &data) {
  std::string s = "auto";
  if (data.d_input_p && data.d_input_p->d_modelDeck_p)
    s = data.d_input_p->d_modelDeck_p->d_mpiStrategy;
  if (s.empty())
    s = "auto";
  if (s == "auto") {
    if (data.d_input_p && data.d_input_p->isMultiParticle())
      return "particle";
    return "dof";
  }
  return s;
}

namespace {

void factor2d(int nproc, double Lx, double Ly, int &nx, int &ny) {
  // nx*ny = nproc; prefer subdomain aspect close to domain aspect.
  nx = 1;
  ny = nproc;
  double best = std::numeric_limits<double>::max();
  for (int i = 1; i <= nproc; ++i) {
    if (nproc % i != 0)
      continue;
    const int j = nproc / i;
    const double ax = Lx / static_cast<double>(i);
    const double ay = Ly / static_cast<double>(j);
    const double score = std::abs(ax - ay);
    if (score < best) {
      best = score;
      nx = i;
      ny = j;
    }
  }
}

void packGrainKinematics(const data::ModelData &data,
                         const particle::BaseParticle &p,
                         std::vector<double> &buf) {
  const size_t n = p.getNumNodes();
  const size_t base = buf.size();
  buf.resize(base + 9 * n);
  for (size_t i = 0; i < n; ++i) {
    const size_t g = p.getNodeId(i);
    const auto &x = data.d_x[g];
    const auto &u = data.d_u[g];
    const auto &v = data.d_v[g];
    const size_t o = base + 9 * i;
    buf[o + 0] = x.d_x;
    buf[o + 1] = x.d_y;
    buf[o + 2] = x.d_z;
    buf[o + 3] = u.d_x;
    buf[o + 4] = u.d_y;
    buf[o + 5] = u.d_z;
    buf[o + 6] = v.d_x;
    buf[o + 7] = v.d_y;
    buf[o + 8] = v.d_z;
  }
}

size_t unpackGrainKinematics(data::ModelData &data, particle::BaseParticle &p,
                             const std::vector<double> &buf, size_t offset) {
  const size_t n = p.getNumNodes();
  for (size_t i = 0; i < n; ++i) {
    const size_t g = p.getNodeId(i);
    const size_t o = offset + 9 * i;
    data.d_x[g] = util::Point(buf[o + 0], buf[o + 1], buf[o + 2]);
    data.d_u[g] = util::Point(buf[o + 3], buf[o + 4], buf[o + 5]);
    data.d_v[g] = util::Point(buf[o + 6], buf[o + 7], buf[o + 8]);
    data.d_vMag[g] = data.d_v[g].length();
  }
  return offset + 9 * n;
}

size_t ghostRebuildInterval(const data::ModelData &data) {
  size_t interval = data.d_contNeighUpdateInterval;
  if (interval == 0 && data.d_particleDeck_p)
    interval = data.d_particleDeck_p->d_pNeighDeck.d_neighUpdateInterval;
  if (interval == 0)
    interval = 1;
  return interval;
}

void rebuildGhostPlan(data::ModelData &data) {
  MPI_Comm comm = util::parallel::mpiComm();
  const int size = util::parallel::mpiSize();
  const int rank = util::parallel::mpiRank();
  const auto &grains = data.d_particlesListTypeParticle;
  const size_t n_grains = grains.size();

  std::vector<double> local_c(4 * n_grains, 0.);
  for (size_t g = 0; g < n_grains; ++g) {
    const auto c = grains[g]->getXCenter();
    local_c[4 * g + 0] = c.d_x;
    local_c[4 * g + 1] = c.d_y;
    local_c[4 * g + 2] = c.d_z;
    local_c[4 * g + 3] = grains[g]->d_geom_p->boundingRadius();
  }
  std::vector<double> all_c(4 * n_grains * static_cast<size_t>(size), 0.);
  MPI_Allgather(local_c.data(), static_cast<int>(local_c.size()), MPI_DOUBLE,
                all_c.data(), static_cast<int>(local_c.size()), MPI_DOUBLE,
                comm);

  std::vector<util::Point> centers(n_grains);
  std::vector<double> radii(n_grains);
  for (size_t g = 0; g < n_grains; ++g) {
    const int own = grains[g]->d_mpiOwner;
    const size_t base = static_cast<size_t>(own) * 4 * n_grains + 4 * g;
    centers[g] = util::Point(all_c[base + 0], all_c[base + 1], all_c[base + 2]);
    radii[g] = all_c[base + 3];
  }

  data.d_mpiIncludeInContactCloud.assign(data.d_particlesListTypeAll.size(), 0);
  for (auto *p : data.d_particlesListTypeAll) {
    if (p->isWall() || particle::isLocallyOwned(*p))
      data.d_mpiIncludeInContactCloud[p->getId()] = 1;
  }

  // Contact search radius already carries sFactor skin when interval > 1;
  // add a small extra buffer for MPI plan lifetime between rebuilds.
  const double extra =
      std::max(0.25 * data.d_maxContactR, std::max(data.d_hMax, 1.0e-16));
  const double search_r =
      (data.d_contNeighSearchRadius > 1.0e-16)
          ? data.d_contNeighSearchRadius
          : data.d_maxContactR;
  const double cutoff = search_r + extra;

  data.d_mpiGhostNeedFrom.assign(static_cast<size_t>(size), {});
  size_t n_ghost = 0;
  auto add_ghost = [&](size_t g) {
    auto *pj = grains[g];
    if (particle::isLocallyOwned(*pj))
      return;
    const int own = pj->d_mpiOwner;
    if (own < 0 || own == rank)
      return;
    auto &vec = data.d_mpiGhostNeedFrom[static_cast<size_t>(own)];
    const int gid = static_cast<int>(g);
    if (std::find(vec.begin(), vec.end(), gid) != vec.end())
      return;
    vec.push_back(gid);
    data.d_mpiIncludeInContactCloud[pj->getId()] = 1;
    ++n_ghost;
  };

  for (size_t g = 0; g < n_grains; ++g) {
    bool near = false;
    for (size_t i = 0; i < n_grains && !near; ++i) {
      if (!particle::isLocallyOwned(*grains[i]))
        continue;
      const double lim = radii[i] + radii[g] + cutoff;
      if (centers[i].dist(centers[g]) < lim)
        near = true;
    }
    if (near)
      add_ghost(g);
  }

  // Wall contact is assembled on ranks that own wall nodes (rank 0 for
  // particle-MPI). Ghost any grain near a wall so plate/cup searches see them.
  for (auto *w : data.d_particlesListTypeWall) {
    if (!w)
      continue;
    const auto wc = w->getXCenter();
    const double wr = w->d_geom_p->boundingRadius();
    for (size_t g = 0; g < n_grains; ++g) {
      const double lim = wr + radii[g] + cutoff;
      if (wc.dist(centers[g]) < lim)
        add_ghost(g);
    }
  }

  std::vector<int> req_sendcounts(static_cast<size_t>(size), 0);
  std::vector<int> req_recvcounts(static_cast<size_t>(size), 0);
  for (int r = 0; r < size; ++r)
    req_sendcounts[static_cast<size_t>(r)] =
        static_cast<int>(data.d_mpiGhostNeedFrom[static_cast<size_t>(r)].size());
  MPI_Alltoall(req_sendcounts.data(), 1, MPI_INT, req_recvcounts.data(), 1,
               MPI_INT, comm);

  std::vector<int> req_sdispls(static_cast<size_t>(size), 0);
  std::vector<int> req_rdispls(static_cast<size_t>(size), 0);
  int req_send_total = 0;
  int req_recv_total = 0;
  for (int r = 0; r < size; ++r) {
    req_sdispls[static_cast<size_t>(r)] = req_send_total;
    req_rdispls[static_cast<size_t>(r)] = req_recv_total;
    req_send_total += req_sendcounts[static_cast<size_t>(r)];
    req_recv_total += req_recvcounts[static_cast<size_t>(r)];
  }

  std::vector<int> req_sendbuf(static_cast<size_t>(req_send_total));
  for (int r = 0; r < size; ++r) {
    const auto &v = data.d_mpiGhostNeedFrom[static_cast<size_t>(r)];
    std::copy(v.begin(), v.end(),
              req_sendbuf.begin() + req_sdispls[static_cast<size_t>(r)]);
  }
  std::vector<int> req_recvbuf(static_cast<size_t>(req_recv_total));
  MPI_Alltoallv(req_sendbuf.data(), req_sendcounts.data(), req_sdispls.data(),
                MPI_INT, req_recvbuf.data(), req_recvcounts.data(),
                req_rdispls.data(), MPI_INT, comm);

  data.d_mpiGhostServeTo.assign(static_cast<size_t>(size), {});
  for (int r = 0; r < size; ++r) {
    const int off = req_rdispls[static_cast<size_t>(r)];
    const int nreq = req_recvcounts[static_cast<size_t>(r)];
    auto &dst = data.d_mpiGhostServeTo[static_cast<size_t>(r)];
    dst.resize(static_cast<size_t>(nreq));
    for (int k = 0; k < nreq; ++k) {
      const int g = req_recvbuf[static_cast<size_t>(off + k)];
      if (g < 0 || static_cast<size_t>(g) >= n_grains)
        throw std::runtime_error("rebuildGhostPlan: bad grain id");
      if (grains[static_cast<size_t>(g)]->d_mpiOwner != rank)
        throw std::runtime_error(
            "rebuildGhostPlan: requested grain not owned here");
      dst[static_cast<size_t>(k)] = g;
    }
  }

  data.d_mpiGhostPlanValid = true;
  data.d_mpiGhostStepsSinceRebuild = 0;
  data.setKeyData("mpi_ghost_grain_count", static_cast<double>(n_ghost));
}

void exchangeCachedKinematics(data::ModelData &data) {
  MPI_Comm comm = util::parallel::mpiComm();
  const int size = util::parallel::mpiSize();
  const auto &grains = data.d_particlesListTypeParticle;

  std::vector<std::vector<double>> kin_send(static_cast<size_t>(size));
  for (int r = 0; r < size; ++r) {
    for (int g : data.d_mpiGhostServeTo[static_cast<size_t>(r)])
      packGrainKinematics(data, *grains[static_cast<size_t>(g)],
                          kin_send[static_cast<size_t>(r)]);
  }

  std::vector<int> kin_sendcounts(static_cast<size_t>(size), 0);
  std::vector<int> kin_recvcounts(static_cast<size_t>(size), 0);
  for (int r = 0; r < size; ++r)
    kin_sendcounts[static_cast<size_t>(r)] =
        static_cast<int>(kin_send[static_cast<size_t>(r)].size());
  MPI_Alltoall(kin_sendcounts.data(), 1, MPI_INT, kin_recvcounts.data(), 1,
               MPI_INT, comm);

  std::vector<int> kin_sdispls(static_cast<size_t>(size), 0);
  std::vector<int> kin_rdispls(static_cast<size_t>(size), 0);
  int kin_send_total = 0;
  int kin_recv_total = 0;
  for (int r = 0; r < size; ++r) {
    kin_sdispls[static_cast<size_t>(r)] = kin_send_total;
    kin_rdispls[static_cast<size_t>(r)] = kin_recv_total;
    kin_send_total += kin_sendcounts[static_cast<size_t>(r)];
    kin_recv_total += kin_recvcounts[static_cast<size_t>(r)];
  }

  std::vector<double> kin_sendbuf(static_cast<size_t>(kin_send_total));
  for (int r = 0; r < size; ++r) {
    const auto &v = kin_send[static_cast<size_t>(r)];
    std::copy(v.begin(), v.end(),
              kin_sendbuf.begin() + kin_sdispls[static_cast<size_t>(r)]);
  }
  std::vector<double> kin_recvbuf(static_cast<size_t>(kin_recv_total));
  MPI_Alltoallv(kin_sendbuf.data(), kin_sendcounts.data(), kin_sdispls.data(),
                MPI_DOUBLE, kin_recvbuf.data(), kin_recvcounts.data(),
                kin_rdispls.data(), MPI_DOUBLE, comm);

  for (int r = 0; r < size; ++r) {
    size_t off = static_cast<size_t>(kin_rdispls[static_cast<size_t>(r)]);
    for (int g : data.d_mpiGhostNeedFrom[static_cast<size_t>(r)])
      off = unpackGrainKinematics(data, *grains[static_cast<size_t>(g)],
                                  kin_recvbuf, off);
  }
}

} // namespace

void particle::assignMpiOwners(data::ModelData &data) {
  const int size = util::parallel::mpiSize();
  const int rank = util::parallel::mpiRank();
  const auto &grains = data.d_particlesListTypeParticle;
  const std::string strategy = resolvedMpiStrategy(data);

  for (auto *p : data.d_particlesListTypeAll) {
    if (p->isWall())
      p->d_mpiOwner = -1;
  }

  // Particle-MPI only: spatial brick over grain centers.
  // DOF-MPI assigns nodes in pd::setupDofPartition; do not also brick grains.
  const bool use_particle_partition =
      strategy == "particle" && size > 1 && !grains.empty();

  if (!use_particle_partition) {
    for (auto *p : grains)
      p->d_mpiOwner = 0;
    if (util::parallel::isMpiEnabled() && rank == 0)
      util::io::print(std::format(
          "MPI strategy={}: grain ownership inactive (all grains → rank 0)\n",
          strategy));
  } else {
    double xmin = std::numeric_limits<double>::max();
    double xmax = -std::numeric_limits<double>::max();
    double ymin = std::numeric_limits<double>::max();
    double ymax = -std::numeric_limits<double>::max();
    for (auto *p : grains) {
      const auto &c = p->getXCenter();
      xmin = std::min(xmin, c.d_x);
      xmax = std::max(xmax, c.d_x);
      ymin = std::min(ymin, c.d_y);
      ymax = std::max(ymax, c.d_y);
    }
    const double Lx = std::max(xmax - xmin, 1.0e-16);
    const double Ly = std::max(ymax - ymin, 1.0e-16);
    int nx = 1, ny = size;
    factor2d(size, Lx, Ly, nx, ny);

    const double eps = 1.0e-14 * std::max(Lx, Ly);
    for (auto *p : grains) {
      const auto &c = p->getXCenter();
      int ix = static_cast<int>((c.d_x - xmin) / Lx * nx);
      int iy = static_cast<int>((c.d_y - ymin) / Ly * ny);
      if (ix < 0)
        ix = 0;
      if (iy < 0)
        iy = 0;
      if (ix >= nx)
        ix = nx - 1;
      if (iy >= ny)
        iy = ny - 1;
      // nudge points on the max edge into the last cell
      (void)eps;
      p->d_mpiOwner = iy * nx + ix;
    }

    if (util::parallel::isMpiEnabled() && rank == 0)
      util::io::print(std::format(
          "MPI spatial owners: {} ranks as {}x{} brick over [{:.3g},{:.3g}] x "
          "[{:.3g},{:.3g}]\n",
          size, nx, ny, xmin, xmax, ymin, ymax));
  }

  data.d_mpiIncludeInContactCloud.assign(data.d_particlesListTypeAll.size(), 1);
  data.d_mpiGhostPlanValid = false;
  data.d_mpiGhostStepsSinceRebuild = 0;
  data.d_mpiGhostNeedFrom.clear();
  data.d_mpiGhostServeTo.clear();

  if (util::parallel::isMpiEnabled()) {
    size_t n_local = 0;
    for (auto *p : grains)
      if (isLocallyOwned(*p))
        ++n_local;
    util::io::print(std::format(
        "MPI particle owners: rank {}/{} owns {}/{} grains (walls replicated)\n",
        rank, size, n_local, grains.size()));
  }
}

void particle::exchangeGhostKinematics(data::ModelData &data) {
  if (!util::parallel::isMpiEnabled()) {
    data.d_mpiIncludeInContactCloud.assign(data.d_particlesListTypeAll.size(),
                                           1);
    return;
  }

  // Particle-MPI only. DOF-MPI owns nodes, not whole grains; grain packing
  // from the "grain owner" rank would overwrite remote-owned nodal u/v with
  // stale zeros. Nodal sync lives in pd::exchangeGhostDisplacement.
  if (data.d_pdDofMpi || resolvedMpiStrategy(data) == "dof") {
    data.d_mpiIncludeInContactCloud.assign(data.d_particlesListTypeAll.size(),
                                           1);
    return;
  }

  using clock = std::chrono::steady_clock;
  const auto t0 = clock::now();

  const size_t interval = ghostRebuildInterval(data);
  int local_need =
      (!data.d_mpiGhostPlanValid ||
       data.d_mpiGhostStepsSinceRebuild >= interval)
          ? 1
          : 0;
  // All ranks must take the same branch: rebuild uses Allgather, exchange uses
  // Alltoall — disagreeing on rebuild deadlocks (seen past ~50% with contact).
  int global_need = local_need;
  MPI_Allreduce(MPI_IN_PLACE, &global_need, 1, MPI_INT, MPI_MAX,
                util::parallel::mpiComm());
  const bool need_rebuild = global_need != 0;

  auto t_rebuild0 = t0;
  auto t_rebuild1 = t0;
  if (need_rebuild) {
    t_rebuild0 = clock::now();
    rebuildGhostPlan(data);
    t_rebuild1 = clock::now();
  }

  const auto t_ex0 = clock::now();
  exchangeCachedKinematics(data);
  const auto t1 = clock::now();

  ++data.d_mpiGhostStepsSinceRebuild;

  data.appendKeyData("mpi_ghost_select_time",
                     util::methods::timeDiff(t_rebuild0, t_rebuild1));
  data.appendKeyData("mpi_halo_exchange_time",
                     util::methods::timeDiff(t_ex0, t1));
  data.appendKeyData("mpi_exchange_time", util::methods::timeDiff(t0, t1));
  data.setKeyData("mpi_ghost_rebuild", need_rebuild ? 1.0 : 0.0);
}
