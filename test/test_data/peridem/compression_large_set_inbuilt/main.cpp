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
 * Random row-wise packing (same spirit as problem_setup.py method 1) with std::mt19937(30).
 * One characteristic radius R; zones 0–7 pick among eight geometry types.
 */
std::vector<PackedParticle> generateParticleLocations(const std::vector<double> &in_rect, double max_y,
                                                      double mesh_size, double R, int N_target,
                                                      double padding, std::mt19937 &gen) {
  std::vector<PackedParticle> particles;
  std::uniform_real_distribution<double> u_r(-0.1 * R, 0.1 * R);
  std::uniform_int_distribution<int> pick_zone(0, 7);

  const double check_r = R;
  const int rows = static_cast<int>((max_y - in_rect[1]) / (2.0 * check_r));
  const double rect_L = in_rect[3] - in_rect[0];
  const int cols = static_cast<int>(rect_L / (2.0 * check_r));

  int counter = 0;
  double x_old = in_rect[0];
  double x_old_right = in_rect[3];
  double y_old = in_rect[1];
  const double cz = 0.0;

  std::vector<double> cy_accptd;
  cy_accptd.push_back(y_old);

  std::vector<double> row_rads_prev{R, R};
  for (int i = 0; i < rows; ++i) {
    if (i > 0)
      y_old = maxElem(cy_accptd) + maxElem(row_rads_prev);

    std::vector<double> row_rads{R};

    if (y_old + padding + maxElem(row_rads) >= max_y)
      break;

    int num_p_cols = 0;
    int j = 0;
    while (true) {
      if (num_p_cols > cols - 1 || j > 100 * N_target)
        break;
      if (counter >= N_target)
        break;

      if (j == 0) {
        x_old = in_rect[0];
        x_old_right = in_rect[3];
      }

      const int p_zone = pick_zone(gen);
      const double r0 = R;
      double r = r0 + u_r(gen);

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

        ++counter;
        num_p_cols++;
      }
      ++j;
    }
    row_rads_prev = std::move(row_rads);
  }
  return particles;
}

