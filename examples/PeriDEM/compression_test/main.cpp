/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 */

#include "geom/geomObjectsUtil.h"
#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "model/dem/demModel.h"
#include "util/function.h"
#include "util/io.h"
#include <cmath>
#include <filesystem>
#include <format>
#include <limits>
#include <random>
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

struct PackedParticle {
  double x{}, y{}, z{}, r{}, theta{};
};

struct PackedBounds {
  double min_x{}, max_x{}, min_y{}, max_y{};
};

PackedBounds packedAxisBounds(const std::vector<PackedParticle> &packed) {
  PackedBounds b;
  b.min_x = std::numeric_limits<double>::infinity();
  b.max_x = -std::numeric_limits<double>::infinity();
  b.min_y = std::numeric_limits<double>::infinity();
  b.max_y = -std::numeric_limits<double>::infinity();
  for (const auto &p : packed) {
    b.min_x = std::min(b.min_x, p.x - p.r);
    b.max_x = std::max(b.max_x, p.x + p.r);
    b.min_y = std::min(b.min_y, p.y - p.r);
    b.max_y = std::max(b.max_y, p.y + p.r);
  }
  return b;
}

/**
 * Regular grid of identical circles: surface-to-surface gap = gap (e.g. 1.1 * mesh_size)
 * to left/right/bottom walls; centers stepped by 2R + gap.
 */
std::vector<PackedParticle> generateCircularGrid(int ncols, int nrows, double x_lo, double y_lo, double R,
                                                 double gap) {
  std::vector<PackedParticle> particles;
  particles.reserve(static_cast<size_t>(ncols * nrows));
  const double step = 2.0 * R + gap;
  const double x0 = x_lo + gap + R;
  const double y0 = y_lo + gap + R;
  for (int j = 0; j < nrows; ++j) {
    for (int i = 0; i < ncols; ++i) {
      PackedParticle p;
      p.x = x0 + static_cast<double>(i) * step;
      p.y = y0 + static_cast<double>(j) * step;
      p.z = 0.0;
      p.r = R;
      p.theta = 0.0;
      particles.push_back(p);
    }
  }
  return particles;
}

json contactPairJson(double R_contact_factor, bool damping_on, bool friction_on, double Kn, double beta_n_eps,
                     double friction_coeff, double Kn_factor, double beta_n_factor) {
  json j;
  j["Contact_Radius_Factor"] = R_contact_factor;
  if (Kn < 1e-20) {
    j["Kn"] = 0.0;
  } else {
    j["Kn"] = Kn;
  }
  j["Damping_On"] = damping_on;
  j["Epsilon"] = beta_n_eps;
  j["Friction_On"] = friction_on;
  j["Friction_Coeff"] = friction_coeff;
  j["Kn_Factor"] = Kn_factor;
  j["Beta_n_Factor"] = damping_on ? beta_n_factor : 0.0;
  return j;
}

double KnFromBulk(double Ka, double Kb, double horizon) {
  return 18.0 * util::harmonicMean(Ka, Kb) / (M_PI * std::pow(horizon, 5));
}

