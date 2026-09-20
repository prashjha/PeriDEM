/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 *
 * Notched-plate impact driver (Silling / Trask / Bhat KW setups).
 * Checks: prenotch bonds, tip damage growth, no blow-up.
 * Paper-angle validation lives outside ctest.
 * Flags: -quick, -traskDisp, -bhatKW, -dim3, -vImpact, -Gc, -meshSize,
 * -finalTime, -outputDir, -nThreads, -selfContact, -bondBreak.
 */

#include "fracture/prenotch.h"
#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "periDEMModel.h"
#include "time_int/integrator.h"
#include "util/function.h"
#include "util/io.h"
#include "util/parallelUtil.h"
#include "util/point.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
// The pre-notch functions are in src/fracture/prenotch.h, so that this driver
// and the Python interface break the same bonds.
size_t applyNotches(PeriDEMModel &dem, double x_left, double x_right, double y_lo,
                    double y_hi) {
  return geometry::breakBondsCrossingVerticalLines(
      *dem.d_fracture_p, dem.d_xRef, dem.d_neighPd, dem.d_ptId, 0,
      {x_left, x_right}, y_lo, y_hi);
}

size_t applyNotchSlots(PeriDEMModel &dem, double notch_half, double notch_w,
                       double y_lo, double y_hi) {
  return geometry::breakBondsInSlots(*dem.d_fracture_p, dem.d_xRef,
                                     dem.d_neighPd, dem.d_ptId, 0,
                                     {-notch_half, notch_half}, notch_w, y_lo,
                                     y_hi);
}

// A V-notch is seeded on its midplane, which is the same zero-width cut.
size_t applyNotchMidplanes(PeriDEMModel &dem, double notch_half, double y_tip,
                           double y_top) {
  return geometry::breakBondsCrossingVerticalLines(
      *dem.d_fracture_p, dem.d_xRef, dem.d_neighPd, dem.d_ptId, 0,
      {-notch_half, notch_half}, y_tip, y_top);
}


std::string directoryPathWithTrailingSep(const std::filesystem::path &dir) {
  namespace fs = std::filesystem;
  fs::path n = fs::absolute(dir).lexically_normal();
  std::string s = n.string();
  if (!s.empty() && s.back() != '/' && s.back() != '\\')
    s += fs::path::preferred_separator;
  return s;
}




// Break bonds that cross the midplane of each prenotch.

double nodePhi(const PeriDEMModel &dem, size_t i) {
  const auto &nb = dem.d_neighPd[i];
  if (nb.empty())
    return 0.;
  size_t n_br = 0;
  for (size_t k = 0; k < nb.size(); ++k) {
    if (dem.d_fracture_p->getBondState(i, k))
      ++n_br;
  }
  return static_cast<double>(n_br) / static_cast<double>(nb.size());
}

double nodeDamageForFit(const PeriDEMModel &dem, size_t i, bool use_bond_count) {
  if (use_bond_count) {
    if (!dem.d_phiBond.empty() && i < dem.d_phiBond.size())
      return static_cast<double>(dem.d_phiBond[i]);
    return nodePhi(dem, i);
  }
  if (!dem.d_phi.empty() && i < dem.d_phi.size())
    return static_cast<double>(dem.d_phi[i]);
  return nodePhi(dem, i);
}

struct CrackFit {
  size_t n_pts = 0;
  double angle_to_notch_deg = 0.;
  double mean_dx = 0.;
  double mean_dy = 0.;
};

