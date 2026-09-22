/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * Checks that a peridynamic material reproduces the elastic constants it was
 * calibrated from. One force evaluation per field on a uniform grid:
 *  - homogeneous strain: interior strain energy density = mu e:e + lambda/2 (tr e)^2
 *  - u_x = a x^2 / 2:  interior force density f_x = (lambda + 2 mu) a
 *  - u_y = a x^2 / 2:  interior force density f_y = mu a
 * lambda is the plane-stress value in 2D plane stress. Exits 1 if any
 * relative error exceeds -tol.
 */
#include "inp/deckIncludes.h"
#include "periDEMModel.h"
#include "util/io.h"
#include "util/parallelUtil.h"

#include <cmath>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>

namespace {

json makeDeck(const std::string &model, size_t dim, bool plane_strain,
              int influence, double L, double h, double horizon, double E,
              double nu, double Gc, const std::string &out) {
  const double rho = 1000.;
  json j;
  j["Model"] = inp::ModelDeck::getExampleJson(
      json{{"Dimension", dim}, {"Final_Time", 1.0e-6}, {"Time_Steps", 1},
           {"Particle_Sim_Type", "Single_Particle"}});
  j["Output"] = inp::OutputDeck::getExampleJson(
      json{{"Path", out}, {"Perform_Out", false}, {"Debug", 0}});
  json particle;
  particle["Sets"] = 1;
  particle["Set_1"] =
      json{{"Type", dim == 2 ? "rectangle" : "cuboid"},
           {"Parameters", {-L / 2, -L / 2, dim == 2 ? 0. : -L / 2, L / 2, L / 2,
                           dim == 2 ? 0. : L / 2}}};
  j["Particle"] = particle;
  j["Mesh"] = json{{"Sets", 1},
                   {"Set_1", json{{"File", out + "mesh.vtu"},
                                  {"CreateMesh", json{{"Flag", true},
                                                      {"Info", "uniform"},
                                                      {"Mesh_Size", h}}}}}};
  json mat = inp::ParticleDeck::getParticleMaterialExampleJson(1);
  json m{{"Type", model}, {"Is_Plane_Strain", plane_strain},
         {"Horizon", horizon}, {"Density", rho}, {"E", E}, {"Gc", Gc},
         {"Compute_From_Classical", true},
         {"Influence_Function", json{{"Type", influence}}}};
  if (model == "PDState")
    m["G"] = E / (2. * (1. + nu));
  mat["Set_1"] = inp::MaterialDeck::getExampleJson(m);
  j["Material"] = mat;
  json contact = inp::ParticleDeck::getParticleContactExampleJson(1);
  contact["Set_1_1"] = inp::ContactPairDeck::getExampleJson(
      json{{"Contact_Radius", 0.9 * h}, {"Kn", 1.0}, {"Damping_On", false},
           {"Friction_On", false}, {"Friction_Coeff", 0.}, {"K", E}});
  j["Contact"] = contact;
  j["Particle_Generation"] = json{
      {"Method", "From_File"},
      {"Data", json{{"N", 1},
                    {"0", json{{"x", 0.}, {"y", 0.}, {"z", 0.}, {"theta", 0.},
                               {"s", 1.}, {"geom_id", 0}, {"mat_id", 0},
                               {"contact_id", 0}}}}}};
  return j;
}

} // namespace

