/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * One grain vs a rectangle floor with Model.Wall_Contact = analytical_plane.
 * Asserts force density vs Kn*(gap-Rc)*voli*(-n) and that contact keeps
 * signed gap from deep wall penetration.
 */

#include "contact/contact.h"
#include "geom/geomIncludes.h"
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

namespace {

std::string directoryPathWithTrailingSep(const std::filesystem::path &dir) {
  namespace fs = std::filesystem;
  fs::path n = fs::absolute(dir).lexically_normal();
  std::string s = n.string();
  if (!s.empty() && s.back() != '/' && s.back() != '\\')
    s += fs::path::preferred_separator;
  return s;
}

json buildInputJson(const std::string &output_path,
                    const std::filesystem::path &mesh_cir,
                    const std::filesystem::path &mesh_wall, double R,
                    double mesh_size, double horizon, double Rc_factor,
                    double Kn, double final_time, size_t num_steps,
                    bool policy_combo = false) {
  auto model = inp::ModelDeck::getExampleJson(2, final_time, num_steps,
                                              "finite_difference",
                                              "central_difference", true, 2,
                                              "Multi_Particle", 0);
  model["Wall_Contact"] = "analytical_plane";
  if (policy_combo) {
    model["Self_Contact"] = "reference_gap";
    model["Bond_Break"] = "absolute_stretch";
  }

  auto output = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", output_path},
           {"Tags", std::vector<std::string>({"Displacement", "Force", "Particle_ID"})},
           {"Output_Interval", std::max<size_t>(1, num_steps / 5)},
           {"Debug", 1},
           {"Perform_FE_Out", false},
           {"Compress_Type", "zlib"},
           {"Perform_Out", false},
           {"Test_Output_Interval", num_steps},
           {"Tag_PP", ""},
           {"PVD_Collection", false}});

  auto bc = inp::BCDeck::getExampleJson(
      json{{"Displacement_BC_Sets", 1}});
  bc["Displacement_BC"]["Set_1"] = json{
      {"Particle_List", std::vector<size_t>{1}},
      {"Direction", std::vector<size_t>{1, 2}},
      {"Time_Function", {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
      {"Spatial_Function", {{"Type", "constant"}}},
      {"Zero_Displacement", true}};

  const double wall_half = 3.0 * R;
  const double wall_thick = mesh_size;
  geom::GeomData grain;
  grain.d_geomName = "circle";
  grain.d_geomParams = {R, 0., 0., 0.};
  geom::GeomData floor;
  floor.d_geomName = "rectangle";
  floor.d_geomParams = {-wall_half, -wall_thick, 0., wall_half, 0., 0.};

  auto particle = inp::ParticleDeck::getParticleGeomExampleJson({grain, floor});

  auto mesh_set = [&](const std::filesystem::path &f) {
    return json{{"File", f.string()},
                {"CreateMesh",
                 {{"Flag", true},
                  {"Info", "gmsh_builtin_mesh"},
                  {"Mesh_Size", mesh_size},
                  {"Write_Mesh_File", true}}}};
  };
  json mesh = {{"Sets", 2},
               {"Set_1", mesh_set(mesh_cir)},
               {"Set_2", mesh_set(mesh_wall)}};

  const double rho = 1200.0;
  const double K = 2.16e7;
  const double G = 1.296e7;
  const double Gc = 50.0;
  auto material = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  material["Set_1"] =
      inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho, K, G, Gc,
                                        true, 1);
  material["Set_2"] =
      inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho, K, G, Gc,
                                        true, 1);

  json contact_base = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius_Factor", Rc_factor},
               {"Kn", Kn},
               {"Damping_On", false},
               {"Epsilon", 1.0},
               {"Friction_On", false},
               {"Friction_Coeff", 0.0},
               {"Kn_Factor", 1.0},
               {"Beta_n_Factor", 100.0},
               {"K", K}});
  // Soften spring for a short stable settle (formula check uses this Kn too).
  contact_base["Kn"] = Kn;
  contact_base["Kn_Factor"] = 1.0;
  json contact = inp::ParticleDeck::getParticleContactExampleJson(2);
  contact["Set_1_1"] = contact_base;
  contact["Set_1_2"] = contact_base;
  contact["Set_2_2"] = contact_base;
  contact["Damping_Law"] = policy_combo ? "node" : "off";
  contact["Friction_Law"] = policy_combo ? "stick_slip" : "coulomb_simple";

  // Bottom of grain near the floor (y=0): slight overlap into Rc after setup.
  const double cy = R + 0.25 * mesh_size;
  auto pgen = inp::PGenDeck::getExampleJson(json{{"Method", "From_File"}});
  pgen["Random_Rotation"] = false;
  pgen["Data"]["N"] = 2;
  pgen["Data"]["0"] = json{{"x", 0.},
                           {"y", cy},
                           {"z", 0.},
                           {"theta", 0.},
                           {"s", 1.},
                           {"geom_id", 0},
                           {"mat_id", 0},
                           {"contact_id", 0}};
  pgen["Data"]["1"] = json{{"x", 0.},
                           {"y", -0.5 * wall_thick},
                           {"z", 0.},
                           {"theta", 0.},
                           {"s", 1.},
                           {"geom_id", 1},
                           {"mat_id", 1},
                           {"contact_id", 1},
                           {"is_wall", true}};

  return json{{"Model", model},
              {"Output", output},
              {"Displacement_BC", bc["Displacement_BC"]},
              {"Particle", particle},
              {"Mesh", mesh},
              {"Material", material},
              {"Contact", contact},
              {"Neighbor",
               inp::PNeighborDeck::getExampleJson(
          json{{"Update_Criteria", "simple_all"},
               {"Search_Factor", 5.0},
               {"Search_Interval", 1},
               {"Near_Bd_Nodes_Tol", 0.5}})},
              {"Particle_Generation", pgen}};
}

