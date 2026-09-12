/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * Notched plate + rectangular impactor (Kalthoff-style). Prenotch bonds are
 * removed after init. Checks: tip damage outward and down, no blow-up.
 */

#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "periDEMModel.h"
#include "util/function.h"
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

bool segmentCrossesVertical(double x0, double y0, double x1, double y1, double x_line,
                            double y_lo, double y_hi) {
  if ((x0 - x_line) * (x1 - x_line) >= 0.)
    return false;
  const double t = (x_line - x0) / (x1 - x0);
  if (t < 0. || t > 1.)
    return false;
  const double y = y0 + t * (y1 - y0);
  return y >= y_lo && y <= y_hi;
}

size_t applyNotches(PeriDEMModel &dem, double x_left, double x_right, double y_lo,
                    double y_hi) {
  size_t n_broken = 0;
  for (size_t i = 0; i < dem.d_neighPd.size(); ++i) {
    if (dem.d_ptId[i] != 0)
      continue;
    const auto &xi = dem.d_xRef[i];
    for (size_t k = 0; k < dem.d_neighPd[i].size(); ++k) {
      const size_t j = dem.d_neighPd[i][k];
      if (dem.d_ptId[j] != 0)
        continue;
      const auto &xj = dem.d_xRef[j];
      if (segmentCrossesVertical(xi.d_x, xi.d_y, xj.d_x, xj.d_y, x_left, y_lo, y_hi) ||
          segmentCrossesVertical(xi.d_x, xi.d_y, xj.d_x, xj.d_y, x_right, y_lo, y_hi)) {
        dem.d_fracture_p->setBondState(i, k, true);
        ++n_broken;
      }
    }
  }
  return n_broken;
}

struct CrackFit {
  size_t n_pts = 0;
  double angle_deg = 0.;
  double mean_dx = 0.;
  double mean_dy = 0.;
};

CrackFit fitCrackAngle(const PeriDEMModel &dem, double tip_x, double tip_y,
                       double outward_sign, double z_cut, double band_x,
                       double band_y, double exclude_r) {
  CrackFit fit;
  std::vector<double> xs, ys;
  for (size_t i = 0; i < dem.d_xRef.size(); ++i) {
    if (dem.d_ptId[i] != 0)
      continue;
    if (dem.d_Z[i] < z_cut)
      continue;
    const auto &p = dem.d_xRef[i];
    const double dx = p.d_x - tip_x;
    const double dy = tip_y - p.d_y;
    if (dy < exclude_r || dy > band_y)
      continue;
    if (outward_sign * dx < exclude_r)
      continue;
    if (std::abs(dx) > band_x)
      continue;
    xs.push_back(dx);
    ys.push_back(dy);
  }
  fit.n_pts = xs.size();
  if (fit.n_pts < 4)
    return fit;
  double mx = 0., my = 0.;
  for (size_t i = 0; i < fit.n_pts; ++i) {
    mx += xs[i];
    my += ys[i];
  }
  mx /= static_cast<double>(fit.n_pts);
  my /= static_cast<double>(fit.n_pts);
  fit.mean_dx = mx;
  fit.mean_dy = my;
  double cxx = 0., cxy = 0., cyy = 0.;
  for (size_t i = 0; i < fit.n_pts; ++i) {
    const double x = xs[i] - mx;
    const double y = ys[i] - my;
    cxx += x * x;
    cxy += x * y;
    cyy += y * y;
  }
  const double trace = cxx + cyy;
  const double det = cxx * cyy - cxy * cxy;
  const double tmp = std::sqrt(std::max(0., 0.25 * trace * trace - det));
  const double l1 = 0.5 * trace + tmp;
  double vx = cxy;
  double vy = l1 - cxx;
  if (vx * vx + vy * vy < 1.e-30) {
    vx = l1 - cyy;
    vy = cxy;
  }
  fit.angle_deg = std::atan2(std::abs(vy), std::abs(vx)) * 180. / M_PI;
  return fit;
}