// Max-damage ridge in outward bins from the tip; PCA for a rough path angle.
CrackFit fitCrackFromPhi(const PeriDEMModel &dem, double tip_x, double tip_y,
                         double outward_sign, double phi_cut, double band_x, double band_y,
                         double exclude_r, double bin_h, bool use_bond_count = false) {
  CrackFit fit;
  std::vector<double> xs, ys;

  const int nbin = std::max(4, static_cast<int>(std::ceil((band_x - exclude_r) / bin_h)));
  for (int b = 0; b < nbin; ++b) {
    const double r0 = exclude_r + b * bin_h;
    const double r1 = std::min(band_x, r0 + bin_h);
    double best_phi = phi_cut;
    double best_dx = 0., best_dy = 0.;
    bool found = false;
    for (size_t i = 0; i < dem.d_xRef.size(); ++i) {
      if (dem.d_ptId[i] != 0)
        continue;
      const double phi = nodeDamageForFit(dem, i, use_bond_count);
      if (phi < best_phi)
        continue;
      const double dx_ref = outward_sign * (dem.d_xRef[i].d_x - tip_x);
      const double dy_ref = tip_y - dem.d_xRef[i].d_y;
      if (dx_ref < r0 || dx_ref >= r1)
        continue;
      if (dy_ref < -2.0 * bin_h || dy_ref > band_y)
        continue;
      const double dx = dem.d_xRef[i].d_x - tip_x;
      const double dy = tip_y - dem.d_xRef[i].d_y;
      best_phi = phi;
      best_dx = dx;
      best_dy = dy;
      found = true;
    }
    if (found) {
      xs.push_back(best_dx);
      ys.push_back(best_dy);
    }
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
  const double angle_from_horiz =
      std::atan2(std::abs(vy), std::abs(vx)) * 180. / M_PI;
  fit.angle_to_notch_deg = 90. - angle_from_horiz;
  return fit;
}

json meshSetJson(const std::filesystem::path &f, double mesh_size) {
  return json{{"File", f.string()},
              {"CreateMesh",
               {{"Flag", true},
                {"Info", "gmsh_builtin_mesh"},
                {"Mesh_Size", mesh_size},
                {"Write_Mesh_File", true}}}};
}

// Silling 2003: "a rectangular, equally spaced structured grid with dimensions
// 200 x 100 x 9". `voids` carves the two notch slots out of that grid so they
// are real gaps in the material, matching Fig. 2.
json uniformMeshSetJson(const std::filesystem::path &f, double mesh_size,
                        const std::vector<std::vector<double>> &voids = {}) {
  json cm = {{"Flag", true},
             {"Info", "uniform"},
             {"Mesh_Size", mesh_size},
             {"Write_Mesh_File", true}};
  if (!voids.empty())
    cm["Void_Regions"] = voids;
  return json{{"File", f.string()}, {"CreateMesh", cm}};
}

// The two 1.5 mm notch slots as boxes, open at the top edge.
std::vector<std::vector<double>> notchVoidBoxes(double H, double notch_half,
                                                double notch_w, double notch_depth,
                                                double z_lo, double z_hi) {
  const double hw = 0.5 * notch_w;
  const double y_tip = 0.5 * H - notch_depth;
  const double y_hi = 0.5 * H + 1.0e-9;
  return {{-notch_half - hw, y_tip, z_lo, -notch_half + hw, y_hi, z_hi},
          {notch_half - hw, y_tip, z_lo, notch_half + hw, y_hi, z_hi}};
}

// Silling Fig. 2: plate W×H with two open top notches (width notch_w, depth notch_depth),
// centers at x=±notch_half (tip-to-tip = 2*notch_half).
geom::GeomData sillingNotchedPlateGeom(double W, double H, double notch_half,
                                       double notch_w, double notch_depth) {
  const double x0 = -0.5 * W, y0 = -0.5 * H, x1 = 0.5 * W, y1 = 0.5 * H;
  const double hw = 0.5 * notch_w;
  const double y_tip = y1 - notch_depth;
  geom::GeomData plate;
  plate.d_geomName = "complex";
  plate.d_geomComplexInfo = {
      std::vector<std::string>{"rectangle", "rectangle", "rectangle"},
      std::vector<std::string>{"plus", "minus", "minus"}};
  plate.d_geomParams = {
      // outer plate
      x0, y0, 0., x1, y1, 0.,
      // left notch cutout (opens at top)
      -notch_half - hw, y_tip, 0., -notch_half + hw, y1 + 1.0e-6, 0.,
      // right notch cutout
      notch_half - hw, y_tip, 0., notch_half + hw, y1 + 1.0e-6, 0.};
  return plate;
}

// Bhat Fig. 4(a): V-notches — 1.5 mm opening at the top edge, tapering to a
// sharp tip at 50 mm depth. Triangle cutouts (9 params = three vertices each).
geom::GeomData bhatVNotchedPlateGeom(double W, double H, double notch_half,
                                     double notch_w, double notch_depth) {
  const double x0 = -0.5 * W, y0 = -0.5 * H, x1 = 0.5 * W, y1 = 0.5 * H;
  const double hw = 0.5 * notch_w;
  const double y_tip = y1 - notch_depth;
  const double y_top = y1 + 1.0e-6;
  geom::GeomData plate;
  plate.d_geomName = "complex";
  plate.d_geomComplexInfo = {
      std::vector<std::string>{"rectangle", "triangle", "triangle"},
      std::vector<std::string>{"plus", "minus", "minus"}};
  plate.d_geomParams = {
      // outer plate
      x0, y0, 0., x1, y1, 0.,
      // left V: top-left, top-right, tip
      -notch_half - hw, y_top, 0., -notch_half + hw, y_top, 0., -notch_half, y_tip, 0.,
      // right V
      notch_half - hw, y_top, 0., notch_half + hw, y_top, 0., notch_half, y_tip, 0.};
  return plate;
}

json fixedTopOuterBC(double W, double H, double notch_half) {
  const double fix_h = 0.08 * H;
  return json{
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
}

// Trask: Single_Particle plate; between notches u=<0,-v t> (two BC sets — one scalar du each).
json buildTraskInputJson(const std::string &output_path,
                         const std::filesystem::path &mesh_plate, double W, double H,
                         double notch_half, double notch_w, double notch_depth, double mesh_size,
                         double horizon, double rho, double E, double K, double G, double Gc,
                         double v_impact, double final_time, size_t num_steps) {
  auto model = inp::ModelDeck::getExampleJson(
      json{{"Dimension", 2},
           {"Final_Time", final_time},
           {"Time_Steps", num_steps},
           {"Discretization_Type", json{{"Spatial", "finite_difference"}, {"Time", "central_difference"}}},
           {"Populate_ElementNodeConnectivity", true},
           {"Quad_Approximation_Order", 2},
           {"Particle_Sim_Type", "Single_Particle"},
           {"Seed", 0}});
  // Trask §5: broken bond → weight 0 (no force). No self-contact.
  model["Self_Contact"] = "none";
  model["Bond_Break"] = "tension"; // literature PMB: break in tension only

  auto output = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", output_path},
           {"Tags", std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage", "Damage_Z",
                                "Particle_ID", "Fixity"})},
           {"Output_Interval", std::max<size_t>(1, num_steps / 10)},
           {"Debug", 1},
           {"Perform_FE_Out", false},
           {"Compress_Type", "zlib"},
           {"Perform_Out", true},
           {"Test_Output_Interval", num_steps},
           {"Tag_PP", ""},
           {"PVD_Collection", false}});

  // Trask §6.2: top left/right of notches u=<0,0>; drive between notches
  // u=<0,-v t>; sides and bottom free (collar bond-break is not implemented).
  const double drive_h = std::max(2.0 * mesh_size, 0.002);
  const std::vector<double> drive_strip{-notch_half, 0.5 * H - drive_h, 0., notch_half,
                                        0.55 * H, 0.};
  json outer = fixedTopOuterBC(W, H, notch_half);
  json bc_disp = {
      {"Sets", 4},
      {"Set_1", outer["Set_1"]},
      {"Set_2", outer["Set_2"]},
      {"Set_3",
       {{"Particle_List", std::vector<size_t>{0}},
        {"Region", {{"Geometry", {{"Type", "rectangle"}, {"Parameters", drive_strip}}}}},
        {"Direction", std::vector<size_t>{1}},
        {"Time_Function", {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
        {"Spatial_Function", {{"Type", "constant"}}},
        {"Zero_Displacement", true}}},
      {"Set_4",
       {{"Particle_List", std::vector<size_t>{0}},
        {"Region", {{"Geometry", {{"Type", "rectangle"}, {"Parameters", drive_strip}}}}},
        {"Direction", std::vector<size_t>{2}},
        {"Time_Function",
         {{"Type", "linear"}, {"Parameters", std::vector<double>{-v_impact}}}},
        {"Spatial_Function", {{"Type", "constant"}}}}}};

  geom::GeomData plate =
      sillingNotchedPlateGeom(W, H, notch_half, notch_w, notch_depth);
  auto particle = inp::ParticleDeck::getParticleGeomExampleJson({plate});
  json mesh = {{"Sets", 1}, {"Set_1", meshSetJson(mesh_plate, mesh_size)}};
  auto material = inp::ParticleDeck::getParticleMaterialExampleJson(1);
  material["Set_1"] = inp::MaterialDeck::getExampleJson(
      json{{"Type", "PMBBond"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", Gc},
           {"E", E},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 0}}}});
  // Trask/Silling ω≡1 (ConstInfluence default a0=dim+1 would mis-scale c)
  material["Set_1"]["Influence_Function"] = {
      {"Type", 0}, {"Parameters", std::vector<double>{1.0}}};

  return json{{"Model", model},
              {"Output", output},
              {"Displacement_BC", bc_disp},
              {"Particle", particle},
              {"Mesh", mesh},
              {"Material", material}};
}