json buildInputJson(const std::string &output_path_for_deck, const std::filesystem::path &inp_dir,
                    int argc, char *argv[]) {

  util::io::InputParser input(argc, argv);

  /* Moving wall prescribed velocity (y), same scale as Displacement_BC Set_2 linear slope. */
  constexpr double wall_vy = 0.; //-0.06;
  const double v_perturb = 0.3 * std::abs(wall_vy);

  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double R = 0.001;
  const double mesh_size = R / 5.0;
  const double horizon = 2.0 * mesh_size;

  /* Spacing: gap between particle surfaces and from side/bottom walls = 1.1 * mesh_size. */
  const double gap = 0.7 * mesh_size;

  constexpr double wall_top_inset = 1e-7;
  constexpr double annulus_inset = 1e-7;
  const double geom_pad = 0.25 * mesh_size;
  const double plate_thickness = 2.0 * mesh_size;

  const double final_time = 0.02;
  size_t num_steps = 400000;
  if (input.cmdOptionExists("-numSteps"))
    num_steps = static_cast<size_t>(std::stoul(input.getCmdOption("-numSteps")));

  const size_t num_outputs = 200;
  const size_t dt_out_n = num_steps / num_outputs;
  const size_t test_dt_out_n = dt_out_n / 10;

  const double rho_wall = 1200.;
  const double poisson_wall = 0.25;
  const double K_wall = 2.16e+7;
  const double E_wall = material::toE(K_wall, poisson_wall);
  const double G_wall = material::toGE(E_wall, poisson_wall);
  const double KIc_wall = 10e+6;
  const double Gc_wall = material::toGc(KIc_wall, poisson_wall, E_wall);

  const double rho_p = 1200.;
  const double poisson_p = poisson_wall;
  const double K_p = 2.16e+7;
  const double E_p = material::toE(K_p, poisson_p);
  const double G_p = material::toGE(E_p, poisson_p);
  const double KIc_p = 10e+6;
  const double Gc_p = material::toGc(KIc_p, poisson_p, E_p);

  const double R_contact_factor = 0.5;
  constexpr int kNcols = 8;
  constexpr int kNrows = 5;
  constexpr int N_target = kNcols * kNrows;
  static_assert(N_target == 40, "grid must hold 40 particles");

  const double Lin = 2.0 * gap + 2.0 * R + static_cast<double>(kNcols - 1) * (2.0 * R + gap);
  const double Win = gap + 2.0 * R + static_cast<double>(kNrows - 1) * (2.0 * R + gap);

  const std::vector<double> in_rect = {center[0] - 0.5 * Lin, center[1] - 0.5 * Win, center[2],
                                       center[0] + 0.5 * Lin, center[1] + 0.5 * Win, center[2]};

  std::vector<PackedParticle> packed =
      generateCircularGrid(kNcols, kNrows, in_rect[0], in_rect[1], R, gap);
  if (static_cast<int>(packed.size()) != N_target)
    throw std::runtime_error("compression_test: particle grid count mismatch");

  const double m = gap;
  {
    const PackedBounds bb0 = packedAxisBounds(packed);
    const double delta_y = (in_rect[1] + m) - bb0.min_y;
    for (auto &p : packed)
      p.y += delta_y;
  }

  for (auto &p : packed)
    p.z = 0.0;

  const PackedBounds bb = packedAxisBounds(packed);

  const double void_lox = bb.min_x - m - geom_pad;
  const double void_hix = bb.max_x + m + geom_pad;
  const double void_loy = in_rect[1];
  const double plate_bottom_y = bb.max_y + m + geom_pad;
  double plate_top_y = plate_bottom_y + plate_thickness;

  const double side = std::max(1.5 * horizon, 2.0 * mesh_size);
  const double out_lox = void_lox - side;
  const double out_hix = void_hix + side;
  const double out_loy = void_loy - side;
  const double out_hiy = plate_top_y + wall_top_inset + 2.5 * mesh_size;

  std::vector<double> out_rect = {out_lox, out_loy, center[2], out_hix, out_hiy, center[2]};

  const double void_hi_y = out_rect[4] - annulus_inset;
  std::vector<double> remove_rect = {void_lox, void_loy, center[2], void_hix, void_hi_y, center[2]};

  util::io::print(std::format(
      "[compression_test] bbox(center±r) x∈[{:.6f},{:.6f}] y∈[{:.6f},{:.6f}]; "
      "void x∈[{:.6f},{:.6f}] y_lo {:.6f}; plate y [{:.6f},{:.6f}]; void_hi {:.6f}; outer y∈[{:.6f},{:.6f}]\n",
      bb.min_x, bb.max_x, bb.min_y, bb.max_y, void_lox, void_hix, void_loy, plate_bottom_y, plate_top_y,
      void_hi_y, out_rect[1], out_rect[4]));

  std::vector<double> moving_rect = {void_lox, plate_bottom_y, center[2], void_hix, plate_top_y, center[2]};

  std::vector<double> fixed_container_params;
  fixed_container_params.insert(fixed_container_params.end(), remove_rect.begin(), remove_rect.end());
  fixed_container_params.insert(fixed_container_params.end(), out_rect.begin(), out_rect.end());

  const size_t n_pack = packed.size();
  const size_t n_wall_fixed = n_pack;
  const size_t n_wall_moving = n_pack + 1;
  const size_t n_total = n_pack + 2;

  std::vector<geom::GeomData> pGeomVec(3);
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R, center[0], center[1], center[2]};
  pGeomVec[1].d_geomName = "rectangle_minus_rectangle";
  pGeomVec[1].d_geomParams = fixed_container_params;
  pGeomVec[2].d_geomName = "rectangle";
  pGeomVec[2].d_geomParams = moving_rect;

  for (auto &g : pGeomVec)
    geom::createGeomObject(g);

  const util::Point cfix = pGeomVec[1].d_geom_p->center();
  const util::Point cmov = pGeomVec[2].d_geom_p->center();
  const util::Point site_wall_fixed(cfix.d_x, cfix.d_y, 0.0);
  const util::Point site_wall_moving(cmov.d_x, cmov.d_y, 0.0);

  auto modelDeckJson = inp::ModelDeck::getExampleJson(2, final_time, num_steps, "finite_difference",
                                                        "central_difference", true, 2, "Multi_Particle", 0);

  std::vector<std::string> out_tags = {"Displacement", "Velocity", "Force", "Force_Density", "Damage_Z",
                                         "Damage",      "Nodal_Volume", "Zone_ID", "Particle_ID", "Fixity",
                                         "Force_Fixity", "Contact_Nodes", "No_Fail_Node", "Boundary_Node_Flag"};
  auto outputDeckJson = inp::OutputDeck::getExampleJson("vtu", output_path_for_deck, out_tags, dt_out_n, 2,
                                                          true, "zlib", true, test_dt_out_n, "0", true);

  auto bcDeckJson = inp::BCDeck::getExampleJson(0, 2, n_pack, true, util::Point(0, -10, 0));

  bcDeckJson["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("Displacement_BC", false, geom::GeomData(),
                                                                           {n_wall_fixed}, {}, "", {}, "", {},
                                                                           {1, 2}, true, "", {});

  json set2;
  set2["Particle_List"] = std::vector<size_t>{n_wall_moving};
  set2["Direction"] = std::vector<size_t>{2};
  set2["Time_Function"] = json{{"Type", "linear"}, {"Parameters", std::vector<double>{wall_vy}}};
  set2["Spatial_Function"] = json{{"Type", "constant"}};
  bcDeckJson["Displacement_BC"]["Set_2"] = set2;

  json set3;
  set2["Particle_List"] = std::vector<size_t>{n_wall_moving};
  set2["Direction"] = std::vector<size_t>{1};
  set2["Time_Function"] = json{{"Type", "constant"}, {"Parameters", std::vector<double>{0}}};
  set2["Spatial_Function"] = json{{"Type", "constant"}};
  bcDeckJson["Displacement_BC"]["Set_3"] = set3;

  std::mt19937 gen_vel(41);
  std::uniform_real_distribution<double> rnd_v(-v_perturb, v_perturb);
  for (size_t pi = 0; pi < n_pack; ++pi) {
    const double ic_vx = rnd_v(gen_vel);
    const double ic_vy = rnd_v(gen_vel);
    bcDeckJson["IC"]["Set_" + std::to_string(pi + 1)] = inp::BCBaseDeck::getExampleJson(
        "IC", false, geom::GeomData(), std::vector<size_t>{pi}, {}, "", {}, "", {}, {}, false,
        "Constant_Velocity", std::vector<double>{ic_vx, ic_vy, 0.0});
  }

  json pDeckJson = json::object();

  pDeckJson["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);

  json meshRoot = json{{"Sets", 3}};
  const char *mesh_names[] = {"mesh_circle", "mesh_fixed_container", "mesh_moving_container"};
  for (int zi = 0; zi < 3; ++zi) {
    meshRoot["Set_" + std::to_string(zi + 1)] =
        json{{"CreateMesh",
              json{{"Flag", true}, {"Info", "gmsh_builtin_mesh"}, {"Mesh_Size", mesh_size}, {"Write_Mesh_File", false}}}};
  }
  pDeckJson["Mesh"] = meshRoot;

  json matRoot = json{{"Sets", 2}};
  matRoot["Set_1"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho_p, K_p, G_p, Gc_p, true, 1);
  matRoot["Set_2"] =
      inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho_wall, K_wall, G_wall, Gc_wall, true, 1);
  pDeckJson["Material"] = matRoot;

  const double Kn_pp = KnFromBulk(K_p, K_p, horizon);
  const double Kn_pw = KnFromBulk(K_p, K_wall, horizon);
  const double Kn_ww = 0.0;

  const double beta_n_eps = 0.95;
  const double friction_coeff = 0.5;
  const bool damping_on = true;
  const bool friction_on = false;
  const double beta_n_factor = 10.;
  const double Kn_factor = 0.01;

  const json j_contact_pp =
      contactPairJson(R_contact_factor, damping_on, friction_on, Kn_pp, beta_n_eps, friction_coeff, Kn_factor,
                      beta_n_factor);
  const json j_contact_pw =
      contactPairJson(R_contact_factor, damping_on, friction_on, Kn_pw, beta_n_eps, friction_coeff, Kn_factor,
                      beta_n_factor);
  const json j_contact_ww =
      contactPairJson(R_contact_factor, damping_on, friction_on, Kn_ww, beta_n_eps, friction_coeff, Kn_factor,
                      beta_n_factor);

  json contactRoot = inp::ContactDeck::getExampleJson(2);
  contactRoot["Set_1_1"] = j_contact_pp;
  contactRoot["Set_1_2"] = j_contact_pw;
  contactRoot["Set_2_2"] = j_contact_ww;
  pDeckJson["Contact"] = contactRoot;

  pDeckJson["Neighbor"] = inp::PNeighborDeck::getExampleJson("simple_all", 10.0, 100, 0.5);

  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");
  pGenJson["Random_Rotation"] = false;
  pGenJson["Data"]["N"] = n_total;

  constexpr size_t k_mat_particle = 0;
  constexpr size_t k_mat_wall = 1;
  constexpr size_t k_contact_grains = 0;
  constexpr size_t k_contact_wall = 1;

  for (size_t pi = 0; pi < n_pack; ++pi) {
    const auto &p = packed[pi];
    pGenJson["Data"][std::to_string(pi)] = json{{"x", p.x},
                                                 {"y", p.y},
                                                 {"z", 0.0},
                                                 {"theta", p.theta},
                                                 {"s", 1.0},
                                                 {"geom_id", size_t(0)},
                                                 {"mat_id", k_mat_particle},
                                                 {"contact_id", k_contact_grains}};
  }

  pGenJson["Data"][std::to_string(n_pack)] = json{{"x", site_wall_fixed.d_x},
                                                  {"y", site_wall_fixed.d_y},
                                                  {"z", 0.0},
                                                  {"theta", 0.0},
                                                  {"s", 1.0},
                                                  {"geom_id", size_t(1)},
                                                  {"mat_id", k_mat_wall},
                                                  {"contact_id", k_contact_wall}};

  pGenJson["Data"][std::to_string(n_pack + 1)] = json{{"x", site_wall_moving.d_x},
                                                       {"y", site_wall_moving.d_y},
                                                       {"z", 0.0},
                                                       {"theta", 0.0},
                                                       {"s", 1.0},
                                                       {"geom_id", size_t(2)},
                                                       {"mat_id", k_mat_wall},
                                                       {"contact_id", k_contact_wall}};

  pDeckJson["Particle_Generation"] = pGenJson;

  return json{{"Comment", "compression_test"},
              {"Model", modelDeckJson},
              {"Output", outputDeckJson},
              {"Force_BC", bcDeckJson["Force_BC"]},
              {"Displacement_BC", bcDeckJson["Displacement_BC"]},
              {"IC", bcDeckJson["IC"]},
              {"Particle", pDeckJson["Particle"]},
              {"Mesh", pDeckJson["Mesh"]},
              {"Material", pDeckJson["Material"]},
              {"Contact", pDeckJson["Contact"]},
              {"Neighbor", pDeckJson["Neighbor"]},
              {"Particle_Generation", pDeckJson["Particle_Generation"]}};
}

} // namespace

