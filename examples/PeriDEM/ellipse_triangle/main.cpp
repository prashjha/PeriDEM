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
  const double Kn_tt = util::normalContactStiffness(K_t, K_t, horizon);
  const double Kn_ee = util::normalContactStiffness(K_e, K_e, horizon);
  const double Kn_te = util::normalContactStiffness(K_t, K_e, horizon);

  const double g = 10.0;
  const double ic_vy = -2.5;

  const size_t dt_out_n = std::max<size_t>(1, num_steps / 40);

  auto model = inp::ModelDeck::getExampleJson(
      json{{"Dimension", 2},
           {"Final_Time", final_time},
           {"Time_Steps", num_steps},
           {"Discretization_Type", json{{"Spatial", "finite_difference"}, {"Time", "central_difference"}}},
           {"Populate_ElementNodeConnectivity", true},
           {"Quad_Approximation_Order", 2},
           {"Particle_Sim_Type", "Multi_Particle"},
           {"Seed", 0}});
  model["Bond_Break"] = "tension";
  // No self-contact across broken bonds — otherwise the crack stays glued.
  model["Self_Contact"] = "none";

  auto output = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", output_path},
           {"Tags", std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage_Z", "Damage",
                                "Particle_ID", "Fixity"})},
           {"Output_Interval", dt_out_n},
           {"Debug", 1},
           {"Perform_FE_Out", true},
           {"Compress_Type", "zlib"},
           {"Perform_Out", true},
           {"Test_Output_Interval", 1},
           {"Tag_PP", ""},
           {"PVD_Collection", true}});

  auto bc = inp::BCDeck::getExampleJson(
      json{{"Displacement_BC_Sets", 1},
           {"IC_Sets", 1},
           {"Gravity", util::Point(0., -g, 0.).toVec()}});
  bc["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson(
      json{{"Type", "Displacement_BC"},
           {"Particle_List", {0}},
           {"Direction", {1, 2}},
           {"Zero_Displacement", true}});

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
      inp::MaterialDeck::getExampleJson(
      json{{"Type", "PDState"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho_t},
           {"K", K_t},
           {"G", G_t},
           {"Gc", Gc_t},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 0}}}});
  mat["Set_2"] =
      inp::MaterialDeck::getExampleJson(
      json{{"Type", "PDState"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho_e},
           {"K", K_e},
           {"G", G_e},
           {"Gc", Gc_e},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 0}}}});
  pDeck["Material"] = mat;

  auto contact = inp::ParticleDeck::getParticleContactExampleJson(2);
  json cpair_wall = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius_Factor", R_contact_factor},
               {"Kn", Kn_tt},
               {"Damping_On", true},
               {"Epsilon", 0.95},
               {"Friction_On", false},
               {"Friction_Coeff", 0.0},
               {"Kn_Factor", 1.0},
               {"Beta_n_Factor", 100.0},
               {"K", K_t}});
  // Tip contact: enough Beta_n to drive a crack, damping on to limit spray.
  json cpair_tip = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius_Factor", 0.90},
               {"Kn", Kn_te},
               {"Damping_On", true},
               {"Epsilon", 0.4},
               {"Friction_On", false},
               {"Friction_Coeff", 0.0},
               {"Kn_Factor", 1.0},
               {"Beta_n_Factor", 8.0},
               {"K", util::harmonicMean(K_t, K_e)}});
  json cpair_ell = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius_Factor", R_contact_factor},
               {"Kn", Kn_ee},
               {"Damping_On", true},
               {"Epsilon", 0.95},
               {"Friction_On", false},
               {"Friction_Coeff", 0.0},
               {"Kn_Factor", 1.0},
               {"Beta_n_Factor", 100.0},
               {"K", K_e}});
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

  pDeck["Neighbor"] = inp::PNeighborDeck::getExampleJson(
          json{{"Update_Criteria", "simple_all"},
               {"Search_Factor", 8.0},
               {"Search_Interval", 5},
               {"Near_Bd_Nodes_Tol", 0.5}});

  auto gen = inp::PGenDeck::getExampleJson(json{{"Method", "From_File"}});
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

  // -deckOnly stops after the deck is written. The Python comparison reads
  // the deck and does not need this driver to run the simulation.
  if (input.cmdOptionExists("-deckOnly")) {
    util::io::print("deck written; -deckOnly, not running\n");
    util::parallel::finalizeMpi();
    return 0;
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
