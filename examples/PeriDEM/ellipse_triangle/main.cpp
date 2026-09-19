/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Hollow ellipse dropped onto a short tip-up triangle (pointy tip only).
 * Tip contact opens a crack; target is bipartition into two pieces.
 *
 * -outputDir / -inputDir / -finalTime / -numSteps / -nThreads
 */

#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "periDEMModel.h"
#include "util/function.h"
#include "util/io.h"
#include "util/parallelUtil.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
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

json buildInputJson(const std::string &output_path, const std::filesystem::path &mesh_tri,
                    const std::filesystem::path &mesh_ell, double final_time, size_t num_steps,
                    double mesh_size, double horizon) {
  // Short tip — point only, not a tall triangle. Narrower tip concentrates load.
  const double W = 0.0008;
  const double H = 0.0012;

  // Hollow ellipse (visibly elliptical ring).
  const double a_out = 0.0020;
  const double b_out = 0.0014;
  // Thin ring so a tip-seeded crack can bipartition.
  const double a_in = 0.00180;
  const double b_in = 0.00122;
  const double ell_theta = 0.0;
  const double tip_gap = 0.00008;
  const double tip_y = H;
  const double ell_cy = tip_y + tip_gap + b_out;

  const double rho_t = 1200.0;
  const double K_t = 2.16e7;
  const double nu_t = 0.25;
  const double E_t = material::toE(K_t, nu_t);
  const double G_t = material::toGE(E_t, nu_t);
  const double Gc_t = 200.0;

  const double rho_e = 1200.0;
  const double K_e = 2.16e7;
  const double nu_e = 0.25;
  const double E_e = material::toE(K_e, nu_e);
  const double G_e = material::toGE(E_e, nu_e);
  // Low Gc so tip damage runs into a diametral through-crack.
  const double Gc_e = 1.0;

  const double R_contact_factor = 0.95;
  const double Kn_tt = 18.0 * util::harmonicMean(K_t, K_t) / (M_PI * std::pow(horizon, 5));
  const double Kn_ee = 18.0 * util::harmonicMean(K_e, K_e) / (M_PI * std::pow(horizon, 5));
  const double Kn_te = 18.0 * util::harmonicMean(K_t, K_e) / (M_PI * std::pow(horizon, 5));

  const double g = 10.0;
  const double ic_vy = -2.5;

  const size_t dt_out_n = std::max<size_t>(1, num_steps / 40);

  auto model = inp::ModelDeck::getExampleJson(2, final_time, num_steps, "finite_difference",
                                              "central_difference", true, 2, "Multi_Particle", 0);
  model["Bond_Break"] = "tension";
  // No self-contact across broken bonds — otherwise the crack stays glued.
  model["Self_Contact"] = "none";

  auto output = inp::OutputDeck::getExampleJson(
      "vtu", output_path,
      std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage_Z", "Damage",
                                "Particle_ID", "Fixity"}),
      dt_out_n, 1, true, "zlib", true, 1, "", true);

  auto bc = inp::BCDeck::getExampleJson(0, 1, 1, true, util::Point(0., -g, 0.));
  bc["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson(
      "Displacement_BC", false, geom::GeomData(), {0}, {}, "", {}, "", {}, {1, 2}, true, "", {});

  bc["IC"]["Set_1"] = {
      {"Particle_List", std::vector<size_t>{1}},
      {"Constant_Velocity", {{"Velocity_Vector", std::vector<double>{0., ic_vy, 0.}}}}};

  std::vector<geom::GeomData> pGeom(2);
  pGeom[0].d_geomName = "triangle";
  pGeom[0].d_geomParams = {-0.5 * W, 0., 0., 0.5 * W, 0., 0., 0., H, 0.};

  pGeom[1].d_geomName = "ellipse_minus_ellipse";
  pGeom[1].d_geomParams = {a_out, b_out, a_in, b_in, ell_theta, 0., 0., 0.};

  for (auto &g : pGeom)
    geom::createGeomObject(g);

  const util::Point c_tri = pGeom[0].d_geom_p->center();

  json pDeck = json::object();
  pDeck["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeom);

  pDeck["Mesh"] = {
      {"Sets", 2},
      {"Set_1",
       {{"File", mesh_tri.string()},
        {"CreateMesh",
         {{"Flag", true},
          {"Info", "gmsh_builtin_mesh"},
          {"Mesh_Size", mesh_size},
          {"Write_Mesh_File", true}}}}},
      {"Set_2",
       {{"File", mesh_ell.string()},
        {"CreateMesh",
         {{"Flag", true},
          {"Info", "gmsh_builtin_mesh"},
          {"Mesh_Size", mesh_size},
          {"Write_Mesh_File", true}}}}}};

  auto mat = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  mat["Set_1"] =
      inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho_t, K_t, G_t, Gc_t, true, 1);
  mat["Set_2"] =
      inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho_e, K_e, G_e, Gc_e, true, 1);
  pDeck["Material"] = mat;

  auto contact = inp::ParticleDeck::getParticleContactExampleJson(2);
  json cpair_wall = inp::ContactPairDeck::getExampleJson(R_contact_factor, true, true, false, Kn_tt, 0.95,
                                                      0.0, 1.0, 100.0, 1.0, 0.0, K_t);
  // Tip contact: enough Beta_n to drive a crack, damping on to limit spray.
  json cpair_tip = inp::ContactPairDeck::getExampleJson(0.90, true, true, false, Kn_te, 0.4,
                                                     0.0, 1.0, 8.0, 1.0, 0.0,
                                                     util::harmonicMean(K_t, K_e));
  json cpair_ell = inp::ContactPairDeck::getExampleJson(R_contact_factor, true, true, false, Kn_ee, 0.95,
                                                     0.0, 1.0, 100.0, 1.0, 0.0, K_e);
  contact["Set_1_1"] = cpair_wall;
  contact["Set_1_1"]["Kn"] = Kn_tt;
  contact["Set_1_1"]["K"] = K_t;
  contact["Set_1_2"] = cpair_tip;
  contact["Set_1_2"]["Kn"] = Kn_te;
  contact["Set_1_2"]["K"] = util::harmonicMean(K_t, K_e);
  contact["Set_2_2"] = cpair_ell;
  contact["Set_2_2"]["Kn"] = Kn_ee;
  contact["Set_2_2"]["K"] = K_e;
  contact["Damping_Law"] = "com_and_node";
  contact["Friction_Law"] = "coulomb_simple";
  pDeck["Contact"] = contact;

  pDeck["Neighbor"] = inp::PNeighborDeck::getExampleJson("simple_all", 8.0, 5, 0.5);

  auto gen = inp::PGenDeck::getExampleJson("From_File");
  gen["Random_Rotation"] = false;
  gen["Data"]["N"] = 2;
  gen["Data"]["0"] = {{"x", c_tri.d_x}, {"y", c_tri.d_y}, {"z", 0.0},
                      {"theta", 0.0},   {"s", 1.0},
                      {"geom_id", 0},   {"mat_id", 0},    {"contact_id", 0},
                      {"is_wall", true}};
  gen["Data"]["1"] = {{"x", 0.0}, {"y", ell_cy}, {"z", 0.0},
                      {"theta", 0.0}, {"s", 1.0},
                      {"geom_id", 1}, {"mat_id", 1}, {"contact_id", 1}};
  pDeck["Particle_Generation"] = gen;

  json j = json::object();
  j["Model"] = model;
  j["Output"] = output;
  j["Force_BC"] = bc["Force_BC"];
  j["Displacement_BC"] = bc["Displacement_BC"];
  j["IC"] = bc["IC"];
  j["Particle"] = pDeck["Particle"];
  j["Mesh"] = pDeck["Mesh"];
  j["Material"] = pDeck["Material"];
  j["Contact"] = pDeck["Contact"];
  j["Neighbor"] = pDeck["Neighbor"];
  j["Particle_Generation"] = pDeck["Particle_Generation"];
  return j;
}

} // namespace