int main(int argc, char *argv[]) {

  util::parallel::initMpi(argc, argv);
  int mpiSize = util::parallel::mpiSize(), mpiRank = util::parallel::mpiRank();
  util::io::print(std::format("Initialized MPI. MPI size = {}, MPI rank = {}\n", mpiSize, mpiRank));
  util::io::print(util::parallel::getMpiStatus()->printStr());

  util::io::InputParser input(argc, argv);

  unsigned int nThreads;
  if (input.cmdOptionExists("-nThreads"))
    nThreads = std::stoi(input.getCmdOption("-nThreads"));
  else {
    nThreads = std::thread::hardware_concurrency();
    util::io::print(std::format("Running test with default number of threads = {}\n", nThreads));
  }
  util::parallel::initNThreads(nThreads);
  util::io::print(std::format("Number of threads = {}\n", util::parallel::getNThreads()));

  namespace fs = std::filesystem;
  const fs::path cwd = fs::current_path();

  fs::path out_dir = cwd / "out";
  fs::path inp_dir = cwd / "inp";

  fs::create_directories(out_dir);

  const std::string output_path_for_deck = directoryPathWithTrailingSep(out_dir);

  auto inputJson = buildInputJson(output_path_for_deck, inp_dir, argc, argv);

  auto deck = std::make_shared<inp::Input>(inputJson);

  model::DEMModel dem(deck);
  dem.run(deck);

  return EXIT_SUCCESS;
}