struct Probe {
  size_t n_contact_nodes = 0;
  double min_gap = 1.e300;
  double max_force_err = 0.;
  double max_force_ref = 0.;
};

Probe checkWallSpringOnly(PeriDEMModel &dem, geom::GeomObject &wall_geom) {
  Probe p;
  if (!dem.d_contact_p || !dem.d_contact_p->d_wallContact ||
      !dem.d_contact_p->d_pairForce)
    throw std::runtime_error("analytical wall: contact path not set up");

  const auto &contact = dem.d_particleDeck_p->d_contactDeck.getContact(0, 1);
  const double Kn = contact.d_Kn;
  const double Rc = contact.d_contactR;

  for (auto &f : dem.d_f)
    f = util::Point();
  dem.d_contact_p->d_wallContact->apply(dem, dem.d_contact_p->d_pairForce.get(),
                                        /*use_node_damping*/ false);

  for (size_t i = 0; i < dem.d_x.size(); ++i) {
    if (dem.d_ptId[i] != 0)
      continue;
    geom::WallContactHit hit;
    if (!wall_geom.wallContactQuery(dem.d_x[i], hit) || !hit.active)
      continue;
    if (!(hit.signed_gap < Rc))
      continue;

    ++p.n_contact_nodes;
    p.min_gap = std::min(p.min_gap, hit.signed_gap);

    auto scalar = Kn * (hit.signed_gap - Rc) * dem.d_vol[i];
    if (scalar > 0.)
      scalar = 0.;
    const util::Point f_ref = scalar * (-1. * hit.outward_n);
    const double err = (dem.d_f[i] - f_ref).length();
    p.max_force_err = std::max(p.max_force_err, err);
    p.max_force_ref = std::max(p.max_force_ref, f_ref.length());
  }
  if (p.n_contact_nodes == 0)
    p.min_gap = 0.;
  return p;
}

double minGrainWallGap(PeriDEMModel &dem, geom::GeomObject &wall_geom) {
  double ming = 1.e300;
  size_t n = 0;
  for (size_t i = 0; i < dem.d_x.size(); ++i) {
    if (dem.d_ptId[i] != 0)
      continue;
    geom::WallContactHit hit;
    if (!wall_geom.wallContactQuery(dem.d_x[i], hit) || !hit.active)
      continue;
    ming = std::min(ming, hit.signed_gap);
    ++n;
  }
  return n ? ming : 0.;
}

} // namespace