int main(int argc, char *argv[]) {
  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);
  util::parallel::initNThreads(
      input.cmdOptionExists("-nThreads")
          ? std::stoi(input.getCmdOption("-nThreads"))
          : 4);
  auto opt = [&input](const char *f, const std::string &fb) {
    return input.cmdOptionExists(f) ? input.getCmdOption(f) : fb;
  };

  const std::string model = opt("-model", "PDState");
  const size_t dim = std::stoul(opt("-dim", "2"));
  const bool plane_strain = std::stoi(opt("-planeStrain", "0")) != 0;
  const int influence = std::stoi(opt("-influence", "0"));
  const double m_ratio = std::stod(opt("-m", "4"));
  const double h = std::stod(opt("-h", dim == 2 ? "0.02" : "0.05"));
  const double tol = std::stod(opt("-tol", "0.02"));
  const double L = std::stod(opt("-L", "1.0")), horizon = m_ratio * h;
  const double E = 1.0e5, Gc = 10.;
  // bond-based models fix nu by the plane mode; PDState takes the table nu
  double nu = std::stod(opt("-nu", "0.3"));
  if (model != "PDState")
    nu = (dim == 2 && !plane_strain) ? 1. / 3. : 0.25;

  const double mu = E / (2. * (1. + nu));
  const double lambda = (dim == 2 && !plane_strain)
                            ? E * nu / (1. - nu * nu)
                            : E * nu / ((1. + nu) * (1. - 2. * nu));

  namespace fs = std::filesystem;
  struct Check { std::string name; double got, want; };

  // build the model, apply each field, and compare with linear elasticity
  auto runChecks = [&](const std::string &mname) {
    const fs::path out = fs::current_path() /
        std::format("calib_{}_{}d_{}_J{}", mname, dim,
                    plane_strain ? "pstrain" : "pstress", influence);
    fs::create_directories(out);
    auto deck = std::make_shared<inp::Input>(
        makeDeck(mname, dim, plane_strain, influence, L, h, horizon, E, nu, Gc,
                 out.string() + "/"));
    PeriDEMModel dem(deck);
    dem.init();

    // energy at x needs a full horizon; the state-based force at x also uses
    // the dilatation of neighbors, so it needs two
    auto inside = [&](const util::Point &x, double margin) {
      const double c = 0.5 * L - margin - 1.5 * h;
      return std::abs(x.d_x) < c && std::abs(x.d_y) < c &&
             (dim == 2 || std::abs(x.d_z) < c);
    };

    // apply a displacement field, compute forces, and return interior
    // averages of energy density, f_x and f_y
    auto evaluate =
        [&](const std::function<util::Point(const util::Point &)> &u) {
      for (size_t i = 0; i < dem.d_xRef.size(); i++) {
        dem.d_u[i] = u(dem.d_xRef[i]);
        dem.d_x[i] = dem.d_xRef[i] + dem.d_u[i];
      }
      dem.computeForces();
      double e = 0., fx = 0., fy = 0.;
      size_t ne = 0, nf = 0;
      for (size_t i = 0; i < dem.d_xRef.size(); i++) {
        if (inside(dem.d_xRef[i], horizon)) {
          e += dem.d_e[i];
          ne++;
        }
        if (inside(dem.d_xRef[i], 2. * horizon)) {
          fx += dem.d_f[i].d_x;
          fy += dem.d_f[i].d_y;
          nf++;
        }
      }
      return std::array<double, 3>{e / ne, fx / nf, fy / nf};
    };

    const double eps = 1.0e-5, a = 1.0e-5 / L;
    std::vector<Check> checks;

    // homogeneous strains: uniaxial, equibiaxial, shear
    auto r = evaluate([&](const util::Point &x) {
      return util::Point(eps * x.d_x, 0., 0.); });
    checks.push_back({"energy uniaxial strain", r[0],
                      0.5 * (lambda + 2. * mu) * eps * eps});
    r = evaluate([&](const util::Point &x) {
      return util::Point(eps * x.d_x, eps * x.d_y, dim == 3 ? eps * x.d_z : 0.); });
    checks.push_back({"energy equibiaxial", r[0],
                      (mu * dim + 0.5 * lambda * dim * dim) * eps * eps});
    r = evaluate([&](const util::Point &x) {
      return util::Point(eps * x.d_y, eps * x.d_x, 0.); });
    checks.push_back({"energy shear", r[0], 2. * mu * eps * eps});

    // quadratic fields: divergence of stress
    r = evaluate([&](const util::Point &x) {
      return util::Point(0.5 * a * x.d_x * x.d_x, 0., 0.); });
    checks.push_back({"force (lambda+2mu)", r[1], (lambda + 2. * mu) * a});
    r = evaluate([&](const util::Point &x) {
      return util::Point(0., 0.5 * a * x.d_x * x.d_x, 0.); });
    checks.push_back({"force mu", r[2], mu * a});

    util::io::print(std::format(
        "CALIB model={} dim={} {} J={} delta/h={} nodes={} E={} nu={:.4f}\n",
        mname, dim, plane_strain ? "plane_strain" : "plane_stress", influence,
        m_ratio, dem.d_xRef.size(), E, nu));
    return checks;
  };

  // -compareModel: a second bond-based model must give the same small-strain
  // response (e.g. RNP against PMB), which removes the quadrature error
  std::vector<Check> checks;
  try {
    checks = runChecks(model);
    if (input.cmdOptionExists("-compareModel")) {
      const auto ref = runChecks(input.getCmdOption("-compareModel"));
      for (size_t k = 0; k < checks.size(); k++)
        checks[k].want = ref[k].got;
    }
  } catch (const std::exception &e) {
    // input checks are tested by matching this message
    util::io::print(std::format("CALIB_ERROR {}\n", e.what()));
    util::parallel::finalizeMpi();
    return 1;
  }

  bool ok = true;
  for (const auto &c : checks) {
    const double rel = (c.got - c.want) / c.want;
    const bool pass = std::abs(rel) <= tol;
    ok = ok && pass;
    util::io::print(std::format("  {:<24} got={:+.15e} want={:+.6e} rel={:+.4f} {}\n",
                                c.name, c.got, c.want, rel, pass ? "OK" : "FAIL"));
  }
  util::io::print(ok ? "CALIB_PASS\n" : "CALIB_FAIL\n");
  util::parallel::finalizeMpi();
  return ok ? 0 : 1;
}