int main(int argc, char *argv[]) {
  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);

  unsigned int n_threads = std::thread::hardware_concurrency();
  if (input.cmdOptionExists("-nThreads"))
    n_threads = std::stoul(input.getCmdOption("-nThreads"));
  util::parallel::initNThreads(n_threads);
  util::io::print(std::format("Number of threads = {}\n", util::parallel::getNThreads()));

  double final_time = 0.0030;
  size_t num_steps = 30000;
  if (input.cmdOptionExists("-finalTime"))
    final_time = std::stod(input.getCmdOption("-finalTime"));
  if (input.cmdOptionExists("-numSteps"))
    num_steps = std::stoul(input.getCmdOption("-numSteps"));

  const double mesh_size = 0.00010;
  const double horizon = 3.0 * mesh_size;

  namespace fs = std::filesystem;
  const fs::path cwd = fs::current_path();
  fs::path out_dir = cwd / "out";
  fs::path inp_dir = cwd / "inp";
  if (input.cmdOptionExists("-outputDir")) {
    fs::path p = input.getCmdOption("-outputDir");
    out_dir = p.is_absolute() ? std::move(p) : cwd / p;
  }
  if (input.cmdOptionExists("-inputDir")) {
    fs::path p = input.getCmdOption("-inputDir");
    inp_dir = p.is_absolute() ? std::move(p) : cwd / p;
  } else if (input.cmdOptionExists("-outputDir")) {
    inp_dir = out_dir.parent_path() / "inp";
  }
  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);

  const std::string output_path = directoryPathWithTrailingSep(out_dir);
  const fs::path mesh_tri = inp_dir / "mesh_triangle.msh";
  const fs::path mesh_ell = inp_dir / "mesh_hollow_ellipse.msh";

  auto j = buildInputJson(output_path, mesh_tri, mesh_ell, final_time, num_steps, mesh_size,
                          horizon);
  {
    std::ofstream ofs(inp_dir / "input.json");
    ofs << std::setw(2) << j << std::endl;
  }

  // Hard check: deck must request a hollow ellipse, not a rectangle plate.
  if (j["Particle"]["Set_2"]["Type"] != "ellipse_minus_ellipse")
    throw std::runtime_error("ellipse_triangle: Particle Set_2 must be ellipse_minus_ellipse");

  util::io::print(std::format(
      "[ellipse_triangle] Type={}, T={}, Nt={}, mesh={:.4e}, out={}\n",
      j["Particle"]["Set_2"]["Type"].get<std::string>(), final_time, num_steps, mesh_size,
      out_dir.string()));

  auto deck = std::make_shared<inp::Input>(j);
  PeriDEMModel dem(deck);
  dem.run(deck);

  return 0;
}