json buildImpactInputJson(const std::string &output_path,
                          const std::filesystem::path &mesh_plate,
                          const std::filesystem::path &mesh_impactor, double W, double H,
                          double notch_half, double notch_w, double notch_depth, double Iw,
                          double Ih, double gap, double mesh_size, double horizon,
                          double Rc_factor, double Kn, double rho, double E, double K, double G,
                          double Gc, double v_impact, double final_time, size_t num_steps,
                          double thickness = 0.) {
  // Silling 2003 EMU KW: explicit central difference; brittle-microelastic PMB;
  // broken bond force = 0; plate boundaries load-free (no Displacement_BC);
  // rigid impactor of finite mass.
  // thickness > 0 runs his actual 3D case (200 x 100 x 9 grid, 9 mm plate).
  const bool dim3 = thickness > 0.;
  const size_t dim = dim3 ? 3 : 2;
  // Element-node connectivity is only used for strain/stress output and does not
  // support hexahedra, which is what the 3D structured grid produces.
  auto model = inp::ModelDeck::getExampleJson(
      json{{"Dimension", dim},
           {"Final_Time", final_time},
           {"Time_Steps", num_steps},
           {"Discretization_Type", json{{"Spatial", "finite_difference"}, {"Time", "central_difference"}}},
           {"Populate_ElementNodeConnectivity", !dim3},
           {"Quad_Approximation_Order", 2},
           {"Particle_Sim_Type", "Multi_Particle"},
           {"Seed", 0}});
  model["Self_Contact"] = "none";
  model["Bond_Break"] = "tension";
  model["Wall_Contact"] = "meshed";
  // Silling Fig. 2: rigid cylinder of 1.57 kg. In 2D the analogue is the mass
  // per unit thickness of his 9 mm plate, M/t = 1.57/0.009 kg/m.
  model["Rigid_Particles"] = json::array(
      {json{{"Id", 1}, {"Mass", dim3 ? 1.57 : 1.57 / 0.009}}});

  auto output = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", output_path},
           {"Tags", std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage", "Damage_Bond",
                                "Damage_Z", "Particle_ID"})},
           {"Output_Interval", std::max<size_t>(1, num_steps / 10)},
           {"Debug", 1},
           {"Perform_FE_Out", false},
           {"Compress_Type", "zlib"},
           {"Perform_Out", true},
           {"Test_Output_Interval", num_steps},
           {"Tag_PP", ""},
           {"PVD_Collection", false}});

  json ic = {
      {"Sets", 1},
      {"Set_1",
       {{"Particle_List", std::vector<size_t>{1}},
        {"Constant_Velocity",
         {{"Velocity_Vector", std::vector<double>{0., -v_impact, 0.}}}}}}};

  // The impactor is rigid but of finite mass (Model.Rigid_Particles above), so
  // it decelerates on contact instead of being driven through the plate.
  json bc_disp = {{"Sets", 1},
                  {"Set_1",
                   {{"Particle_List", std::vector<size_t>{1}},
                    {"Direction", std::vector<size_t>{1}},
                    {"Time_Function",
                     {{"Type", "constant"}, {"Parameters", std::vector<double>{0.}}}},
                    {"Spatial_Function", {{"Type", "constant"}}},
                    {"Zero_Displacement", true}}}};

  // Plate is the full box on a structured grid with the two 1.5 mm notch slots
  // removed from the mesh, so they are real gaps (Fig. 2) rather than material
  // whose bonds have been cut.
  geom::GeomData plate;
  geom::GeomData impactor;
  if (dim3) {
    // Keep both bodies centred on z = 0 so particle generation places them
    // consistently (it positions objects by their centre).
    plate.d_geomName = "cuboid";
    plate.d_geomParams = {-0.5 * W, -0.5 * H, -0.5 * thickness,
                          0.5 * W,  0.5 * H,  0.5 * thickness};
    // Silling Fig. 4: the cylinder axis is along the impact direction and its
    // flat end face strikes the plate edge. Params: radius, centre of the
    // beginning cross-section, and the vector to the end cross-section.
    impactor.d_geomName = "cylinder";
    impactor.d_geomParams = {0.5 * Iw, 0., -0.5 * Ih, 0., 0., Ih, 0.};
  } else {
    plate.d_geomName = "rectangle";
    plate.d_geomParams = {-0.5 * W, -0.5 * H, 0., 0.5 * W, 0.5 * H, 0.};
    // In-plane section of that same cylinder: Iw wide, Ih long, flat face down.
    impactor.d_geomName = "rectangle";
    impactor.d_geomParams = {-0.5 * Iw, -0.5 * Ih, 0., 0.5 * Iw, 0.5 * Ih, 0.};
  }
  auto particle = inp::ParticleDeck::getParticleGeomExampleJson({plate, impactor});

  const auto voids =
      notchVoidBoxes(H, notch_half, notch_w, notch_depth,
                     dim3 ? -0.5 * thickness - 1.0e-9 : -1.0e-9,
                     dim3 ? 0.5 * thickness + 1.0e-9 : 1.0e-9);
  // Plate on Silling's equally spaced structured grid. In 2D the impactor is a
  // rectangle so it goes on the same grid; in 3D it is a cylinder, which the
  // uniform grid cannot represent, so it is meshed by Gmsh at the same spacing.
  json mesh = {{"Sets", 2},
               {"Set_1", uniformMeshSetJson(mesh_plate, mesh_size, voids)},
               {"Set_2", dim3 ? meshSetJson(mesh_impactor, mesh_size)
                              : uniformMeshSetJson(mesh_impactor, mesh_size)}};

  // Impactor is steel like the plate but unbreakable, and its nodal forces are
  // replaced by the rigid-body acceleration each step, so its bond stiffness
  // never enters the solution.
  auto material = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  material["Set_1"] = inp::MaterialDeck::getExampleJson(
      json{{"Type", "PMBBond"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", Gc},
           {"E", E},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 0}}}});
  material["Set_2"] = inp::MaterialDeck::getExampleJson(
      json{{"Type", "PDElasticBond"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", 0.},
           {"E", E},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 0}}}});
  material["Set_1"]["Influence_Function"] = {
      {"Type", 0}, {"Parameters", std::vector<double>{1.0}}};
  material["Set_2"]["Influence_Function"] = {
      {"Type", 0}, {"Parameters", std::vector<double>{1.0}}};

  // Absolute Contact_Radius from the requested mesh size (not Factor×hMin).
  const double Rc_abs = Rc_factor * mesh_size;
  json contact_base = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius", Rc_abs},
               {"Kn", Kn},
               {"Damping_On", false},
               {"Epsilon", 1.0},
               {"Friction_On", false},
               {"Friction_Coeff", 0.0},
               {"Kn_Factor", 1.0},
               {"Beta_n_Factor", 0.0},
               {"K", K}});
  contact_base["Kn"] = Kn;
  json contact = inp::ParticleDeck::getParticleContactExampleJson(2);
  contact["Set_1_1"] = contact_base;
  contact["Set_1_2"] = contact_base;
  contact["Set_2_2"] = contact_base;
  contact["Damping_Law"] = "off";
  contact["Friction_Law"] = "coulomb_simple";

  const double cy_imp = 0.5 * H + 0.5 * Ih + gap; // flat face `gap` above the edge
  auto pgen = inp::PGenDeck::getExampleJson(json{{"Method", "From_File"}});
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

  // Plate has no Displacement_BC (Silling: load-free all around); the only
  // Displacement_BC drives the rigid impactor.
  return json{{"Model", model},
              {"Output", output},
              {"Displacement_BC", bc_disp},
              {"IC", ic},
              {"Particle", particle},
              {"Mesh", mesh},
              {"Material", material},
              {"Contact", contact},
              {"Neighbor", inp::PNeighborDeck::getExampleJson(
          json{{"Update_Criteria", "simple_all"},
               {"Search_Factor", 5.0},
               {"Search_Interval", 1},
               {"Near_Bd_Nodes_Tol", 0.5}})},
              {"Particle_Generation", pgen}};
}

