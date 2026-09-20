/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * Single-particle Mode-I open then close across a through precrack.
 * Compares Model.Self_Contact broken_bond_kn vs reference_gap under the same
 * prescribed kinematics (r0 ≠ Rc for bonds that crossed the crack).
 */

#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "periDEMModel.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "util/point.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string directoryPathWithTrailingSep(const std::filesystem::path &dir) {
  namespace fs = std::filesystem;
  fs::path n = fs::absolute(dir).lexically_normal();
  std::string s = n.string();
  if (!s.empty() && s.back() != '/' && s.back() != '\\')
    s += fs::path::preferred_separator;
  return s;
}

json buildInputJson(const std::string &output_path, const std::filesystem::path &mesh_file,
                    double L, double mesh_size, double horizon, double final_time,
                    size_t num_steps, const std::string &self_contact) {
  auto model = inp::ModelDeck::getExampleJson(
      json{{"Dimension", 2},
           {"Final_Time", final_time},
           {"Time_Steps", num_steps},
           {"Discretization_Type", json{{"Spatial", "finite_difference"}, {"Time", "central_difference"}}},
           {"Populate_ElementNodeConnectivity", true},
           {"Quad_Approximation_Order", 2},
           {"Particle_Sim_Type", "Single_Particle"},
           {"Seed", 0}});
  model["Self_Contact"] = self_contact;
  model["Bond_Break"] = "tension";

  // Soft PD; large Gc so only the precrack is fractured.
  const double rho = 1200.0;
  const double K = 2.16e5;
  const double G = 1.296e5;
  const double Gc = 5.0e6;

  auto output = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", output_path},
           {"Tags", std::vector<std::string>({"Displacement", "Force", "Damage_Z", "Particle_ID"})},
           {"Output_Interval", num_steps},
           {"Debug", 1},
           {"Perform_FE_Out", false},
           {"Compress_Type", "zlib"},
           {"Perform_Out", false},
           {"Test_Output_Interval", num_steps},
           {"Tag_PP", ""},
           {"PVD_Collection", false}});

  const double strip = 0.2 * L;
  json bc = json::object();
  bc["Displacement_BC"] = {
      {"Sets", 2},
      {"Set_1",
       {{"Region",
         {{"Geometry",
           {{"Type", "rectangle"},
            {"Parameters", std::vector<double>{-0.05 * L, -0.05 * L, 0., strip, L + 0.05 * L, 0.}}}}}},
        {"Direction", std::vector<size_t>{1, 2}},
        {"Time_Function", {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
        {"Spatial_Function", {{"Type", "constant"}}},
        {"Zero_Displacement", true}}},
      {"Set_2",
       {{"Region",
         {{"Geometry",
           {{"Type", "rectangle"},
            {"Parameters",
             std::vector<double>{L - strip, -0.05 * L, 0., L + 0.05 * L, L + 0.05 * L, 0.}}}}}},
        {"Direction", std::vector<size_t>{1}},
        {"Time_Function", {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
        {"Spatial_Function", {{"Type", "constant"}}},
        {"Zero_Displacement", false}}}};

  geom::GeomData rect;
  rect.d_geomName = "rectangle";
  rect.d_geomParams = {0., 0., 0., L, L, 0.};
  auto particle = inp::ParticleDeck::getParticleGeomExampleJson({rect});

  json mesh_set = {{"File", mesh_file.string()},
                   {"CreateMesh",
                    {{"Flag", true},
                     {"Info", "gmsh_builtin_mesh"},
                     {"Mesh_Size", mesh_size},
                     {"Write_Mesh_File", true}}}};
  json mesh = {{"Set_1", mesh_set}};

  auto material = inp::ParticleDeck::getParticleMaterialExampleJson(1);
  material["Set_1"] =
      inp::MaterialDeck::getExampleJson(
      json{{"Type", "PDState"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", Gc},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 1}}}});

  json root = {{"Model", model},
               {"Output", output},
               {"Particle", particle},
               {"Mesh", mesh},
               {"Material", material}};
  root.merge_patch(bc);
  return root;
}

size_t applyPrecrack(PeriDEMModel &dem, double x_crack) {
  size_t n_broken = 0;
  for (size_t i = 0; i < dem.d_neighPd.size(); ++i) {
    const auto &xi = dem.d_xRef[i];
    for (size_t k = 0; k < dem.d_neighPd[i].size(); ++k) {
      const size_t j = dem.d_neighPd[i][k];
      const auto &xj = dem.d_xRef[j];
      if ((xi.d_x - x_crack) * (xj.d_x - x_crack) < 0.) {
        dem.d_fracture_p->setBondState(i, k, true);
        ++n_broken;
      }
    }
  }
  return n_broken;
}

void applyRigidOpenClose(PeriDEMModel &dem, double x_crack, double u_right) {
  for (size_t i = 0; i < dem.d_xRef.size(); ++i) {
    const double ux = (dem.d_xRef[i].d_x >= x_crack) ? u_right : 0.;
    dem.d_u[i] = util::Point(ux, 0., 0.);
    dem.d_v[i] = util::Point();
    dem.d_x[i] = dem.d_xRef[i] + dem.d_u[i];
  }
}

struct CrackMetrics {
  double min_gap = 1.0e300;
  double max_gap = 0.;
  size_t n_pairs = 0;
  double force_x_left = 0.;
  double force_x_right = 0.;
};

CrackMetrics measure(PeriDEMModel &dem, double x_crack, double band) {
  CrackMetrics m;
  for (size_t i = 0; i < dem.d_neighPd.size(); ++i) {
    const auto &xi = dem.d_xRef[i];
    for (size_t k = 0; k < dem.d_neighPd[i].size(); ++k) {
      if (!dem.d_fracture_p->getBondState(i, k))
        continue;
      const size_t j = dem.d_neighPd[i][k];
      const auto &xj = dem.d_xRef[j];
      if ((xi.d_x - x_crack) * (xj.d_x - x_crack) >= 0.)
        continue;
      const double gap = (dem.d_x[j] - dem.d_x[i]).length();
      m.min_gap = std::min(m.min_gap, gap);
      m.max_gap = std::max(m.max_gap, gap);
      ++m.n_pairs;
    }
  }
  if (m.n_pairs == 0)
    m.min_gap = 0.;

  for (size_t i = 0; i < dem.d_xRef.size(); ++i) {
    const double x = dem.d_xRef[i].d_x;
    const double fx = dem.d_f[i].d_x * dem.d_vol[i];
    if (x < x_crack && x > x_crack - band)
      m.force_x_left += fx;
    if (x >= x_crack && x < x_crack + band)
      m.force_x_right += fx;
  }
  return m;
}

struct LawResult {
  double open_min_gap = 0.;
  double close_min_gap = 0.;
  double close_force_amp = 0.;
};

LawResult runLaw(const std::string &self_contact, const std::filesystem::path &run_dir, double L,
                 double mesh_size, double horizon, unsigned n_threads,
                 bool deck_only = false) {
  namespace fs = std::filesystem;
  const fs::path out_dir = run_dir / "out";
  const fs::path inp_dir = run_dir / "inp";
  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);

  const fs::path mesh = inp_dir / "mesh_rect.msh";
  // Time fields unused for prescribed kinematics; keep Output_Interval valid.
  auto input_json =
      buildInputJson(directoryPathWithTrailingSep(out_dir), mesh, L, mesh_size, horizon, 1.0e-6,
                     2, self_contact);
  {
    std::ofstream os(inp_dir / "input.json");
    os << input_json.dump(2);
  }

  // -deckOnly stops after the deck is written. The deck comparison reads the
  // deck and does not need this driver to run the simulation.
  if (deck_only) {
    util::io::print("deck written; -deckOnly, not running\n");
    return LawResult{};
  }

  auto deck = std::make_shared<inp::Input>(input_json);
  PeriDEMModel dem(deck);
  dem.init();

  const double x_crack = 0.5 * L;
  const size_t n_broken = applyPrecrack(dem, x_crack);
  if (n_broken == 0)
    throw std::runtime_error(self_contact + ": precrack broke no bonds");

  const double Rc = dem.d_maxContactR > 0. ? dem.d_maxContactR : 0.95 * mesh_size;
  const double h = dem.d_hMax > 0. ? dem.d_hMax : mesh_size;
  util::io::print(std::format("{}: nodes={}, broken_bonds={}, h={}, Rc={}, horizon={}\n",
                              self_contact, dem.d_x.size(), n_broken, h, Rc, horizon));

  // Open: separate faces so current gap >> Rc (no self-contact yet).
  const double u_open = 2.0 * Rc;
  applyRigidOpenClose(dem, x_crack, u_open);
  dem.computeForces();
  const auto open_m = measure(dem, x_crack, 2.0 * horizon);
  if (!(open_m.min_gap > 1.5 * Rc))
    throw std::runtime_error(std::format("{}: open did not separate (min_gap={})", self_contact,
                                         open_m.min_gap));

  // Close into the window where reference_gap (natural R = r0) and
  // broken_bond_kn (natural R = Rc) disagree: Rc < R < typical r0.
  // Mid-horizon neighbor distances are ~O(horizon) >> Rc = 0.95 h.
  const double u_close = -0.5 * (Rc + 0.5 * horizon);
  applyRigidOpenClose(dem, x_crack, u_close);
  dem.computeForces();
  const auto close_m = measure(dem, x_crack, 2.0 * horizon);
  const double force_amp =
      0.5 * (std::abs(close_m.force_x_left) + std::abs(close_m.force_x_right));

  util::io::print(std::format("{}: open min_gap={}, close min_gap={}, force_amp={}\n", self_contact,
                              open_m.min_gap, close_m.min_gap, force_amp));

  dem.close();

  LawResult r;
  r.open_min_gap = open_m.min_gap;
  r.close_min_gap = close_m.min_gap;
  r.close_force_amp = force_amp;
  return r;
}

} // namespace

int main(int argc, char *argv[]) {
  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);

  unsigned int n_threads = 2;
  if (input.cmdOptionExists("-nThreads"))
    n_threads = static_cast<unsigned>(std::stoi(input.getCmdOption("-nThreads")));
  util::parallel::initNThreads(n_threads);

  namespace fs = std::filesystem;
  fs::path base = fs::current_path() / "self_contact_mode1_run";
  if (input.cmdOptionExists("-outputDir"))
    base = input.getCmdOption("-outputDir");

  const double L = 0.002;
  const double mesh_size = L / 6.0;
  const double horizon = 3.0 * mesh_size;

  LawResult bb;
  LawResult rg;
  try {
    const bool deck_only = input.cmdOptionExists("-deckOnly");
    bb = runLaw("broken_bond_kn", base / "broken_bond_kn", L, mesh_size, horizon,
                n_threads, deck_only);
    rg = runLaw("reference_gap", base / "reference_gap", L, mesh_size, horizon,
                n_threads, deck_only);
    if (deck_only)
      return EXIT_SUCCESS;
  } catch (const std::exception &e) {
    util::io::print(std::format("self-contact Mode-I failed: {}\n", e.what()));
    return 1;
  }

  if (!(bb.close_force_amp > 0.) || !(rg.close_force_amp > 0.)) {
    util::io::print(std::format(
        "close did not activate self-contact: F_bb={}, F_rg={}\n", bb.close_force_amp,
        rg.close_force_amp));
    return 1;
  }

  const double fmax = std::max(bb.close_force_amp, rg.close_force_amp);
  const double rel = std::abs(bb.close_force_amp - rg.close_force_amp) / fmax;
  if (!(rel > 0.05)) {
    util::io::print(std::format(
        "self-contact laws did not differ enough under close: F_bb={}, F_rg={}, rel={}\n",
        bb.close_force_amp, rg.close_force_amp, rel));
    return 1;
  }

  util::io::print(std::format(
      "self-contact Mode-I OK: open gaps bb={}/rg={}, close forces bb={}/rg={}, rel_diff={}\n", bb.open_min_gap,
      rg.open_min_gap, bb.close_force_amp, rg.close_force_amp, rel));
  return 0;
}
