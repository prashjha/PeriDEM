/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Multi-particle compression example (translation of
 * test/test_data/peridem/compression_large_set/inp/problem_setup.py):
 * - Ten geometry / mesh groups: eight particle shapes (small/large × circle/triangle/drum2d/hex),
 *   fixed U-channel wall (rectangle_minus_rectangle), moving top rectangle.
 * - Particle packing follows problem_setup.py::particle_locations (method 1) with std::mt19937(30);
 *   draws differ from NumPy so particle count/positions are not bit-identical to the Python CSV.
 * - Fixed-container annulus: builtin Gmsh requires the “remove” rectangle strictly inside the outer
 *   box; the removal top is inset by 1e-7 from problem_setup.py (which shared an edge with the outer).
 * - Optional flag: -numSteps <N> overrides Model time steps (ctest uses 50 for a quick smoke run;
 *   default 4000 matches problem_setup.py).
 *
 * Layout: ./out/ (VTU, log.txt), ./inp/ (input.json, meshes). Same CLI as twop_circ_inbuilt.
 */

#include "geom/geomObjectsUtil.h"
#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "model/dem/demModel.h"
#include "util/function.h"
#include "util/io.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
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
  int zone{};
  double x{}, y{}, z{}, r{}, theta{};
};

double maxElem(const std::vector<double> &v) {
  double m = -std::numeric_limits<double>::infinity();
  for (double x : v)
    m = std::max(m, x);
  return m;
}

bool doesParticleIntersect(const PackedParticle &p, const std::vector<PackedParticle> &existing,
                           const std::vector<double> &rect, double padding) {
  const double p_rect_lox = p.x - p.r;
  const double p_rect_loy = p.y - p.r;
  const double p_rect_hix = p.x + p.r;
  const double p_rect_hiy = p.y + p.r;
  if (p_rect_lox < rect[0] + padding || p_rect_loy < rect[1] + padding || p_rect_hix > rect[3] - padding ||
      p_rect_hiy > rect[4] - padding)
    return true;

  for (const auto &q : existing) {
    const double dx = p.x - q.x;
    const double dy = p.y - q.y;
    const double dz = p.z - q.z;
    const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist <= p.r + q.r + padding)
      return true;
  }
  return false;
}

/**
 * Mirrors problem_setup.py::particle_locations (method_to_use == 1) with std::mt19937(30).
 */