// Bhat 2023 §6.2: V-notched plate, free impactor, reference_gap self-contact.
json buildBhatInputJson(const std::string &output_path,
                        const std::filesystem::path &mesh_plate,
                        const std::filesystem::path &mesh_impactor, double W, double H,
                        double notch_half, double notch_w, double notch_depth, double Iw,
                        double Ih, double gap, double mesh_size, double horizon,
                        double Rc_factor, double rho, double E, double K, double G, double Gc,
                        double v_impact, double final_time, size_t num_steps) {
  auto model = inp::ModelDeck::getExampleJson(
      json{{"Dimension", 2},
           {"Final_Time", final_time},
           {"Time_Steps", num_steps},
           {"Discretization_Type", json{{"Spatial", "finite_difference"}, {"Time", "velocity_verlet"}}},
           {"Populate_ElementNodeConnectivity", true},
           {"Quad_Approximation_Order", 2},
           {"Particle_Sim_Type", "Multi_Particle"},
           {"Seed", 0}});
  model["Self_Contact"] = "reference_gap"; // Bhat 2023 §4.4
  model["Bond_Break"] = "absolute_stretch";  // Bhat §3.1: |s| > |s0|
  model["Wall_Contact"] = "meshed";

  auto output = inp::OutputDeck::getExampleJson(
      json{{"File_Format", "vtu"},
           {"Path", output_path},
           {"Tags", std::vector<std::string>({"Displacement", "Velocity", "Force", "Damage", "Damage_Bond",
                                "Damage_Z", "Particle_ID"})},
           {"Output_Interval", std::max<size_t>(1, num_steps / 10)},
           {"Debug", 1},
           {"Perform_FE_Out", false},
           {"Compress_Type", "zlib"},
           {"Perform_Out", true},
           {"Test_Output_Interval", num_steps},
           {"Tag_PP", ""},
           {"PVD_Collection", false}});

  json ic = {
      {"Sets", 1},
      {"Set_1",
       {{"Particle_List", std::vector<size_t>{1}},
        {"Constant_Velocity",
         {{"Velocity_Vector", std::vector<double>{0., -v_impact, 0.}}}}}}};

  // Paper: only the outer top ligaments are fixed; impactor is free.
  json outer = fixedTopOuterBC(W, H, notch_half);
  json bc_disp = {{"Sets", 2}, {"Set_1", outer["Set_1"]}, {"Set_2", outer["Set_2"]}};

  // Bhat specimen: OCC/Gmsh nonconvex plate with V-notch cutouts (§5, Fig. 4).
  geom::GeomData plate =
      bhatVNotchedPlateGeom(W, H, notch_half, notch_w, notch_depth);
  geom::GeomData impactor;
  impactor.d_geomName = "rectangle";
  impactor.d_geomParams = {-0.5 * Iw, -0.5 * Ih, 0., 0.5 * Iw, 0.5 * Ih, 0.};
  auto particle = inp::ParticleDeck::getParticleGeomExampleJson({plate, impactor});

  json mesh = {{"Sets", 2},
               {"Set_1", meshSetJson(mesh_plate, mesh_size)},
               {"Set_2", meshSetJson(mesh_impactor, mesh_size)}};

  // Bhat Table 1, constant micromodulus, ν = 1/3 (bond-based, 2D).
  const double nu_bhat = 1.0 / 3.0;
  const double c_bhat =
      6.0 * E / (M_PI * std::pow(horizon, 3.0) * (1.0 - nu_bhat));
  const double s0_bhat = std::sqrt(4.0 * M_PI * Gc / (9.0 * E * horizon));

  auto material = inp::ParticleDeck::getParticleMaterialExampleJson(2);
  auto mat_bhat = inp::MaterialDeck::getExampleJson(
      json{{"Type", "PMBBond"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", Gc},
           {"E", E},
           {"Compute_From_Classical", false},
           {"Influence_Function", json{{"Type", 0}}}});
  mat_bhat["Bond_Potential_Params"] = std::vector<double>{c_bhat, s0_bhat};
  mat_bhat["Influence_Function"] = {{"Type", 0}, {"Parameters", std::vector<double>{1.0}}};
  material["Set_1"] = mat_bhat;
  // Free deformable striker (paper). Unbreakable elastic so the contact face
  // delivers bulk impulse; plate remains PMB with paper (c, s0).
  auto mat_strike = inp::MaterialDeck::getExampleJson(
      json{{"Type", "PDElasticBond"},
           {"Is_Plane_Strain", false},
           {"Horizon", horizon},
           {"Density", rho},
           {"K", K},
           {"G", G},
           {"Gc", 0.},
           {"E", E},
           {"Compute_From_Classical", true},
           {"Influence_Function", json{{"Type", 0}}}});
  mat_strike["Influence_Function"] = {{"Type", 0}, {"Parameters", std::vector<double>{1.0}}};
  material["Set_2"] = mat_strike;

  // Absolute Rc from mesh_size (not Factor×hMin).
  const double Kn_bhat = 18.0 * K / (M_PI * std::pow(horizon, 4.0));
  const double Rc_abs = Rc_factor * mesh_size;
  json contact_base = inp::ContactPairDeck::getExampleJson(
          json{{"Contact_Radius", Rc_abs},
               {"Kn", Kn_bhat},
               {"Damping_On", false},
               {"Epsilon", 1.0},
               {"Friction_On", false},
               {"Friction_Coeff", 0.0},
               {"Kn_Factor", 1.0},
               {"Beta_n_Factor", 0.0},
               {"K", K}});
  contact_base["Kn"] = Kn_bhat;
  json contact = inp::ParticleDeck::getParticleContactExampleJson(2);
  contact["Set_1_1"] = contact_base;
  contact["Set_1_2"] = contact_base;
  contact["Set_2_2"] = contact_base;
  contact["Damping_Law"] = "off";            // Bhat §6: β_d = 0
  contact["Friction_Law"] = "coulomb_simple"; // μ = 0 below
  contact["Set_1_1"]["Friction_Coefficient"] = 0.;
  contact["Set_1_2"]["Friction_Coefficient"] = 0.;
  contact["Set_2_2"]["Friction_Coefficient"] = 0.;

  const double cy_imp = 0.5 * H + 0.5 * Ih + gap;
  auto pgen = inp::PGenDeck::getExampleJson(json{{"Method", "From_File"}});
  pgen["Random_Rotation"] = false;
  pgen["Data"]["N"] = 2;
  pgen["Data"]["0"] = json{{"x", 0.},     {"y", 0.},     {"z", 0.},
                           {"theta", 0.}, {"s", 1.},     {"geom_id", 0},
                           {"mat_id", 0}, {"contact_id", 0}};
  pgen["Data"]["1"] = json{{"x", 0.},     {"y", cy_imp}, {"z", 0.},
                           {"theta", 0.}, {"s", 1.},     {"geom_id", 1},
                           {"mat_id", 1}, {"contact_id", 1}};

  return json{{"Model", model},
              {"Output", output},
              {"Displacement_BC", bc_disp},
              {"IC", ic},
              {"Particle", particle},
              {"Mesh", mesh},
              {"Material", material},
              {"Contact", contact},
              {"Neighbor", inp::PNeighborDeck::getExampleJson(
          json{{"Update_Criteria", "simple_all"},
               {"Search_Factor", 5.0},
               {"Search_Interval", 1},
               {"Near_Bd_Nodes_Tol", 0.5}})},
              {"Particle_Generation", pgen}};
}

} // namespace