/** Axis-aligned bounds of the pack using the same center±r proxy as packing / contact. */
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
  const double R = 0.001;
  const double mesh_size = R / 5.0;
  const double horizon = 2.0 * mesh_size;

  const double Lin = 0.05;
  const double Win = 0.04;

  constexpr double wall_top_inset = 1e-7;
  constexpr double annulus_inset = 1e-7;
  /* Clearance between void floor / plate and particle circum-bounds (~1–2 mesh). */
  constexpr double clearance_mesh = 1.5;
  /* Meshed triangles/hex/drum can extend slightly beyond circumcircle r in VTU. */
  const double geom_pad = 0.25 * mesh_size;
  const double plate_thickness = std::max(3.0 * mesh_size, 2.0 * mesh_size);

  const double w_drum2d = R * 0.2;

  const double final_time = 0.001;
  size_t num_steps = 400;
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

  const double rho_p = 600.;
  const double poisson_p = poisson_wall;
  const double K_p = 5.e+3;
  const double E_p = material::toE(K_p, poisson_p);
  const double G_p = material::toGE(E_p, poisson_p);
  const double Gc_p = 100.;

  const double R_contact_factor = 0.95;
  const double padding = 1.1 * R_contact_factor * mesh_size;
  const int N_target = 500;

  const std::vector<double> in_rect = {center[0] - 0.5 * Lin, center[1] - 0.5 * Win, center[2],
                                       center[0] + 0.5 * Lin, center[1] + 0.5 * Win, center[2]};

  std::mt19937 gen(30);
  const double max_y = in_rect[4] - clearance_mesh * mesh_size;
  std::vector<PackedParticle> packed =
      generateParticleLocations(in_rect, max_y, mesh_size, R, N_target, padding, gen);
  if (packed.empty())
    throw std::runtime_error("compression_large_set_inbuilt_2mat2contact: particle pack is empty");
  for (auto &p : packed)
    p.z = 0.0;

  const double m = clearance_mesh * mesh_size;
  /* Seat bed: lowest circum-bottom = void_floor + m (void floor = seed inner bottom). */
  {
    const PackedBounds bb0 = packedAxisBounds(packed);
    const double delta_y = (in_rect[1] + m) - bb0.min_y;
    for (auto &p : packed)
      p.y += delta_y;
  }

  const PackedBounds bb = packedAxisBounds(packed);

  /* Inner void (Gmsh “remove” rectangle): circum-bounds ± m ± geom_pad in x; floor at seed; top = domain. */
  const double void_lox = bb.min_x - m - geom_pad;
  const double void_hix = bb.max_x + m + geom_pad;
  const double void_loy = in_rect[1];
  /* Moving plate: bottom = top of bed + clearance + pad so FE nodes stay under the plate. */
  const double plate_bottom_y = bb.max_y + m + geom_pad;
  double plate_top_y = plate_bottom_y + plate_thickness;

  /* Outer solid: wrap void with wall thickness; extend upward just past the plate (no huge empty band). */
  const double side = std::max(1.5 * horizon, 2.0 * mesh_size);
  const double out_lox = void_lox - side;
  const double out_hix = void_hix + side;
  const double out_loy = void_loy - side;
  const double out_hiy = plate_top_y + wall_top_inset + 2.5 * mesh_size;

  std::vector<double> out_rect = {out_lox, out_loy, center[2], out_hix, out_hiy, center[2]};

  /* Open-top void: inner cut runs to outer top (inset for Gmsh strict-inside). */
  const double void_hi_y = out_rect[4] - annulus_inset;
  std::vector<double> remove_rect = {void_lox, void_loy, center[2], void_hix, void_hi_y, center[2]};

  util::io::print(std::format(
      "[compression_large_set_inbuilt_2mat2contact] bbox(center±r) x∈[{:.6f},{:.6f}] y∈[{:.6f},{:.6f}]; "
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

  /* Reference geometries at origin; packing uses circum-radius ~ R for all zones. */
  const double ell_a = 0.95 * R;
  const double ell_b = 0.58 * R;
  const double ell_theta = 0.35;
  const double sq_half = R / std::sqrt(2.0);
  /* Axis-aligned rectangle (~same circum extent as R). */
  const double rx = 0.82 * R;
  const double ry = 0.62 * R;

  std::vector<geom::GeomData> pGeomVec(10);
  pGeomVec[0].d_geomName = "circle";
  pGeomVec[0].d_geomParams = {R, center[0], center[1], center[2]};
  pGeomVec[1].d_geomName = "triangle";
  pGeomVec[1].d_geomParams = {R, center[0], center[1], center[2]};
  pGeomVec[2].d_geomName = "drum2d";
  pGeomVec[2].d_geomParams = {R, w_drum2d, center[0], center[1], center[2]};
  pGeomVec[3].d_geomName = "hexagon";
  pGeomVec[3].d_geomParams = {R, center[0], center[1], center[2]};
  pGeomVec[4].d_geomName = "ellipse";
  pGeomVec[4].d_geomParams = {ell_a, ell_b, ell_theta, center[0], center[1], center[2]};
  pGeomVec[5].d_geomName = "rectangle";
  pGeomVec[5].d_geomParams = {-rx, -ry, center[2], rx, ry, center[2]};
  pGeomVec[6].d_geomName = "square";
  pGeomVec[6].d_geomParams = {-sq_half, -sq_half, center[2], sq_half, sq_half, center[2]};
  pGeomVec[7].d_geomName = "circle_minus_circle";
  pGeomVec[7].d_geomParams = {center[0], center[1], center[2], R, 0.35 * R};
  pGeomVec[8].d_geomName = "rectangle_minus_rectangle";
  pGeomVec[8].d_geomParams = fixed_container_params;
  pGeomVec[9].d_geomName = "rectangle";
  pGeomVec[9].d_geomParams = moving_rect;

  for (auto &g : pGeomVec)
    geom::createGeomObject(g);

  /* Annulus composite centroid can pick up tiny numerical z; 2D setup keeps all sites on z = 0. */
  const util::Point cfix = pGeomVec[8].d_geom_p->center();
  const util::Point cmov = pGeomVec[9].d_geom_p->center();
  const util::Point site_wall_fixed(cfix.d_x, cfix.d_y, 0.0);
  const util::Point site_wall_moving(cmov.d_x, cmov.d_y, 0.0);

  auto modelDeckJson = inp::ModelDeck::getExampleJson(2, final_time, num_steps, "finite_difference",
                                                        "central_difference", true, 2, "Multi_Particle", 0);

  std::vector<std::string> out_tags = {"Displacement", "Velocity", "Force", "Force_Density", "Damage_Z",
                                         "Damage",      "Nodal_Volume", "Zone_ID", "Particle_ID", "Fixity",
                                         "Force_Fixity", "Contact_Nodes", "No_Fail_Node", "Boundary_Node_Flag"};
  /* Perform_FE_Out must be true so VTU includes element connectivity (appendMesh). If false, only
   * points are written (appendNodes) and ParaView’s default “Surface” view looks empty — use
   * Representation → Points, or enable this flag. problem_setup.yaml used false for huge runs. */
  auto outputDeckJson = inp::OutputDeck::getExampleJson("vtu", output_path_for_deck, out_tags, dt_out_n, 2,
                                                          true, "zlib", true, test_dt_out_n, "0", true);

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
  const char *mesh_names[] = {"mesh_cir",      "mesh_tri",      "mesh_drum2d",   "mesh_hex",
                              "mesh_ellipse",  "mesh_rect",     "mesh_square",   "mesh_circirc",
                              "mesh_fixed_container", "mesh_moving_container"};
  for (int zi = 0; zi < 10; ++zi) {
    const std::string fname = (inp_dir / (std::string(mesh_names[zi]) + ".msh")).string();
    meshRoot["Set_" + std::to_string(zi + 1)] =
        json{{"File", fname},
             {"CreateMesh",
              json{{"Flag", true}, {"Info", "gmsh_builtin_mesh"}, {"Mesh_Size", mesh_size}, {"Write_Mesh_File", true}}}};
  }
  pDeckJson["Mesh"] = meshRoot;

  /* Two material laws: 0 = particle, 1 = wall (fixed + moving). geom_id still selects mesh shape (10 meshes). */
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
  const double beta_n_factor = 100.;
  const double Kn_factor = 1.;

  /* Contact zones: 0 = granular, 1 = wall (fixed + moving). 2×2 matrix → three unique pairs (1-1, 1-2, 2-2). */
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
    pGenJson["Data"][std::to_string(pi)] = json{
        {"x", p.x},         {"y", p.y},         {"z", 0.0},         {"theta", p.theta},
        {"s", 1.0},         {"geom_id", static_cast<size_t>(p.zone)},
        {"mat_id", k_mat_particle},
        {"contact_id", k_contact_grains},
    };
  }

  pGenJson["Data"][std::to_string(n_pack)] = json{{"x", site_wall_fixed.d_x},
                                                  {"y", site_wall_fixed.d_y},
                                                  {"z", 0.0},
                                                  {"theta", 0.0},
                                                  {"s", 1.0},
                                                  {"geom_id", size_t(8)},
                                                  {"mat_id", k_mat_wall},
                                                  {"contact_id", k_contact_wall}};

  pGenJson["Data"][std::to_string(n_pack + 1)] = json{{"x", site_wall_moving.d_x},
                                                       {"y", site_wall_moving.d_y},
                                                       {"z", 0.0},
                                                       {"theta", 0.0},
                                                       {"s", 1.0},
                                                       {"geom_id", size_t(9)},
                                                       {"mat_id", k_mat_wall},
                                                       {"contact_id", k_contact_wall}};

  pDeckJson["Particle_Generation"] = pGenJson;

  /* Top-level comment only: ignored by inp::Input (only known sections are parsed). */
  return json{{"Comment",
               "compression_large_set_inbuilt"},
              {"Model", modelDeckJson},
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