json buildInputJson(const std::string &output_path,
                    const std::filesystem::path &mesh_plate,
                    const std::filesystem::path &mesh_impactor, double W, double H,
                    double Iw, double Ih, double gap, double mesh_size, double horizon,
                    double Rc_factor, double Kn, double rho, double K, double G, double Gc,
                    double v_impact, double final_time, size_t num_steps) {
  auto model = inp::ModelDeck::getExampleJson(2, final_time, num_steps, "finite_difference",
                                              "central_difference", true, 2, "Multi_Particle",
                                              0);
  model["Self_Contact"] = "reference_gap";
  model["Bond_Break"] = "absolute_stretch";
  model["Wall_Contact"] = "meshed";

  auto output = inp::OutputDeck::getExampleJson(
      "vtu", output_path,
      std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage_Z", "Particle_ID"}),
      std::max<size_t>(1, num_steps / 10), 1, false, "zlib", true, num_steps, "", false);

  // Fix top outer ligaments (left / right of the two notches).
  const double notch_half = 0.125 * W; // tip x = ±notch_half (scaled KW spacing)
  const double fix_h = 0.08 * H;
  json bc = json::object();
  bc["Displacement_BC"] = {
      {"Sets", 2},
      {"Set_1",
       {{"Particle_List", std::vector<size_t>{0}},
        {"Region",
         {{"Geometry",
           {{"Type", "rectangle"},
            {"Parameters",
             std::vector<double>{-0.55 * W, 0.5 * H - fix_h, 0., -notch_half - 0.02 * W,
                                 0.55 * H, 0.}}}}}},
        {"Direction", std::vector<size_t>{1, 2}},
        {"Time_Function", {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
        {"Spatial_Function", {{"Type", "constant"}}},
        {"Zero_Displacement", true}}},
      {"Set_2",
       {{"Particle_List", std::vector<size_t>{0}},
        {"Region",
         {{"Geometry",
           {{"Type", "rectangle"},
            {"Parameters",
             std::vector<double>{notch_half + 0.02 * W, 0.5 * H - fix_h, 0., 0.55 * W, 0.55 * H,
                                 0.}}}}}},
        {"Direction", std::vector<size_t>{1, 2}},
        {"Time_Function", {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
        {"Spatial_Function", {{"Type", "constant"}}},
        {"Zero_Displacement", true}}}};
  bc["IC"] = {
      {"Sets", 1},
      {"Set_1",
       {{"Particle_List", std::vector<size_t>{1}},
        {"Constant_Velocity",
         {{"Velocity_Vector", std::vector<double>{0., -v_impact, 0.}}}}}}};

  geom::GeomData plate;
  plate.d_geomName = "rectangle";
  plate.d_geomParams = {-0.5 * W, -0.5 * H, 0., 0.5 * W, 0.5 * H, 0.};
  geom::GeomData impactor;
  impactor.d_geomName = "rectangle";
  impactor.d_geomParams = {-0.5 * Iw, -0.5 * Ih, 0., 0.5 * Iw, 0.5 * Ih, 0.};
  auto particle = inp::ParticleDeck::getParticleGeomExampleJson({plate, impactor});

  auto mesh_set = [&](const std::filesystem::path &f) {
    return json{{"File", f.string()},
                {"CreateMesh",
                 {{"Flag", true},
                  {"Info", "gmsh_builtin_mesh"},
                  {"Mesh_Size", mesh_size},
                  {"Write_Mesh_File", true}}}};
  };
  json mesh = {{"Sets", 2},
               {"Set_1", mesh_set(mesh_plate)},
               {"Set_2", mesh_set(mesh_impactor)}};

  auto material = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  material["Set_1"] =
      inp::MaterialDeck::getExampleJson("PMBBond", false, horizon, 0, rho, K, G, Gc, true, 1);
  material["Set_2"] =
      inp::MaterialDeck::getExampleJson("PMBBond", false, horizon, 0, rho, K, G, Gc, true, 1);

  json contact_base = inp::ContactPairDeck::getExampleJson(
      Rc_factor, true, /*damping*/ false, /*friction*/ false, Kn, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0,
      K);
  contact_base["Kn"] = Kn;
  json contact = inp::ParticleDeck::getParticleContactExampleJson(2);
  contact["Set_1_1"] = contact_base;
  contact["Set_1_2"] = contact_base;
  contact["Set_2_2"] = contact_base;
  contact["Damping_Law"] = "off";
  contact["Friction_Law"] = "coulomb_simple";

  const double cy_imp = 0.5 * H + 0.5 * Ih + gap;
  auto pgen = inp::PGenDeck::getExampleJson("From_File");
  pgen["Random_Rotation"] = false;
  pgen["Data"]["N"] = 2;
  pgen["Data"]["0"] = json{{"x", 0.},
                           {"y", 0.},
                           {"z", 0.},
                           {"theta", 0.},
                           {"s", 1.},
                           {"geom_id", 0},
                           {"mat_id", 0},
                           {"contact_id", 0}};
  pgen["Data"]["1"] = json{{"x", 0.},
                           {"y", cy_imp},
                           {"z", 0.},
                           {"theta", 0.},
                           {"s", 1.},
                           {"geom_id", 1},
                           {"mat_id", 1},
                           {"contact_id", 1}};

  return json{{"Model", model},
              {"Output", output},
              {"Displacement_BC", bc["Displacement_BC"]},
              {"IC", bc["IC"]},
              {"Particle", particle},
              {"Mesh", mesh},
              {"Material", material},
              {"Contact", contact},
              {"Neighbor", inp::PNeighborDeck::getExampleJson("simple_all", 5.0, 1, 0.5)},
              {"Particle_Generation", pgen}};
}

} // namespace

int main(int argc, char *argv[]) {
  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);

  unsigned n_threads = 4;
  if (input.cmdOptionExists("-nThreads"))
    n_threads = static_cast<unsigned>(std::stoi(input.getCmdOption("-nThreads")));
  util::parallel::initNThreads(n_threads);

  namespace fs = std::filesystem;
  fs::path base = fs::current_path() / "notched_impact_run";
  if (input.cmdOptionExists("-outputDir"))
    base = input.getCmdOption("-outputDir");
  const fs::path out_dir = base / "out";
  const fs::path inp_dir = base / "inp";
  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);

  // 1/5 scale of classic KW outline (200 mm × 100 mm).
  const double W = 0.040;
  const double H = 0.020;
  const double notch_depth = 0.5 * H;
  const double notch_half = 0.125 * W; // tip-to-tip = 0.25 W (50 mm @ full scale)
  const double Iw = 0.008;
  const double Ih = 0.004;
  const double gap = 0.0004;
  // Keep horizon well below tip-to-tip spacing so prenotch does not wipe the ligament.
  const double mesh_size = H / 16.0; // 1.25 mm
  const double horizon = 3.0 * mesh_size; // 3.75 mm < 10 mm tip spacing
  const double Rc_factor = 0.95;

  // Soften moduli vs steel M1 so a coarse CI mesh can grow tip damage without
  // immediate plate-wide blow-up.
  const double rho = 1200.0;
  const double E = 1.23e9;
  const double Kbulk = 2.0e9;
  const double nu = 0.5 * (1.0 - E / (3.0 * Kbulk));
  const double G = E / (2.0 * (1.0 + nu));
  const double Gc = 424.0;
  const double Kn = 18.0 * util::harmonicMean(Kbulk, Kbulk) / (M_PI * std::pow(horizon, 5));
  const double v_impact = 32.0;
  const double dt = 1.0e-8;
  const double final_time = 1.0e-4;
  const size_t num_steps = static_cast<size_t>(std::llround(final_time / dt));

  auto input_json =
      buildInputJson(directoryPathWithTrailingSep(out_dir), inp_dir / "mesh_plate.msh",
                     inp_dir / "mesh_impactor.msh", W, H, Iw, Ih, gap, mesh_size, horizon,
                     Rc_factor, Kn, rho, Kbulk, G, Gc, v_impact, final_time, num_steps);
  {
    std::ofstream os(inp_dir / "input.json");
    os << input_json.dump(2);
  }

  auto deck = std::make_shared<inp::Input>(input_json);
  PeriDEMModel dem(deck);
  dem.init();

  const double y_top = 0.5 * H;
  const double y_tip = y_top - notch_depth;
  const size_t n_pre = applyNotches(dem, -notch_half, notch_half, y_tip, y_top + 0.01 * H);
  if (n_pre < 10) {
    std::cerr << "notched_impact: expected prenotch bonds, got " << n_pre << "\n";
    return 1;
  }

  dem.integrate();

  double vmax = 0., vmax0 = 0., vmax1 = 0., zmax = 0.;
  for (size_t i = 0; i < dem.d_v.size(); ++i) {
    const double s = dem.d_v[i].length();
    vmax = std::max(vmax, s);
    if (dem.d_ptId[i] == 0)
      vmax0 = std::max(vmax0, s);
    else
      vmax1 = std::max(vmax1, s);
    zmax = std::max(zmax, static_cast<double>(dem.d_Z[i]));
  }
  double min_gap = 1.e300;
  for (size_t i = 0; i < dem.d_x.size(); ++i) {
    if (dem.d_ptId[i] != 0)
      continue;
    for (size_t j = 0; j < dem.d_x.size(); ++j) {
      if (dem.d_ptId[j] != 1)
        continue;
      min_gap = std::min(min_gap, (dem.d_x[j] - dem.d_x[i]).length());
    }
  }
  std::cout << std::format(
      "notched_impact diag: prenotch={} max|v|={:.3e} (plate={:.3e} imp={:.3e}) "
      "maxZ={:.3e} min_gap_ij={:.3e} Rc={:.3e}\n",
      n_pre, vmax, vmax0, vmax1, zmax, min_gap, dem.d_maxContactR);

  if (!(vmax < 5.0e3) || !std::isfinite(vmax)) {
    std::cerr << "notched_impact: blow-up, max |v| = " << vmax << "\n";
    return 1;
  }

  size_t n_extra = 0;
  for (size_t i = 0; i < dem.d_neighPd.size(); ++i) {
    if (dem.d_ptId[i] != 0)
      continue;
    for (size_t k = 0; k < dem.d_neighPd[i].size(); ++k) {
      if (!dem.d_fracture_p->getBondState(i, k))
        continue;
      const size_t j = dem.d_neighPd[i][k];
      if (dem.d_ptId[j] != 0)
        continue;
      const auto &xi = dem.d_xRef[i];
      const auto &xj = dem.d_xRef[j];
      const bool on_notch =
          segmentCrossesVertical(xi.d_x, xi.d_y, xj.d_x, xj.d_y, -notch_half, y_tip,
                                 y_top + 0.01 * H) ||
          segmentCrossesVertical(xi.d_x, xi.d_y, xj.d_x, xj.d_y, notch_half, y_tip,
                                 y_top + 0.01 * H);
      if (!on_notch)
        ++n_extra;
    }
  }
  if (n_extra < 20) {
    std::cerr << "notched_impact: expected new broken bonds below notches, got " << n_extra
              << " (prenotch=" << n_pre << ")\n";
    return 1;
  }

  const double band_x = 0.40 * W;
  const double band_y = 0.55 * H;
  const double z_cut = 1.0;
  const double exclude_r = 1.5 * mesh_size;
  auto left = fitCrackAngle(dem, -notch_half, y_tip, -1.0, z_cut, band_x, band_y, exclude_r);
  auto right = fitCrackAngle(dem, notch_half, y_tip, +1.0, z_cut, band_x, band_y, exclude_r);
  if (left.n_pts < 4 || right.n_pts < 4) {
    std::cerr << "notched_impact: insufficient damage points near tips (L=" << left.n_pts
              << " R=" << right.n_pts << ")\n";
    return 1;
  }
  // Tip damage clouds sit outward and below each notch.
  if (!(left.mean_dx < 0. && left.mean_dy > 0. && right.mean_dx > 0. && right.mean_dy > 0.)) {
    std::cerr << std::format(
        "notched_impact: tip damage centroids not outward/down "
        "(L dx={:.3e} dy={:.3e} R dx={:.3e} dy={:.3e})\n",
        left.mean_dx, left.mean_dy, right.mean_dx, right.mean_dy);
    return 1;
  }

  std::cout << std::format(
      "notched_impact PASS: prenotch={} new_broken={} "
      "L(dx,dy,ang)=({:.3e},{:.3e},{:.1f}°) R=({:.3e},{:.3e},{:.1f}°) max|v|={:.3e}\n",
      n_pre, n_extra, left.mean_dx, left.mean_dy, left.angle_deg, right.mean_dx, right.mean_dy,
      right.angle_deg, vmax);

  dem.close();
  return 0;
}