int main(int argc, char *argv[]) {
  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);

  unsigned n_threads = 2;
  if (input.cmdOptionExists("-nThreads"))
    n_threads = static_cast<unsigned>(std::stoi(input.getCmdOption("-nThreads")));
  util::parallel::initNThreads(n_threads);

  namespace fs = std::filesystem;
  fs::path base = fs::current_path() / "wall_analytical_run";
  if (input.cmdOptionExists("-outputDir"))
    base = input.getCmdOption("-outputDir");
  const fs::path out_dir = base / "out";
  const fs::path inp_dir = base / "inp";
  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);

  const double R = 0.001;
  const double mesh_size = R / 5.0;
  const double horizon = 3.0 * mesh_size;
  const double Rc_factor = 0.95;
  const double Kn = 1.0e11;
  const double final_time = 5.0e-4;
  const size_t num_steps = 5000;

  const bool policy_combo = input.cmdOptionExists("-policyCombo");
  auto input_json =
      buildInputJson(directoryPathWithTrailingSep(out_dir), inp_dir / "mesh_cir.msh",
                     inp_dir / "mesh_wall.msh", R, mesh_size, horizon, Rc_factor, Kn,
                     final_time, num_steps, policy_combo);
  {
    std::ofstream os(inp_dir / "input.json");
    os << input_json.dump(2);
  }

  // -deckOnly stops after the deck is written. The deck comparison reads the
  // deck and does not need this driver to run the simulation.
  if (input.cmdOptionExists("-deckOnly")) {
    util::io::print("deck written; -deckOnly, not running\n");
    util::parallel::finalizeMpi();
    return 0;
  }

  auto deck = std::make_shared<inp::Input>(input_json);
  PeriDEMModel dem(deck);
  dem.init();

  if (dem.d_modelDeck_p->d_wallContact != "analytical_plane")
    throw std::runtime_error("analytical wall: Wall_Contact not analytical_plane");
  if (policy_combo) {
    if (dem.d_modelDeck_p->d_selfContact != "reference_gap")
      throw std::runtime_error("alternate deck: Self_Contact is not reference_gap");
    if (dem.d_modelDeck_p->d_bondBreak != "absolute_stretch")
      throw std::runtime_error("alternate deck: Bond_Break is not absolute_stretch");
    if (dem.d_particleDeck_p->d_contactDeck.d_frictionLaw != "stick_slip")
      throw std::runtime_error("alternate deck: Friction_Law is not stick_slip");
    if (dem.d_particleDeck_p->d_contactDeck.d_dampingLaw != "node")
      throw std::runtime_error("alternate deck: Damping_Law is not node");
    if (!dem.d_contact_p || !dem.d_contact_p->d_pairForce ||
        dem.d_contact_p->d_useNodeDamping != true)
      throw std::runtime_error("alternate deck: node damping not active on contact");
    util::io::print("alternate deck fields applied\n");
  }
  if (!dem.d_contact_p || !dem.d_contact_p->d_wallContact ||
      !dem.d_contact_p->d_wallContact->skipsMeshedGrainWall())
    throw std::runtime_error("analytical wall: must skip meshed grain-wall");

  const double Rc = dem.d_particleDeck_p->d_contactDeck.getContact(0, 1).d_contactR;
  // Floor top face is at y=0 (matches rectangle placement in the deck).
  geom::Plane floor_plane(util::Point(0., 1., 0.), util::Point(0., 0., 0.));

  // Wall spring-only force density vs closed form.
  {
    const double push = 0.5 * Rc;
    for (size_t i = 0; i < dem.d_x.size(); ++i) {
      if (dem.d_ptId[i] != 0)
        continue;
      dem.d_u[i].d_y -= push;
      dem.d_x[i] = dem.d_xRef[i] + dem.d_u[i];
      dem.d_v[i] = util::Point();
    }
    const auto frozen = checkWallSpringOnly(dem, floor_plane);
    util::io::print(std::format(
        "analytical wall formula: n_contact={}, min_gap={}, Rc={}, max|f_ref|={}, max|f-f_ref|={}\n",
        frozen.n_contact_nodes, frozen.min_gap, Rc, frozen.max_force_ref,
        frozen.max_force_err));
    if (frozen.n_contact_nodes == 0)
      throw std::runtime_error("analytical wall: no grain nodes in contact");
    if (!(frozen.max_force_ref > 0.))
      throw std::runtime_error("analytical wall: expected nonzero force");
    if (!(frozen.max_force_err <= 1.e-6 * (1. + frozen.max_force_ref)))
      throw std::runtime_error(std::format(
          "analytical wall: force density mismatch vs Kn*(gap-Rc)*voli*(-n): err={}",
          frozen.max_force_err));
    if (!(frozen.min_gap > -Rc))
      throw std::runtime_error(std::format(
          "analytical wall: deep penetration past Rc in contact config: min_gap={}, Rc={}",
          frozen.min_gap, Rc));
  }

  // Full contact assembly (PD + analytical wall) must keep the same gap bound.
  dem.computeForces();
  const double ming = minGrainWallGap(dem, floor_plane);
  util::io::print(std::format("analytical wall after computeForces: min_gap={}, Rc={}\n", ming, Rc));
  if (!(ming < Rc))
    throw std::runtime_error(
        "analytical wall: expected grain-wall contact (min_gap < Rc)");
  if (!(ming > -Rc))
    throw std::runtime_error(std::format(
        "analytical wall: deep penetration past Rc: min_gap={}, Rc={}", ming, Rc));

  dem.close();
  util::io::print("analytical wall OK\n");
  return 0;
}