int main(int argc, char *argv[]) {
  util::parallel::initMpi(argc, argv);
  util::io::InputParser input(argc, argv);

  unsigned n_threads = 1;
  if (input.cmdOptionExists("-nThreads"))
    n_threads = static_cast<unsigned>(std::stoi(input.getCmdOption("-nThreads")));
  util::parallel::initNThreads(n_threads);

  const bool quick = input.cmdOptionExists("-quick");
  const bool trask = input.cmdOptionExists("-traskDisp");
  const bool bhat = input.cmdOptionExists("-bhatKW");
  // Silling's actual case is 3D: a 9 mm plate on a 200 x 100 x 9 grid.
  const bool dim3 = input.cmdOptionExists("-dim3");

  namespace fs = std::filesystem;
  // Always write under the binary cwd (build/linux/.../notched_impact_inbuilt/) unless overridden.
  std::string run_tag =
      quick ? "quick" : (trask ? "trask" : (bhat ? "bhat" : (dim3 ? "silling3d" : "lit")));
  // New campaign outputs go under runs_new/ (do not mix with archived runs/).
  fs::path base = fs::current_path() / "runs_new" / run_tag;
  if (input.cmdOptionExists("-outputDir"))
    base = input.getCmdOption("-outputDir");
  const fs::path out_dir = base / "out";
  const fs::path inp_dir = base / "inp";
  fs::create_directories(out_dir);
  fs::create_directories(inp_dir);
  std::cout << std::format("notched_impact: outputDir={}\n", base.string());

  // Fig. 2 Silling 2003: 200×100 mm plate, two 1.5 mm notches depth 50 mm, tip-to-tip 50 mm.
  double W = 0.200;
  double H = 0.100;
  double notch_depth = 0.050;
  double notch_half = 0.025;   // notch centerline ±25 mm
  double notch_w = 0.0015;     // Silling 1.5 mm open gap
  double plate_thickness = 0.009; // Silling Fig. 2: 9 mm (only used with -dim3)
  // Impactor: Silling Fig. 2/4 give a cylinder of 1.57 kg striking edge-on, as
  // wide as the 50 mm ligament between the notches. At ρ = 8000 that mass fixes
  // its length: 1.57/(8000·π·0.025²) = 0.100 m. Bhat Fig. 4(b) draws exactly
  // this 50 × 100 mm section, giving the same width and length.
  double Iw = 0.050;
  double Ih = 0.100;
  // Silling 2003 EMU grid 200×100×9 on the mm plate → h ≈ 1 mm; Trask KW: δ = 3h.
  double mesh_size = 0.001;
  double horizon = 3.0 * mesh_size; // 3 mm
  double Rc_factor = 0.95;
  // The impactor must start outside the contact radius, which the solver sets to
  // Rc_factor·h_min: any closer and the first step applies a saturated repulsive
  // force that fires the struck nodes off at kilometres per second. Nothing acts
  // on the impactor while it closes this gap, so its only effect is to delay
  // contact by gap/v, which the crack-speed fit measures from damage arrival.
  double gap = 1.5 * Rc_factor * mesh_size;

  // Table 2 M1 ≡ Silling maraging steel (E=191 GPa, ν≈0.3).
  // Gc from KIc≈90 MPa√m: plane-stress KIc²/E = 42408 J/m² (Trask/Silling handbook).
  double rho = 8000.0;
  double E = 191.0e9;
  double Kbulk = 159.2e9;
  double nu = 0.5 * (1.0 - E / (3.0 * Kbulk));
  double G = E / (2.0 * (1.0 + nu));
  double Gc = 42408.0;
  double v_impact = 32.0;
  double dt = 2.5e-9; // Bhat §6.1
  // Silling's cracks run all the way to the free edges (75 mm) at ~900 m/s, so
  // ~85 µs of propagation on top of the free flight and the initiation delay.
  double final_time = 1.7e-4; // 170 µs
  if (input.cmdOptionExists("-Gc"))
    Gc = std::stod(input.getCmdOption("-Gc"));
  if (input.cmdOptionExists("-finalTime"))
    final_time = std::stod(input.getCmdOption("-finalTime"));
  if (input.cmdOptionExists("-vImpact"))
    v_impact = std::stod(input.getCmdOption("-vImpact"));
  if (input.cmdOptionExists("-meshSize")) {
    mesh_size = std::stod(input.getCmdOption("-meshSize"));
    horizon = 3.0 * mesh_size;
  }
  if (input.cmdOptionExists("-horizon"))
    horizon = std::stod(input.getCmdOption("-horizon"));
  if (input.cmdOptionExists("-horizonFactor"))
    horizon = std::stod(input.getCmdOption("-horizonFactor")) * mesh_size;

  if (quick) {
    W = 0.040;
    H = 0.020;
    notch_depth = 0.5 * H;
    notch_half = 0.125 * W;
    mesh_size = H / 16.0;
    notch_w = std::max(mesh_size, 0.1 * notch_half);
    Iw = 0.008;
    Ih = 0.004;
    gap = 1.5 * Rc_factor * mesh_size;
    horizon = 3.0 * mesh_size;
    rho = 1200.0;
    E = 1.23e9;
    Kbulk = 2.0e9;
    nu = 0.5 * (1.0 - E / (3.0 * Kbulk));
    G = E / (2.0 * (1.0 + nu));
    Gc = 424.0;
    dt = 1.0e-8;
    final_time = 1.0e-4;
  }

  // Contact stiffness, Bhat Eq. (4.1). The solver's contact force density is
  // Kn·V_j·overlap, so Kn carries one power of δ per spatial dimension of the
  // node weight: 18k/(πδ⁴) with 2D weights h²·1 m, 18k/(πδ⁵) with 3D weights h³.
  // Using the 3D form in 2D makes contact 1/δ ≈ 333× too stiff.
  const double Kn =
      util::normalContactStiffness(Kbulk, Kbulk, horizon, dim3 ? 5 : 4);
  const size_t num_steps = static_cast<size_t>(std::llround(final_time / dt));

  json input_json;
  if (trask) {
    input_json = buildTraskInputJson(directoryPathWithTrailingSep(out_dir),
                                     inp_dir / "mesh_plate.msh", W, H, notch_half, notch_w,
                                     notch_depth, mesh_size, horizon, rho, E, Kbulk, G, Gc,
                                     v_impact, final_time, num_steps);
  } else if (bhat) {
    input_json = buildBhatInputJson(
        directoryPathWithTrailingSep(out_dir), inp_dir / "mesh_plate.msh",
        inp_dir / "mesh_impactor.msh", W, H, notch_half, notch_w, notch_depth, Iw, Ih, gap,
        mesh_size, horizon, Rc_factor, rho, E, Kbulk, G, Gc, v_impact, final_time, num_steps);
  } else {
    input_json = buildImpactInputJson(
        directoryPathWithTrailingSep(out_dir), inp_dir / "mesh_plate.msh",
        inp_dir / "mesh_impactor.msh", W, H, notch_half, notch_w, notch_depth, Iw, Ih, gap,
        mesh_size, horizon, Rc_factor, Kn, rho, E, Kbulk, G, Gc, v_impact, final_time,
        num_steps, dim3 ? plate_thickness : 0.);
  }
  // Optional override (e.g. Bhat paper default is reference_gap; -selfContact none
  // isolates prenotch+void setups from restorative self-contact across the notch).
  if (input.cmdOptionExists("-selfContact")) {
    const auto sc = input.getCmdOption("-selfContact");
    input_json["Model"]["Self_Contact"] = sc;
    std::cout << std::format("notched_impact: Self_Contact override -> {}\n", sc);
  }
  if (input.cmdOptionExists("-bondBreak")) {
    const auto bb = input.getCmdOption("-bondBreak");
    input_json["Model"]["Bond_Break"] = bb;
    std::cout << std::format("notched_impact: Bond_Break override -> {}\n", bb);
  }
  if (input.cmdOptionExists("-knScale")) {
    const double s = std::stod(input.getCmdOption("-knScale"));
    for (auto &kv : input_json["Contact"].items()) {
      if (!kv.value().is_object() || !kv.value().contains("Kn"))
        continue;
      kv.value()["Kn"] = kv.value()["Kn"].get<double>() * s;
    }
    std::cout << std::format("notched_impact: Contact Kn scaled by {}\n", s);
  }
  {
    std::ofstream os(inp_dir / "input.json");
    os << input_json.dump(2);
  }

  // -deckOnly stops after the deck is written. The Python comparison reads
  // the deck and does not need this driver to run the simulation.
  if (input.cmdOptionExists("-deckOnly")) {
    util::io::print("deck written; -deckOnly, not running\n");
    util::parallel::finalizeMpi();
    return 0;
  }

  auto deck = std::make_shared<inp::Input>(input_json);
  PeriDEMModel dem(deck);
  dem.init();

  const double y_top = 0.5 * H;
  const double y_tip = y_top - notch_depth;
  // Prenotch bonds: Bhat §5 removes bonds whose segment leaves the nonconvex
  // V-domain (midplane cuts through each V tip); Silling/Trask: slot spans.
  const size_t n_pre =
      bhat ? applyNotchMidplanes(dem, notch_half, y_tip, y_top + 0.01 * H)
           : applyNotchSlots(dem, notch_half, notch_w, y_tip, y_top + 0.01 * H);
  if (n_pre < 10) {
    std::cerr << "notched_impact: expected prenotch bonds, got " << n_pre << "\n";
    return 1;
  }

  const char *mode_str =
      quick ? "quick"
            : (trask ? "trask_disp"
                     : (bhat ? "bhat_kw" : (dim3 ? "silling_3d" : "silling_impact")));
  std::cout << std::format(
      "notched_impact: mode={} nodes={} prenotch={} h={:.4e} ε={:.4e} notch_w={:.4e} "
      "Gc={:.0f} E={:.3e} v={:.1f} Nt={} T={:.3e}\n",
      mode_str, dem.d_x.size(), n_pre, mesh_size, horizon, notch_w, Gc, E, v_impact, num_steps,
      final_time);

  // Drive the loop ourselves (same sequence as time_int::Integrator::integrate)
  // so we can record when each node first becomes damaged. That arrival-time
  // field gives the crack speed, which Silling reports as ~900 m/s.
  std::vector<float> arrival(dem.d_x.size(), -1.f);
  const size_t sample_every = std::max<size_t>(1, num_steps / 400);
  auto sampleArrivals = [&]() {
    for (size_t i = 0; i < dem.d_xRef.size(); ++i) {
      if (arrival[i] >= 0.f || dem.d_ptId[i] != 0)
        continue;
      if (nodePhi(dem, i) >= 0.30)
        arrival[i] = static_cast<float>(dem.d_time);
    }
  };

  dem.applyInitialCondition();
  if (dem.performOutput())
    dem.output();
  dem.setCurrentDt(dem.timeStep());
  dem.applyDisplacementBC();
  dem.computeForces();
  time_int::applyRigidBodyConstraint(dem);
  while (dem.currentStep() < dem.numTimeSteps()) {
    dem.integrateStep();
    if (dem.shouldOutput())
      dem.output();
    if (dem.currentStep() % sample_every == 0)
      sampleArrivals();
    dem.checkStop();
  }
  sampleArrivals();

  double vmax = 0., phi_max = 0.;
  size_t n_phi30 = 0;
  for (size_t i = 0; i < dem.d_v.size(); ++i) {
    vmax = std::max(vmax, dem.d_v[i].length());
    if (dem.d_ptId[i] != 0)
      continue;
    const double phi = nodePhi(dem, i);
    phi_max = std::max(phi_max, phi);
    if (phi >= 0.30)
      ++n_phi30;
  }
  const float zmax =
      dem.d_Z.empty() ? 0.f : *std::max_element(dem.d_Z.begin(), dem.d_Z.end());
  std::cout << std::format(
      "notched_impact diag: max|v|={:.3e} maxφ={:.3f} maxZ={:.3f} n(φ≥0.3)={}\n", vmax,
      phi_max, zmax, n_phi30);

  if (!(vmax < 8.0e3) || !std::isfinite(vmax)) {
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
          geometry::segmentCrossesVerticalLine(xi.d_x, xi.d_y, xj.d_x, xj.d_y,
                                               -notch_half, y_tip,
                                               y_top + 0.01 * H) ||
          geometry::segmentCrossesVerticalLine(xi.d_x, xi.d_y, xj.d_x, xj.d_y,
                                               notch_half, y_tip,
                                               y_top + 0.01 * H);
      if (!on_notch)
        ++n_extra;
    }
  }
  if (n_extra < 20) {
    std::cerr << "notched_impact: expected new broken bonds, got " << n_extra
              << " (prenotch=" << n_pre << ")\n";
    return 1;
  }

  // Dump the damage field so the crack-path metric can be re-fitted offline
  // without repeating the simulation.
  {
    std::ofstream csv(base / "damage.csv");
    csv << "x_ref,y_ref,z_ref,x_cur,y_cur,z_cur,phi,phi_bond,arrival\n";
    for (size_t i = 0; i < dem.d_xRef.size(); ++i) {
      if (dem.d_ptId[i] != 0)
        continue;
      const double phi = nodeDamageForFit(dem, i, /*use_bond_count=*/false);
      const double phi_b = nodeDamageForFit(dem, i, /*use_bond_count=*/true);
      if (std::max(phi, phi_b) < 0.05)
        continue;
      csv << std::format("{:.6e},{:.6e},{:.6e},{:.6e},{:.6e},{:.6e},{:.4f},{:.4f},{:.6e}\n",
                         dem.d_xRef[i].d_x, dem.d_xRef[i].d_y, dem.d_xRef[i].d_z,
                         dem.d_x[i].d_x, dem.d_x[i].d_y, dem.d_x[i].d_z, phi, phi_b,
                         double(arrival[i]));
    }
  }

  const double band_x = 0.45 * W;
  const double band_y = 0.55 * H;
  const double phi_cut = quick ? 0.20 : 0.35;
  const double exclude_r = 2.0 * mesh_size;
  const bool use_bond_dmg = bhat;
  auto left = fitCrackFromPhi(dem, -notch_half, y_tip, -1.0, phi_cut, band_x, band_y, exclude_r,
                              mesh_size, use_bond_dmg);
  auto right = fitCrackFromPhi(dem, notch_half, y_tip, +1.0, phi_cut, band_x, band_y, exclude_r,
                               mesh_size, use_bond_dmg);
  if (left.n_pts < 4 || right.n_pts < 4) {
    std::cerr << "notched_impact: insufficient φ-ridge for path fit (L=" << left.n_pts
              << " R=" << right.n_pts << ")\n";
    return 1;
  }
  if (!(left.mean_dx < 0. && left.mean_dy > 0. && right.mean_dx > 0. && right.mean_dy > 0.)) {
    std::cerr << std::format(
        "notched_impact: tip damage not outward/down "
        "(L dx={:.3e} dy={:.3e} R dx={:.3e} dy={:.3e})\n",
        left.mean_dx, left.mean_dy, right.mean_dx, right.mean_dy);
    return 1;
  }

  const double ang = 0.5 * (left.angle_to_notch_deg + right.angle_to_notch_deg);
  std::cout << std::format(
      "notched_impact PASS: prenotch={} new_broken={} ang_to_notch={:.1f}° "
      "(L={:.1f}/{} R={:.1f}/{}) max|v|={:.3e}\n",
      n_pre, n_extra, ang, left.angle_to_notch_deg, left.n_pts, right.angle_to_notch_deg,
      right.n_pts, vmax);

  dem.close();
  return 0;
}