std::vector<PackedParticle> generateParticleLocations(const std::vector<double> &in_rect, double max_y,
                                                        double mesh_size, double R1, double R2,
                                                        int N1, int N2, double padding, std::mt19937 &gen) {
  std::vector<PackedParticle> particles;
  std::uniform_real_distribution<double> u01(0.0, 1.0);
  std::uniform_real_distribution<double> u_small_r(-0.1 * R1, 0.1 * R1);
  std::uniform_real_distribution<double> u_large_r(-0.1 * R2, 0.1 * R2);
  std::uniform_int_distribution<int> pick01(0, 1);
  std::uniform_int_distribution<int> pick_zone_small(0, 3);
  std::uniform_int_distribution<int> pick_zone_large(0, 3);

  const double check_r = std::min(R1, R2);
  const int rows = static_cast<int>((max_y - in_rect[1]) / (2.0 * check_r));
  const double rect_L = in_rect[3] - in_rect[0];
  const int cols = static_cast<int>(rect_L / (2.0 * check_r));

  int counter1 = 0;
  int counter2 = 0;
  double x_old = in_rect[0];
  double x_old_right = in_rect[3];
  double y_old = in_rect[1];
  const double cz = 0.0;

  std::vector<double> cy_accptd;
  cy_accptd.push_back(y_old);

  std::vector<double> row_rads_prev{R1, R2};
  for (int i = 0; i < rows; ++i) {
    if (i > 0)
      y_old = maxElem(cy_accptd) + maxElem(row_rads_prev);

    std::vector<double> row_rads{R1, R2};

    if (y_old + padding + maxElem(row_rads) >= max_y)
      break;

    int num_p_cols = 0;
    int j = 0;
    while (true) {
      if (num_p_cols > cols - 1 || j > 100 * (N1 + N2))
        break;
      if (counter1 >= N1 && counter2 >= N2)
        break;

      if (j == 0) {
        x_old = in_rect[0];
        x_old_right = in_rect[3];
      }

      int p_type = pick01(gen);
      if (counter1 >= N1)
        p_type = 1;
      if (counter2 >= N2)
        p_type = 0;

      const double r0 = (p_type == 0) ? R1 : R2;
      int p_zone = 0;
      if (p_type == 0)
        p_zone = pick_zone_small(gen);
      else
        p_zone = 4 + pick_zone_large(gen);

      double r = r0 + ((p_type == 0) ? u_small_r(gen) : u_large_r(gen));

      double cx = 0., cy = 0.;
      if (i % 2 == 0) {
        std::uniform_real_distribution<double> rph(-0.1 * r0, 0.05 * r0);
        std::uniform_real_distribution<double> rpv(-0.05 * r0, 0.05 * r0);
        const double cx0 = x_old_right - padding - r;
        cx = cx0 - rph(gen);
        cy = y_old + padding + r + rpv(gen);
      } else {
        std::uniform_real_distribution<double> rph(-0.05 * r0, 0.1 * r0);
        std::uniform_real_distribution<double> rpv(-0.05 * r0, 0.05 * r0);
        const double cx0 = x_old + padding + r;
        cx = cx0 + rph(gen);
        cy = y_old + padding + r + rpv(gen);
      }

      PackedParticle trial{p_zone, cx, cy, cz, r, 0.0};
      if (!doesParticleIntersect(trial, particles, in_rect, padding)) {
        std::uniform_real_distribution<double> orient(0.0, 2.0 * M_PI);
        trial.theta = orient(gen);
        particles.push_back(trial);
        row_rads.push_back(trial.r);
        cy_accptd.push_back(cy);
        if (i % 2 == 0)
          x_old_right = cx - trial.r;
        else
          x_old = cx + trial.r;

        if (p_type == 0)
          counter1++;
        else
          counter2++;
        num_p_cols++;
      }
      ++j;
    }
    row_rads_prev = std::move(row_rads);
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

  const std::vector<double> center = {0.0, 0.0, 0.0};
  const double R_small = 0.001;
  const double R_large = 0.001;
  const double mesh_size = R_small / 5.0;
  const double horizon = 2.0 * mesh_size;

  const double Lin = 0.05;
  const double Win = 0.04;
  const double L = Lin + 1.5 * horizon;
  const double W = Win + 1.5 * horizon;

  const std::vector<double> in_rect = {center[0] - 0.5 * Lin, center[1] - 0.5 * Win, center[2],
                                       center[0] + 0.5 * Lin, center[1] + 0.5 * Win, center[2]};
  const std::vector<double> out_rect = {center[0] - 0.5 * L, center[1] - 0.5 * W, center[2],
                                        center[0] + 0.5 * L, center[1] + 0.5 * W, center[2]};

  const double moving_wall_y = 0.5 * Win - 1.5 * horizon;
  std::vector<double> moving_rect = {center[0] - 0.5 * Lin, center[1] + moving_wall_y, center[2],
                                     center[0] + 0.5 * Lin, center[1] + moving_wall_y + 1.5 * horizon, center[2]};

  std::vector<double> remove_rect = {in_rect[0], in_rect[1], in_rect[2], in_rect[3], out_rect[4], in_rect[5]};
  if (moving_rect[4] > out_rect[4])
    remove_rect[4] = out_rect[4];

  /* Builtin Gmsh annulus requires the inner rectangle strictly inside the outer (annulusMesh2D.cpp). */
  constexpr double annulus_inset = 1e-7;
  if (remove_rect[4] >= out_rect[4] - annulus_inset)
    remove_rect[4] = out_rect[4] - annulus_inset;

  /* C++ AnnulusGeomObject: inner (hole) first, outer second — see geomObjectsUtil.cpp */
  std::vector<double> fixed_container_params;
  fixed_container_params.insert(fixed_container_params.end(), remove_rect.begin(), remove_rect.end());
  fixed_container_params.insert(fixed_container_params.end(), out_rect.begin(), out_rect.end());

  const double w_small_drum2d = R_small * 0.2;
  const double w_large_drum2d = R_large * 0.2;

  const double final_time = 0.01;
  size_t num_steps = 4000;
  if (input.cmdOptionExists("-numSteps"))
    num_steps = static_cast<size_t>(std::stoul(input.getCmdOption("-numSteps")));

  const size_t num_outputs = 10;
  const size_t dt_out_n = num_steps / num_outputs;
  const size_t test_dt_out_n = dt_out_n / 10;

  const double rho_wall = 600.;
  const double poisson_wall = 0.25;
  const double K_wall = 1.e+4;
  const double E_wall = material::toE(K_wall, poisson_wall);
  const double G_wall = material::toGE(E_wall, poisson_wall);
  const double Gc_wall = 100.;

  const double rho_small = 600.;
  const double poisson_small = poisson_wall;
  const double K_small = 5.e+3;
  const double E_small = material::toE(K_small, poisson_small);
  const double G_small = material::toGE(E_small, poisson_small);
  const double Gc_small = 100.;

  const double rho_large = rho_small;
  const double poisson_large = poisson_small;
  const double K_large = K_small;
  const double E_large = E_small;
  const double G_large = G_small;
  const double Gc_large = Gc_small;

  const double R_contact_factor = 0.95;
  const double padding = 1.1 * R_contact_factor * mesh_size;
  const double max_y = moving_wall_y - 3.0 * mesh_size;
  const int N1 = 300;
  const int N2 = 200;

  std::mt19937 gen(30);
  std::vector<PackedParticle> packed =
      generateParticleLocations(in_rect, max_y, mesh_size, R_small, R_large, N1, N2, padding, gen);

  const size_t n_pack = packed.size();
  const size_t n_wall_fixed = n_pack;
  const size_t n_wall_moving = n_pack + 1;
  const size_t n_total = n_pack + 2;

  std::vector<geom::GeomData> pGeomVec(10);
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R_small, center[0], center[1], center[2]};
  pGeomVec[1].d_geomName = "triangle";
  pGeomVec[1].d_geomParams = {R_small, center[0], center[1], center[2]};
  pGeomVec[2].d_geomName = "drum2d";
  pGeomVec[2].d_geomParams = {R_small, w_small_drum2d, center[0], center[1], center[2]};
  pGeomVec[3].d_geomName = "hexagon";
  pGeomVec[3].d_geomParams = {R_small, center[0], center[1], center[2]};
  pGeomVec[4].d_geomName = "circle";
  pGeomVec[4].d_geomParams = {R_large, center[0], center[1], center[2]};
  pGeomVec[5].d_geomName = "triangle";
  pGeomVec[5].d_geomParams = {R_large, center[0], center[1], center[2]};
  pGeomVec[6].d_geomName = "drum2d";
  pGeomVec[6].d_geomParams = {R_large, w_large_drum2d, center[0], center[1], center[2]};
  pGeomVec[7].d_geomName = "hexagon";
  pGeomVec[7].d_geomParams = {R_large, center[0], center[1], center[2]};
  pGeomVec[8].d_geomName = "rectangle_minus_rectangle";
  pGeomVec[8].d_geomParams = fixed_container_params;
  pGeomVec[9].d_geomName = "rectangle";
  pGeomVec[9].d_geomParams = moving_rect;

  for (auto &g : pGeomVec)
    geom::createGeomObject(g);

  const util::Point site_wall_fixed = pGeomVec[8].d_geom_p->center();
  const util::Point site_wall_moving = pGeomVec[9].d_geom_p->center();

  auto modelDeckJson = inp::ModelDeck::getExampleJson(2, final_time, num_steps, "finite_difference",
                                                        "central_difference", true, 2, "Multi_Particle", 0);

  std::vector<std::string> out_tags = {"Displacement", "Velocity", "Force", "Force_Density", "Damage_Z",
                                         "Damage",      "Nodal_Volume", "Zone_ID", "Particle_ID", "Fixity",
                                         "Force_Fixity", "Contact_Nodes", "No_Fail_Node", "Boundary_Node_Flag"};
  /* Perform_FE_Out must be true so VTU includes element connectivity (appendMesh). If false, only
   * points are written (appendNodes) and ParaView’s default “Surface” view looks empty — use
   * Representation → Points, or enable this flag. problem_setup.yaml used false for huge runs. */
  auto outputDeckJson = inp::OutputDeck::getExampleJson("vtu", output_path_for_deck, out_tags, dt_out_n, 2,
                                                          false, "zlib", true, test_dt_out_n, "0", true);

  auto bcDeckJson = inp::BCDeck::getExampleJson(0, 2, 0, true, util::Point(0, -10, 0));

  bcDeckJson["Displacement_BC"]["Set_1"] = inp::BCBaseDeck::getExampleJson("Displacement_BC", false, geom::GeomData(),
                                                                           {n_wall_fixed}, {}, "", {}, "", {},
                                                                           {1, 2}, true, "", {});

  json set2;
  set2["Particle_List"] = std::vector<size_t>{n_wall_moving};
  set2["Direction"] = std::vector<size_t>{2};
  set2["Time_Function"] = json{{"Type", "linear"}, {"Parameters", std::vector<double>{-0.06}}};
  set2["Spatial_Function"] = json{{"Type", "constant"}};
  bcDeckJson["Displacement_BC"]["Set_2"] = set2;

  json pDeckJson = json::object();

  pDeckJson["Particle"] = inp::ParticleDeck::getParticleGeomExampleJson(pGeomVec);

  json meshRoot = json{{"Sets", 10}};
  const char *mesh_names[] = {"mesh_cir_small",   "mesh_tri_small",   "mesh_drum2d_small", "mesh_hex_small",
                              "mesh_cir_large",   "mesh_tri_large",   "mesh_drum2d_large", "mesh_hex_large",
                              "mesh_fixed_container", "mesh_moving_container"};
  for (int zi = 0; zi < 10; ++zi) {
    const std::string fname = (inp_dir / (std::string(mesh_names[zi]) + ".msh")).string();
    meshRoot["Set_" + std::to_string(zi + 1)] =
        json{{"Mesh_Size", mesh_size},
             {"File", fname},
             {"CreateMesh", json{{"Flag", true}, {"Info", "gmsh_builtin_mesh"}, {"Write_Mesh_File", true}}}};
  }
  pDeckJson["Mesh"] = meshRoot;

  json matRoot = json{{"Sets", 10}};
  matRoot["Set_1"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho_small, K_small, G_small,
                                                       Gc_small, true, 1);
  matRoot["Set_2"] = json{{"Copy_Data", 1}};
  matRoot["Set_3"] = json{{"Copy_Data", 1}};
  matRoot["Set_4"] = json{{"Copy_Data", 1}};
  matRoot["Set_5"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho_large, K_large, G_large,
                                                       Gc_large, true, 1);
  matRoot["Set_6"] = json{{"Copy_Data", 5}};
  matRoot["Set_7"] = json{{"Copy_Data", 5}};
  matRoot["Set_8"] = json{{"Copy_Data", 5}};
  matRoot["Set_9"] = inp::MaterialDeck::getExampleJson("PDState", false, horizon, 0, rho_wall, K_wall, G_wall,
                                                       Gc_wall, true, 1);
  matRoot["Set_10"] = json{{"Copy_Data", 9}};
  pDeckJson["Material"] = matRoot;

  const double Kn_ss = KnFromBulk(K_small, K_small, horizon);
  const double Kn_ll = KnFromBulk(K_large, K_large, horizon);
  const double Kn_sl = KnFromBulk(K_small, K_large, horizon);
  const double Kn_sw = KnFromBulk(K_small, K_wall, horizon);
  const double Kn_lw = KnFromBulk(K_large, K_wall, horizon);
  const double Kn_ww = 0.0;

  const double beta_n_eps = 0.95;
  const double friction_coeff = 0.5;
  const bool damping_on = false;
  const bool friction_on = false;
  const double beta_n_factor = 100.;
  const double Kn_factor = 1.;

  auto KnForPair = [&](int i, int j) -> double {
    auto is_s = [](int z) { return z >= 0 && z <= 3; };
    auto is_l = [](int z) { return z >= 4 && z <= 7; };
    auto is_w = [](int z) { return z >= 8; };
    if (is_w(i) && is_w(j))
      return Kn_ww;
    if (is_s(i) && is_s(j))
      return Kn_ss;
    if (is_l(i) && is_l(j))
      return Kn_ll;
    if ((is_s(i) && is_l(j)) || (is_l(i) && is_s(j)))
      return Kn_sl;
    if ((is_s(i) && is_w(j)) || (is_w(i) && is_s(j)))
      return Kn_sw;
    if ((is_l(i) && is_w(j)) || (is_w(i) && is_l(j)))
      return Kn_lw;
    return Kn_ss;
  };

  json contactRoot = inp::ContactDeck::getExampleJson(10);
  for (int i = 0; i < 10; ++i) {
    for (int k = i; k < 10; ++k) {
      const std::string name = "Set_" + std::to_string(i + 1) + "_" + std::to_string(k + 1);
      const double Kn = KnForPair(i, k);
      contactRoot[name] =
          contactPairJson(R_contact_factor, damping_on, friction_on, Kn, beta_n_eps, friction_coeff, Kn_factor,
                          beta_n_factor);
    }
  }
  pDeckJson["Contact"] = contactRoot;

  pDeckJson["Neighbor"] = inp::PNeighborDeck::getExampleJson("simple_all", 10.0, 100, 0.5);

  auto pGenJson = inp::PGenDeck::getExampleJson("From_File");
  pGenJson["Random_Rotation"] = false;
  pGenJson["Data"]["N"] = n_total;

  for (size_t pi = 0; pi < n_pack; ++pi) {
    const auto &p = packed[pi];
    pGenJson["Data"][std::to_string(pi)] = json{
        {"x", p.x},         {"y", p.y},         {"z", p.z},         {"theta", p.theta},
        {"s", 1.0},         {"geom_id", static_cast<size_t>(p.zone)},
        {"mat_id", static_cast<size_t>(p.zone)},
        {"contact_id", static_cast<size_t>(p.zone)},
    };
  }

  pGenJson["Data"][std::to_string(n_pack)] = json{{"x", site_wall_fixed.d_x},
                                                  {"y", site_wall_fixed.d_y},
                                                  {"z", site_wall_fixed.d_z},
                                                  {"theta", 0.0},
                                                  {"s", 1.0},
                                                  {"geom_id", size_t(8)},
                                                  {"mat_id", size_t(8)},
                                                  {"contact_id", size_t(8)}};

  pGenJson["Data"][std::to_string(n_pack + 1)] = json{{"x", site_wall_moving.d_x},
                                                       {"y", site_wall_moving.d_y},
                                                       {"z", site_wall_moving.d_z},
                                                       {"theta", 0.0},
                                                       {"s", 1.0},
                                                       {"geom_id", size_t(9)},
                                                       {"mat_id", size_t(9)},
                                                       {"contact_id", size_t(9)}};

  pDeckJson["Particle_Generation"] = pGenJson;

  return json{{"Model", modelDeckJson},
              {"Output", outputDeckJson},
              {"Force_BC", bcDeckJson["Force_BC"]},
              {"Displacement_BC", bcDeckJson["Displacement_BC"]},
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

  const std::string output_path_for_deck = directoryPathWithTrailingSep(out_dir);

  util::io::print(std::format("Output directory (VTU, log.txt): {}\n", fs::absolute(out_dir).string()));
  util::io::print(std::format("Input directory (input.json, meshes): {}\n", fs::absolute(inp_dir).string()));

  auto inputJson = buildInputJson(output_path_for_deck, inp_dir, argc, argv);

  const fs::path input_json_path = inp_dir / "input.json";
  {
    std::ofstream os(input_json_path);
    if (!os)
      throw std::runtime_error("Failed to open " + input_json_path.string() + " for writing.");
    os << inputJson.dump(2);
  }
  util::io::print(std::format("Wrote deck to {}\n", fs::absolute(input_json_path).string()));

  auto deck = std::make_shared<inp::Input>(inputJson);

  model::DEMModel dem(deck);
  dem.run(deck);

  return EXIT_SUCCESS;
}
