/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "inp/input.h"
#include "periDEMModel.h"
#include "util/json.h"
#include "util/parallelUtil.h"
#include "util/point.h"

#include <PeriDEMConfig.h>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace nb = nanobind;

namespace {

class CwdGuard {
public:
  explicit CwdGuard(const std::filesystem::path &dir)
      : old_(std::filesystem::current_path()) {
    if (!dir.empty())
      std::filesystem::current_path(dir);
  }
  ~CwdGuard() {
    std::error_code ec;
    std::filesystem::current_path(old_, ec);
  }

  CwdGuard(const CwdGuard &) = delete;
  CwdGuard &operator=(const CwdGuard &) = delete;

private:
  std::filesystem::path old_;
};

nb::ndarray<nb::numpy, double, nb::ndim<2>>
copy_points(const std::vector<util::Point> &pts) {
  const size_t n = pts.size();
  auto *data = new double[n * 3];
  for (size_t i = 0; i < n; ++i) {
    data[3 * i] = pts[i].d_x;
    data[3 * i + 1] = pts[i].d_y;
    data[3 * i + 2] = pts[i].d_z;
  }
  nb::capsule owner(data, [](void *p) noexcept {
    delete[] static_cast<double *>(p);
  });
  return nb::ndarray<nb::numpy, double, nb::ndim<2>>(data, {n, 3}, owner);
}

nb::ndarray<nb::numpy, double, nb::ndim<1>>
copy_scalars_float(const std::vector<float> &v) {
  const size_t n = v.size();
  auto *data = new double[n];
  for (size_t i = 0; i < n; ++i)
    data[i] = static_cast<double>(v[i]);
  nb::capsule owner(data, [](void *p) noexcept {
    delete[] static_cast<double *>(p);
  });
  return nb::ndarray<nb::numpy, double, nb::ndim<1>>(data, {n}, owner);
}

void ensure_runtime(unsigned n_threads) {
  int argc = 0;
  char **argv = nullptr;
  util::parallel::initMpi(argc, argv);
  if (n_threads == 0)
    n_threads = std::max(1u, std::thread::hardware_concurrency());
  util::parallel::initNThreads(n_threads);
}

class Simulation {
public:
  Simulation(std::shared_ptr<inp::Input> deck, std::filesystem::path workdir)
      : deck_(std::move(deck)), workdir_(std::move(workdir)) {
    CwdGuard g(workdir_);
    model_ = std::make_unique<PeriDEMModel>(deck_);
  }

  static Simulation from_json(const std::string &text) {
    auto deck = std::make_shared<inp::Input>(json::parse(text));
    if (!deck->isPeriDEM())
      throw std::runtime_error("deck is not a PeriDEM model");
    return Simulation(deck, {});
  }

  static Simulation from_file(const std::string &path) {
    namespace fs = std::filesystem;
    const fs::path p = fs::absolute(path);
    if (!fs::exists(p))
      throw std::runtime_error("input file does not exist: " + path);
    std::ifstream f(p);
    auto j = json::parse(f);
    auto deck = std::make_shared<inp::Input>(j);
    if (!deck->isPeriDEM())
      throw std::runtime_error("deck is not a PeriDEM model");
    return Simulation(deck, p.parent_path());
  }

  void setup() {
    CwdGuard g(workdir_);
    model_->init();
  }

  void run() {
    CwdGuard g(workdir_);
    model_->run(deck_);
  }

  void step() {
    CwdGuard g(workdir_);
    model_->integrateStep();
  }

  size_t n_nodes() const { return model_->d_u.size(); }

  size_t step_index() const { return model_->d_n; }

  double time() const { return model_->d_time; }

  nb::ndarray<nb::numpy, double, nb::ndim<2>> reference() const {
    return copy_points(model_->d_xRef);
  }

  nb::ndarray<nb::numpy, double, nb::ndim<2>> position() const {
    return copy_points(model_->d_x);
  }

  nb::ndarray<nb::numpy, double, nb::ndim<2>> displacement() const {
    return copy_points(model_->d_u);
  }

  nb::ndarray<nb::numpy, double, nb::ndim<2>> velocity() const {
    return copy_points(model_->d_v);
  }

  nb::ndarray<nb::numpy, double, nb::ndim<2>> force() const {
    return copy_points(model_->d_f);
  }

  nb::ndarray<nb::numpy, double, nb::ndim<1>> damage() const {
    return copy_scalars_float(model_->d_Z);
  }

private:
  std::shared_ptr<inp::Input> deck_;
  std::filesystem::path workdir_;
  std::unique_ptr<PeriDEMModel> model_;
};

} // namespace

NB_MODULE(_core, m) {
  m.doc() = "PeriDEM Python bindings";
  m.def(
      "init",
      [](unsigned n_threads) { ensure_runtime(n_threads); },
      nb::arg("n_threads") = 0u,
      "Initialize MPI (if needed) and thread count. n_threads=0 uses hardware.");
  m.def("mpi_rank", []() { return util::parallel::mpiRank(); });
  m.def("mpi_size", []() { return util::parallel::mpiSize(); });
  m.def("finalize", []() { util::parallel::finalizeMpi(); },
        "Call MPI_Finalize if MPI was initialized.");
  m.def("version", []() {
    return std::to_string(MAJOR_VERSION) + "." + std::to_string(MINOR_VERSION) +
           "." + std::to_string(UPDATE_VERSION);
  });

  nb::class_<Simulation>(m, "Simulation")
      .def_static("from_json", &Simulation::from_json)
      .def_static("from_file", &Simulation::from_file)
      .def("setup", &Simulation::setup)
      .def("run", &Simulation::run)
      .def("step", &Simulation::step)
      .def_prop_ro("n_nodes", &Simulation::n_nodes)
      .def_prop_ro("step_index", &Simulation::step_index)
      .def_prop_ro("time", &Simulation::time)
      .def_prop_ro("reference", &Simulation::reference, nb::rv_policy::automatic)
      .def_prop_ro("position", &Simulation::position, nb::rv_policy::automatic)
      .def_prop_ro("displacement", &Simulation::displacement,
                   nb::rv_policy::automatic)
      .def_prop_ro("velocity", &Simulation::velocity, nb::rv_policy::automatic)
      .def_prop_ro("force", &Simulation::force, nb::rv_policy::automatic)
      .def_prop_ro("damage", &Simulation::damage, nb::rv_policy::automatic);
}
