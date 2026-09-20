/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#include "geom/geomObjectsUtil.h"
#include "inp/deckIncludes.h"
#include "material/materialUtil.h"
#include "fracture/fracture.h"
#include "fracture/prenotch.h"
#include "particle/baseParticle.h"
#include "periDEMModel.h"
#include "time_int/integrator.h"
#include "util/function.h"
#include "util/json.h"
#include "util/parallelUtil.h"
#include "util/point.h"

#include <PeriDEMConfig.h>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace nb = nanobind;

namespace {

// The node views below alias std::vector<util::Point> as an (n, 3) array of
// doubles, which requires util::Point to be three doubles with no padding.
static_assert(sizeof(util::Point) == 3 * sizeof(double),
              "util::Point must be 3 packed doubles for ndarray views");
static_assert(std::is_standard_layout_v<util::Point>,
              "util::Point must be standard layout for ndarray views");

class CwdGuard {
public:
  explicit CwdGuard(const std::filesystem::path &dir)
      : old_(std::filesystem::current_path()) {
    if (!dir.empty())
      std::filesystem::current_path(dir);
  }
  ~CwdGuard() {
    std::error_code ec;
    std::filesystem::current_path(old_, ec);
  }

  CwdGuard(const CwdGuard &) = delete;
  CwdGuard &operator=(const CwdGuard &) = delete;

private:
  std::filesystem::path old_;
};

using Array2D = nb::ndarray<nb::numpy, double, nb::ndim<2>>;
using Array1D = nb::ndarray<nb::numpy, double, nb::ndim<1>>;
using Array1DU8 = nb::ndarray<nb::numpy, uint8_t, nb::ndim<1>>;

/*! Views a util::Point range as an (n, 3) numpy array. The data is not
 *  copied, so writes reach the model. @p owner keeps the Python object
 *  alive for as long as the array exists. */
Array2D point_view(std::vector<util::Point> &pts, size_t begin, size_t n,
                   nb::handle owner) {
  if (begin > pts.size() || begin + n > pts.size())
    throw std::runtime_error("node range out of bounds");
  static double empty[3] = {0., 0., 0.};
  auto *data = n == 0 ? empty : &pts[begin].d_x;
  return Array2D(data, {n, size_t(3)}, owner);
}

Array2D point_view(std::vector<util::Point> &pts, nb::handle owner) {
  return point_view(pts, 0, pts.size(), owner);
}

template <typename T>
nb::ndarray<nb::numpy, T, nb::ndim<1>>
scalar_view(std::vector<T> &v, size_t begin, size_t n, nb::handle owner) {
  if (begin > v.size() || begin + n > v.size())
    throw std::runtime_error("node range out of bounds");
  static T empty{};
  auto *data = n == 0 ? &empty : v.data() + begin;
  return nb::ndarray<nb::numpy, T, nb::ndim<1>>(data, {n}, owner);
}

/*! d_Z is std::vector<float>. This returns a double copy, so that every field
 *  the interface exposes has the same type. */
Array1D copy_damage(const std::vector<float> &v, size_t begin, size_t n) {
  if (begin > v.size() || begin + n > v.size())
    throw std::runtime_error("node range out of bounds");
  auto *data = new double[std::max<size_t>(n, 1)];
  for (size_t i = 0; i < n; ++i)
    data[i] = static_cast<double>(v[begin + i]);
  nb::capsule owner(data,
                    [](void *p) noexcept { delete[] static_cast<double *>(p); });
  return Array1D(data, {n}, owner);
}

void ensure_runtime(unsigned n_threads) {
  int argc = 0;
  char **argv = nullptr;
  util::parallel::initMpi(argc, argv);
  if (n_threads == 0)
    n_threads = std::max(1u, std::thread::hardware_concurrency());
  util::parallel::initNThreads(n_threads);
}

std::vector<double> to_vec(const util::Point &p) {
  return {p.d_x, p.d_y, p.d_z};
}

util::Point to_point(const std::vector<double> &v) {
  if (v.empty())
    return {};
  if (v.size() > 3)
    throw std::runtime_error("expected at most 3 coordinates");
  return util::Point(v);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

/*! Holds a geom::GeomData and calls the same geom::createGeomObject and
 *  geom::writeGeometry as the C++ drivers, so that a deck written in Python
 *  and one written in C++ are produced by the same code. */
class Geometry {
public:
  Geometry(const std::string &name, const std::vector<double> &params,
           const std::vector<std::string> &vec_type,
           const std::vector<std::string> &vec_flag, bool check) {
    // Checked here so that the message can list the accepted names.
    const auto &known = geom::getAcceptableGeometries();
    if (std::find(known.begin(), known.end(), name) == known.end()) {
      std::string msg = "unknown geometry '" + name + "'; expected one of:";
      for (const auto &k : known)
        msg += " " + k;
      throw std::invalid_argument(msg);
    }
    for (const auto &sub : vec_type) {
      if (std::find(known.begin(), known.end(), sub) == known.end())
        throw std::invalid_argument("unknown sub-geometry '" + sub + "'");
    }
    for (const auto &f : vec_flag) {
      if (f != "plus" && f != "minus")
        throw std::invalid_argument(
            "geometry flag must be 'plus' or 'minus', got '" + f + "'");
    }
    if (vec_type.size() != vec_flag.size())
      throw std::invalid_argument(
          "vec_type and vec_flag must have the same length");
    if ((name == "complex") != !vec_type.empty())
      throw std::invalid_argument(
          "vec_type/vec_flag go with the 'complex' geometry and only with it");
    data_.d_geomName = name;
    data_.d_geomParams = params;
    data_.d_geomComplexInfo = {vec_type, vec_flag};
    geom::createGeomObject(data_, check);
    if (data_.d_geom_p == nullptr)
      throw std::runtime_error("could not create geometry '" + name + "'");
  }

  static Geometry example(const std::string &name,
                          const std::vector<double> &center, double s) {
    auto params = geom::exampleGeomParams(name, to_point(center), s);
    return Geometry(name, params, {}, {}, true);
  }

  const std::string &name() const { return data_.d_geomName; }
  const std::vector<double> &params() const { return data_.d_geomParams; }
  const std::vector<std::string> &vec_type() const {
    return data_.d_geomComplexInfo.first;
  }
  const std::vector<std::string> &vec_flag() const {
    return data_.d_geomComplexInfo.second;
  }

  std::vector<double> center() const { return to_vec(data_.d_geom_p->center()); }
  double volume() const { return data_.d_geom_p->volume(); }
  double inscribed_radius() const { return data_.d_geom_p->inscribedRadius(); }
  double bounding_radius() const { return data_.d_geom_p->boundingRadius(); }

  std::pair<std::vector<double>, std::vector<double>> box() const {
    auto b = data_.d_geom_p->box();
    return {to_vec(b.first), to_vec(b.second)};
  }

  bool is_inside(const std::vector<double> &x) const {
    return data_.d_geom_p->isInside(to_point(x));
  }

  bool is_near(const std::vector<double> &x, double tol) const {
    return data_.d_geom_p->isNear(to_point(x), tol);
  }

  /*! The geometry block for one particle set, as geom::writeGeometry emits it. */
  std::string to_json() const {
    json j = json::object();
    geom::writeGeometry(j, data_);
    return j.dump();
  }

  const geom::GeomData &data() const { return data_; }

  std::string repr() const {
    auto c = data_.d_geom_p->center();
    return "<peridem.Geometry '" + data_.d_geomName + "' nparams=" +
           std::to_string(data_.d_geomParams.size()) + " center=(" +
           std::to_string(c.d_x) + ", " + std::to_string(c.d_y) + ", " +
           std::to_string(c.d_z) + ")>";
  }

private:
  geom::GeomData data_;
};

// ---------------------------------------------------------------------------
// Deck factories
//
// Each function calls the inp::*Deck::getExampleJson that the C++ drivers call
// and returns the JSON text, which Python parses into a dict. The default
// values are declared only in those C++ factories.
// ---------------------------------------------------------------------------

std::string model_json(size_t dim, double t_final, size_t n_steps,
                       const std::string &spatial, const std::string &time,
                       bool populate_enc, size_t quad_order,
                       const std::string &particle_sim_type, int seed) {
  return inp::ModelDeck::getExampleJson(
      json{{"Dimension", dim},
           {"Final_Time", t_final},
           {"Time_Steps", n_steps},
           {"Discretization_Type", json{{"Spatial", spatial}, {"Time", time}}},
           {"Populate_ElementNodeConnectivity", populate_enc},
           {"Quad_Approximation_Order", quad_order},
           {"Particle_Sim_Type", particle_sim_type},
           {"Seed", seed}})
      .dump();
}

std::string output_json(const std::string &out_format, const std::string &path,
                        const std::vector<std::string> &tags,
                        size_t output_interval, size_t debug,
                        bool perform_fe_out, const std::string &compress_type,
                        bool perform_out, size_t dt_test_out,
                        const std::string &tag_pp, bool pvd_collection) {
  return inp::OutputDeck::getExampleJson(
      json{{"File_Format", out_format},
           {"Path", path},
           {"Tags", tags},
           {"Output_Interval", output_interval},
           {"Debug", debug},
           {"Perform_FE_Out", perform_fe_out},
           {"Compress_Type", compress_type},
           {"Perform_Out", perform_out},
           {"Test_Output_Interval", dt_test_out},
           {"Tag_PP", tag_pp},
           {"PVD_Collection", pvd_collection}})
      .dump();
}

std::string material_json(const std::string &material_type, bool is_plane_strain,
                          double horizon, double horizon_mesh_ratio,
                          double density, double K, double G, double Gc,
                          bool compute_from_classical, size_t influence_fn_type,
                          double E) {
  json given = json{{"Type", material_type},
                    {"Is_Plane_Strain", is_plane_strain},
                    {"Density", density},
                    {"K", K},
                    {"G", G},
                    {"Gc", Gc},
                    {"Compute_From_Classical", compute_from_classical},
                    {"Influence_Function", json{{"Type", influence_fn_type}}}};

  // One of the two says how far a bond reaches.
  if (horizon_mesh_ratio > 0.)
    given["Horizon_Mesh_Ratio"] = horizon_mesh_ratio;
  else
    given["Horizon"] = horizon;

  // Otherwise it is derived from K.
  if (E > 0.)
    given["E"] = E;

  return inp::MaterialDeck::getExampleJson(given).dump();
}

std::string contact_json(size_t n_sets) {
  return inp::ContactDeck::getExampleJson(json{{"Sets", n_sets}}).dump();
}

std::string contact_pair_json(double contact_r, bool compute_contact_r,
                              bool damping_on, bool friction_on, double Kn,
                              double eps, double mu, double Kn_factor,
                              double beta_n_factor, double delta_max,
                              double v_max, double K) {
  json given = json{{"Damping_On", damping_on},
                    {"Epsilon", eps},
                    {"Friction_On", friction_on},
                    {"Friction_Coeff", mu},
                    {"Kn_Factor", Kn_factor},
                    {"Beta_n_Factor", beta_n_factor},
                    {"K", K}};

  // A factor is applied to the mesh size, an absolute radius is not.
  given[compute_contact_r ? "Contact_Radius_Factor" : "Contact_Radius"] =
      contact_r;

  // A stiffness is used as given; otherwise it is derived from the speed.
  if (Kn > 1.E-10) {
    given["Kn"] = Kn;
  } else {
    given["V_Max"] = v_max;
    given["Delta_Max"] = delta_max;
  }

  return inp::ContactPairDeck::getExampleJson(given).dump();
}

std::string bc_json(size_t n_force_sets, size_t n_disp_sets, size_t n_ic_sets,
                    bool gravity_active, const std::vector<double> &gravity) {
  json given = json{{"Force_BC_Sets", n_force_sets},
                    {"Displacement_BC_Sets", n_disp_sets},
                    {"IC_Sets", n_ic_sets}};
  if (gravity_active)
    given["Gravity"] = to_point(gravity).toVec();
  return inp::BCDeck::getExampleJson(given).dump();
}

std::string bc_set_json(const std::string &type, const Geometry *region,
                        const std::vector<size_t> &p_list,
                        const std::vector<size_t> &p_exclude_list,
                        const std::string &time_fn_type,
                        const std::vector<double> &time_fn_params,
                        const std::string &spatial_fn_type,
                        const std::vector<double> &spatial_fn_params,
                        const std::vector<size_t> &direction,
                        bool zero_displacement, const std::string &ic_type,
                        const std::vector<double> &ic_vec) {
  json given = json{{"Type", type},
                    {"Particle_List", p_list},
                    {"Particle_Exclude_List", p_exclude_list},
                    {"Direction", direction},
                    {"Zero_Displacement", zero_displacement},
                    {"IC_Type", ic_type},
                    {"IC_Vector", ic_vec}};
  if (!time_fn_type.empty())
    given["Time_Function"] =
        json{{"Type", time_fn_type}, {"Parameters", time_fn_params}};
  if (!spatial_fn_type.empty())
    given["Spatial_Function"] =
        json{{"Type", spatial_fn_type}, {"Parameters", spatial_fn_params}};

  const geom::GeomData region_data =
      region != nullptr ? region->data() : geom::GeomData();
  return inp::BCBaseDeck::getExampleJson(
             given, region != nullptr ? &region_data : nullptr)
      .dump();
}

std::string test_json(const std::string &test_name,
                      size_t particle_id_compressive_test,
                      size_t particle_force_direction_compressive_test) {
  json given = json{{"Test_Name", test_name}};
  if (inp::TestDeck::isCompressive(test_name))
    given["Compressive_Test"] =
        json{{"Wall_Id", particle_id_compressive_test},
             {"Wall_Force_Direction", particle_force_direction_compressive_test}};
  return inp::TestDeck::getExampleJson(given).dump();
}

std::string mesh_json(const std::string &filename, double h, bool create_mesh,
                      const std::string &create_mesh_info, bool write_mesh_file,
                      const std::vector<std::vector<double>> &void_regions) {
  return inp::MeshDeck::getExampleJson(
             json{{"File", filename},
                  {"Mesh_Size", h},
                  {"Create_Mesh", create_mesh},
                  {"Info", create_mesh_info},
                  {"Write_Mesh_File", write_mesh_file},
                  {"Void_Regions", void_regions}})
      .dump();
}

std::string neighbor_json(const std::string &update_criteria, double s_factor,
                          size_t update_interval, double near_bd_nodes_tol) {
  return inp::PNeighborDeck::getExampleJson(
          json{{"Update_Criteria", update_criteria},
               {"Search_Factor", s_factor},
               {"Search_Interval", update_interval},
               {"Near_Bd_Nodes_Tol", near_bd_nodes_tol}})
      .dump();
}

std::string particle_gen_json(const std::string &method) {
  return inp::PGenDeck::getExampleJson(json{{"Method", method}}).dump();
}

std::string particle_geom_json(const std::vector<Geometry *> &geoms) {
  std::vector<geom::GeomData> v;
  v.reserve(geoms.size());
  for (auto *g : geoms) {
    if (g == nullptr)
      throw std::runtime_error("null geometry in particle geometry list");
    v.push_back(g->data());
  }
  return inp::ParticleDeck::getParticleGeomExampleJson(v).dump();
}

std::string particle_material_json(size_t n_sets) {
  return inp::ParticleDeck::getParticleMaterialExampleJson(n_sets).dump();
}

std::string particle_mesh_json(const std::vector<std::string> &files,
                               const std::vector<double> &sizes) {
  return inp::ParticleDeck::getParticleMeshExampleJson(files, sizes).dump();
}

// ---------------------------------------------------------------------------
// Simulation and particle views
// ---------------------------------------------------------------------------

class Simulation {
public:
  Simulation(std::shared_ptr<inp::Input> deck, std::filesystem::path workdir,
             std::string deck_text)
      : deck_(std::move(deck)), workdir_(std::move(workdir)),
        deck_text_(std::move(deck_text)) {
    CwdGuard g(workdir_);
    model_ = std::make_unique<PeriDEMModel>(deck_);
  }

  static Simulation from_json(const std::string &text,
                              const std::string &workdir) {
    auto deck = std::make_shared<inp::Input>(json::parse(text));
    if (!deck->isPeriDEM())
      throw std::runtime_error("deck is not a PeriDEM model");
    std::filesystem::path dir;
    if (!workdir.empty())
      dir = std::filesystem::absolute(workdir);
    return Simulation(deck, dir, text);
  }

  static Simulation from_file(const std::string &path) {
    namespace fs = std::filesystem;
    const fs::path p = fs::absolute(path);
    if (!fs::exists(p))
      throw std::runtime_error("input file does not exist: " + path);
    std::ifstream f(p);
    auto j = json::parse(f);
    auto deck = std::make_shared<inp::Input>(j);
    if (!deck->isPeriDEM())
      throw std::runtime_error("deck is not a PeriDEM model");
    return Simulation(deck, p.parent_path(), j.dump());
  }

  void setup() {
    CwdGuard g(workdir_);
    model_->init();
  }

  void run() {
    CwdGuard g(workdir_);
    model_->run(deck_);
  }

  void step() {
    CwdGuard g(workdir_);
    model_->integrateStep();
  }

  /*! Runs the time loop without calling init(). Use this after setup() when
   *  the model state has been changed, for example by a pre-notch or by a
   *  velocity field written from Python. run() calls init() and would
   *  discard those changes. */
  void integrate() {
    CwdGuard g(workdir_);
    model_->integrate();
  }

  void close() {
    CwdGuard g(workdir_);
    model_->close();
  }

  void apply_initial_condition() {
    CwdGuard g(workdir_);
    model_->applyInitialCondition();
  }

  void apply_displacement_bc() {
    CwdGuard g(workdir_);
    model_->applyDisplacementBC();
  }

  void compute_forces() {
    CwdGuard g(workdir_);
    model_->computeForces();
  }

  void apply_rigid_body_constraint() {
    time_int::applyRigidBodyConstraint(*model_);
  }

  void check_stop() {
    CwdGuard g(workdir_);
    model_->checkStop();
  }

  bool stopped() const { return model_->d_stop; }
  bool perform_output() const { return model_->performOutput(); }
  bool should_output() const { return model_->shouldOutput(); }
  size_t n_steps() const { return model_->numTimeSteps(); }
  double dt() const { return model_->timeStep(); }
  void set_current_dt(double dt) { model_->setCurrentDt(dt); }

  void write_output() {
    CwdGuard g(workdir_);
    model_->output();
  }

  size_t n_nodes() const { return model_->d_u.size(); }
  size_t step_index() const { return model_->d_n; }
  double time() const { return model_->d_time; }
  std::string workdir() const { return workdir_.string(); }

  size_t n_particles_all() const {
    return model_->d_particlesListTypeAll.size();
  }
  size_t n_particles() const {
    return model_->d_particlesListTypeParticle.size();
  }
  size_t n_walls() const { return model_->d_particlesListTypeWall.size(); }

  particle::BaseParticle *particle_all(size_t i) const {
    if (i >= model_->d_particlesListTypeAll.size())
      throw std::out_of_range("particle index out of range");
    return model_->d_particlesListTypeAll[i];
  }
  particle::BaseParticle *particle_only(size_t i) const {
    if (i >= model_->d_particlesListTypeParticle.size())
      throw std::out_of_range("particle index out of range");
    return model_->d_particlesListTypeParticle[i];
  }
  particle::BaseParticle *wall(size_t i) const {
    if (i >= model_->d_particlesListTypeWall.size())
      throw std::out_of_range("wall index out of range");
    return model_->d_particlesListTypeWall[i];
  }

  PeriDEMModel &model() { return *model_; }

  // --- peridynamic bonds ---------------------------------------------------
  // setup() builds the neighbour lists and the fracture state, so these
  // require it to have run.

  size_t n_pd_neighbors(size_t i) const {
    check_bonds(i);
    return model_->d_neighPd[i].size();
  }

  nb::ndarray<nb::numpy, size_t, nb::ndim<1>> pd_neighbors(size_t i,
                                                           nb::handle owner) {
    check_bonds(i);
    auto &v = model_->d_neighPd[i];
    return scalar_view<size_t>(v, 0, v.size(), owner);
  }

  bool bond_broken(size_t i, size_t k) const {
    check_bonds(i);
    if (k >= model_->d_neighPd[i].size())
      throw std::out_of_range("bond index out of range");
    return model_->d_fracture_p->getBondState(i, k);
  }

  void set_bond_broken(size_t i, size_t k, bool broken) {
    check_bonds(i);
    if (k >= model_->d_neighPd[i].size())
      throw std::out_of_range("bond index out of range");
    model_->d_fracture_p->setBondState(i, k, broken);
  }

  /*! Breaks the bonds of particle @p particle_id that enter notch slots of
   *  full width @p width centred on @p x_centers, between @p y_lo and @p y_hi.
   *  Calls geometry::breakBondsInSlots. Returns the number broken. */
  size_t break_bonds_in_slots(const std::vector<double> &x_centers, double width,
                              double y_lo, double y_hi, size_t particle_id) {
    check_bonds(0);
    return geometry::breakBondsInSlots(*model_->d_fracture_p, model_->d_xRef,
                                       model_->d_neighPd, model_->d_ptId,
                                       particle_id, x_centers, width, y_lo,
                                       y_hi);
  }

  /*! Breaks the bonds of particle @p particle_id that cross the vertical
   *  lines @p x_lines between @p y_lo and @p y_hi. Calls
   *  geometry::breakBondsCrossingVerticalLines. Returns the number broken. */
  size_t break_bonds_crossing_vertical(const std::vector<double> &x_lines,
                                       double y_lo, double y_hi,
                                       size_t particle_id) {
    check_bonds(0);
    return geometry::breakBondsCrossingVerticalLines(
        *model_->d_fracture_p, model_->d_xRef, model_->d_neighPd,
        model_->d_ptId, particle_id, x_lines, y_lo, y_hi);
  }

  void check_bonds(size_t i) const {
    if (model_->d_fracture_p == nullptr || model_->d_neighPd.empty())
      throw std::runtime_error(
          "peridynamic bonds are not built yet; call setup() first");
    if (i >= model_->d_neighPd.size())
      throw std::out_of_range("node index out of range");
  }

  /*! The deck text this Simulation was built from. */
  const std::string &deck_json() const { return deck_text_; }

private:
  std::shared_ptr<inp::Input> deck_;
  std::filesystem::path workdir_;
  std::string deck_text_;
  std::unique_ptr<PeriDEMModel> model_;
};

/*! Reads and writes one particle's range of the global node arrays. Holds a
 *  reference to the Simulation, which owns the data. */
class ParticleView {
public:
  ParticleView(nb::object sim, particle::BaseParticle *p)
      : sim_(std::move(sim)), p_(p) {
    if (p_ == nullptr)
      throw std::runtime_error("null particle");
  }

  size_t id() const { return p_->getId(); }
  bool is_wall() const { return p_->isWall(); }
  size_t dimension() const { return p_->getDimension(); }
  size_t n_nodes() const { return p_->getNumNodes(); }
  size_t node_start() const { return p_->d_globStart; }
  double density() const { return p_->getDensity(); }
  double horizon() const { return p_->getHorizon(); }
  double mesh_size() const { return p_->getMeshSize(); }
  double radius() const { return p_->getParticleRadius(); }

  /*! Bounding radius of the particle geometry, GeomObject::boundingRadius.
   *  The C++ contact and post-processing code measures a gap against this. */
  double bounding_radius() const {
    if (p_->d_geom_p == nullptr)
      throw std::runtime_error("particle has no geometry");
    return p_->d_geom_p->boundingRadius();
  }
  bool compute_force() const { return p_->d_computeForce; }
  void set_compute_force(bool v) { p_->d_computeForce = v; }
  bool all_dofs_constrained() const { return p_->d_allDofsConstrained; }

  size_t group_id(const std::string &key) const { return p_->getGroupId(key); }

  std::string geometry_name() const {
    return p_->d_geom_p == nullptr ? std::string("none") : p_->d_geom_p->d_name;
  }

  std::pair<std::vector<double>, std::vector<double>> box() const {
    if (p_->d_geom_p == nullptr)
      throw std::runtime_error("particle has no geometry");
    auto b = p_->d_geom_p->box();
    return {to_vec(b.first), to_vec(b.second)};
  }

  PeriDEMModel &model() const { return nb::cast<Simulation &>(sim_).model(); }

  Array2D reference() const {
    return point_view(model().d_xRef, node_start(), n_nodes(), sim_);
  }
  Array2D position() const {
    return point_view(model().d_x, node_start(), n_nodes(), sim_);
  }
  Array2D displacement() const {
    return point_view(model().d_u, node_start(), n_nodes(), sim_);
  }
  Array2D velocity() const {
    return point_view(model().d_v, node_start(), n_nodes(), sim_);
  }
  Array2D force() const {
    return point_view(model().d_f, node_start(), n_nodes(), sim_);
  }
  Array1D volume() const {
    return scalar_view<double>(model().d_vol, node_start(), n_nodes(), sim_);
  }
  Array1DU8 fixity() const {
    return scalar_view<uint8_t>(model().d_fix, node_start(), n_nodes(), sim_);
  }
  Array1D damage() const {
    return copy_damage(model().d_Z, node_start(), n_nodes());
  }

  /*! Current position of the centre node, BaseParticle::getXCenter. The C++
   *  post-processing measures particle separation from this. */
  std::vector<double> x_center() const { return to_vec(p_->getXCenter()); }

  /*! Displacement of the centre node (BaseParticle::getUCenter). */
  std::vector<double> u_center() const { return to_vec(p_->getUCenter()); }

  /*! Volume-weighted centroid of the particle nodes in the current
   *  configuration. This is an average over the body and differs from
   *  x_center, which is one node. */
  std::vector<double> center_of_mass() const {
    auto &m = model();
    const size_t b = node_start(), n = n_nodes();
    double wsum = 0.;
    util::Point c;
    for (size_t i = b; i < b + n; ++i) {
      const double w = m.d_vol[i];
      wsum += w;
      c += m.d_x[i] * w;
    }
    if (wsum <= 0.)
      return {0., 0., 0.};
    return to_vec(c * (1. / wsum));
  }

  std::string repr() const {
    return std::string("<peridem.Particle id=") + std::to_string(id()) +
           (is_wall() ? " wall" : " grain") +
           " nodes=" + std::to_string(n_nodes()) + " geom='" +
           geometry_name() + "'>";
  }

private:
  nb::object sim_;
  particle::BaseParticle *p_;
};

ParticleView make_view(nb::object sim, particle::BaseParticle *p) {
  return ParticleView(std::move(sim), p);
}

} // namespace

NB_MODULE(_core, m) {
  m.doc() = "PeriDEM Python bindings";

  m.def(
      "init", [](unsigned n_threads) { ensure_runtime(n_threads); },
      nb::arg("n_threads") = 0u,
      "Initialize MPI (if needed) and thread count. n_threads=0 uses hardware.");
  m.def("mpi_rank", []() { return util::parallel::mpiRank(); });
  m.def("mpi_size", []() { return util::parallel::mpiSize(); });
  m.def(
      "finalize", []() { util::parallel::finalizeMpi(); },
      "Call MPI_Finalize if MPI was initialized.");
  m.def("version", []() {
    return std::to_string(MAJOR_VERSION) + "." + std::to_string(MINOR_VERSION) +
           "." + std::to_string(UPDATE_VERSION);
  });

  // --- elastic-constant and contact helpers, same functions the C++ drivers use
  m.def("to_E", &material::toE, nb::arg("K"), nb::arg("nu"));
  m.def("to_K", &material::toK, nb::arg("E"), nb::arg("nu"));
  m.def("to_G_from_E", &material::toGE, nb::arg("E"), nb::arg("nu"));
  m.def("to_G_from_K", &material::toGK, nb::arg("K"), nb::arg("nu"));
  m.def("to_nu", &material::toNu, nb::arg("lambda_"), nb::arg("mu"));
  m.def("to_Gc", &material::toGc, nb::arg("KIc"), nb::arg("nu"), nb::arg("E"));
  m.def("to_KIc", &material::toKIc, nb::arg("Gc"), nb::arg("nu"), nb::arg("E"));
  m.def("harmonic_mean", &util::harmonicMean, nb::arg("a"), nb::arg("b"));
  m.def("normal_contact_stiffness", &util::normalContactStiffness,
        nb::arg("K1"), nb::arg("K2"), nb::arg("horizon"),
        nb::arg("horizon_power") = 5,
        "Kn = 18 * harmonic_mean(K1, K2) / (pi * horizon^horizon_power), "
        "util::normalContactStiffness. horizon_power is 5 for "
        "three-dimensional nodal weights and 4 for two-dimensional ones. The "
        "two-dimensional examples use 5, so the exponent is an argument.");
  m.def("acceptable_geometries",
        []() { return geom::getAcceptableGeometries(); });
  m.def(
      "num_params_required",
      [](const std::string &geom_type) {
        const auto &known = geom::getAcceptableGeometries();
        if (std::find(known.begin(), known.end(), geom_type) == known.end())
          throw std::invalid_argument("unknown geometry '" + geom_type + "'");
        return geom::getNumParamsRequired(geom_type);
      },
      nb::arg("geom_type"),
      "Accepted parameter-vector lengths for a geometry type.");

  // --- geometry
  nb::class_<Geometry>(m, "Geometry")
      .def(nb::init<const std::string &, const std::vector<double> &,
                    const std::vector<std::string> &,
                    const std::vector<std::string> &, bool>(),
           nb::arg("name"), nb::arg("params"),
           nb::arg("vec_type") = std::vector<std::string>(),
           nb::arg("vec_flag") = std::vector<std::string>(),
           nb::arg("check") = true)
      .def_static("example", &Geometry::example, nb::arg("name"),
                  nb::arg("center") = std::vector<double>{0., 0., 0.},
                  nb::arg("s") = 0.001)
      .def_prop_ro("name", &Geometry::name)
      .def_prop_ro("params", &Geometry::params)
      .def_prop_ro("vec_type", &Geometry::vec_type)
      .def_prop_ro("vec_flag", &Geometry::vec_flag)
      .def_prop_ro("center", &Geometry::center)
      .def_prop_ro("volume", &Geometry::volume)
      .def_prop_ro("inscribed_radius", &Geometry::inscribed_radius)
      .def_prop_ro("bounding_radius", &Geometry::bounding_radius)
      .def_prop_ro("box", &Geometry::box)
      .def("is_inside", &Geometry::is_inside, nb::arg("x"))
      .def("is_near", &Geometry::is_near, nb::arg("x"), nb::arg("tol"))
      .def("to_json", &Geometry::to_json)
      .def("__repr__", &Geometry::repr);

  // --- deck factories (thin forwards to inp::*Deck::getExampleJson)
  auto d = m.def_submodule("decks",
                           "inp::*Deck::getExampleJson, as JSON text");
  d.def("model", &model_json, nb::arg("dim") = 2, nb::arg("t_final") = 1.0,
        nb::arg("n_steps") = 10,
        nb::arg("spatial") = "finite_difference",
        nb::arg("time") = "central_difference",
        nb::arg("populate_element_node_connectivity") = true,
        nb::arg("quad_order") = 2,
        nb::arg("particle_sim_type") = "Multi_Particle", nb::arg("seed") = 0);
  d.def("output", &output_json, nb::arg("out_format") = "vtu",
        nb::arg("path") = "./",
        nb::arg("tags") = std::vector<std::string>{"Displacement"},
        nb::arg("output_interval") = 1, nb::arg("debug") = 2,
        nb::arg("perform_fe_out") = true, nb::arg("compress_type") = "zlib",
        nb::arg("perform_out") = true, nb::arg("dt_test_out") = 1,
        nb::arg("tag_pp") = "", nb::arg("pvd_collection") = false);
  d.def("material", &material_json, nb::arg("material_type") = "PDState",
        nb::arg("is_plane_strain") = false, nb::arg("horizon") = -1.,
        nb::arg("horizon_mesh_ratio") = -1., nb::arg("density") = 1.,
        nb::arg("K") = 0., nb::arg("G") = 0., nb::arg("Gc") = 0.,
        nb::arg("compute_from_classical") = true,
        nb::arg("influence_fn_type") = 0, nb::arg("E") = -1.);
  d.def("contact", &contact_json, nb::arg("n_sets") = 0);
  d.def("contact_pair", &contact_pair_json, nb::arg("contact_r") = 0.,
        nb::arg("compute_contact_r") = true, nb::arg("damping_on") = true,
        nb::arg("friction_on") = true, nb::arg("Kn") = 0., nb::arg("eps") = 1.,
        nb::arg("mu") = 0., nb::arg("Kn_factor") = 1.,
        nb::arg("beta_n_factor") = 1., nb::arg("delta_max") = 1.,
        nb::arg("v_max") = 0., nb::arg("K") = 0.);
  d.def("bc", &bc_json, nb::arg("n_force_sets") = 0, nb::arg("n_disp_sets") = 0,
        nb::arg("n_ic_sets") = 0, nb::arg("gravity_active") = false,
        nb::arg("gravity") = std::vector<double>{0., 0., 0.});
  d.def("bc_set", &bc_set_json, nb::arg("type") = "Force_BC",
        nb::arg("region").none() = nb::none(),
        nb::arg("particle_list") = std::vector<size_t>(),
        nb::arg("particle_exclude_list") = std::vector<size_t>(),
        nb::arg("time_fn_type") = "",
        nb::arg("time_fn_params") = std::vector<double>(),
        nb::arg("spatial_fn_type") = "",
        nb::arg("spatial_fn_params") = std::vector<double>(),
        nb::arg("direction") = std::vector<size_t>(),
        nb::arg("zero_displacement") = false, nb::arg("ic_type") = "",
        nb::arg("ic_vec") = std::vector<double>());
  d.def("mesh", &mesh_json, nb::arg("filename") = "", nb::arg("h") = -1.,
        nb::arg("create_mesh") = false,
        nb::arg("create_mesh_info") = "gmsh_builtin_mesh",
        nb::arg("write_mesh_file") = true,
        nb::arg("void_regions") = std::vector<std::vector<double>>());
  d.def("test", &test_json, nb::arg("test_name") = "",
        nb::arg("particle_id_compressive_test") = 0,
        nb::arg("particle_force_direction_compressive_test") = 0);
  d.def("neighbor", &neighbor_json, nb::arg("update_criteria") = "simple_all",
        nb::arg("s_factor") = 1., nb::arg("update_interval") = 1,
        nb::arg("near_bd_nodes_tol") = 0.5);
  d.def("particle_gen", &particle_gen_json, nb::arg("method") = "From_File");
  d.def("particle_geom", &particle_geom_json, nb::arg("geometries"));
  d.def("particle_material", &particle_material_json, nb::arg("n_sets") = 0);
  d.def("particle_mesh", &particle_mesh_json, nb::arg("files"),
        nb::arg("sizes") = std::vector<double>());

  // --- particle view
  nb::class_<ParticleView>(
      m, "Particle",
      "One particle's range of the model node arrays.\n\n"
      "The field properties are views and not copies, so writing into\n"
      "p.velocity changes the simulation. setup() rebuilds the particles and\n"
      "resizes the arrays, which invalidates any view taken before it.")
      .def_prop_ro("id", &ParticleView::id)
      .def_prop_ro("is_wall", &ParticleView::is_wall)
      .def_prop_ro("dimension", &ParticleView::dimension)
      .def_prop_ro("n_nodes", &ParticleView::n_nodes)
      .def_prop_ro("node_start", &ParticleView::node_start)
      .def_prop_ro("density", &ParticleView::density)
      .def_prop_ro("horizon", &ParticleView::horizon)
      .def_prop_ro("mesh_size", &ParticleView::mesh_size)
      .def_prop_ro("radius", &ParticleView::radius)
      .def_prop_ro("bounding_radius", &ParticleView::bounding_radius)
      .def_prop_ro("geometry_name", &ParticleView::geometry_name)
      .def_prop_ro("box", &ParticleView::box)
      .def_prop_rw("compute_force", &ParticleView::compute_force,
                   &ParticleView::set_compute_force)
      .def_prop_ro("all_dofs_constrained", &ParticleView::all_dofs_constrained)
      .def_prop_ro("x_center", &ParticleView::x_center)
      .def_prop_ro("u_center", &ParticleView::u_center)
      .def_prop_ro("center_of_mass", &ParticleView::center_of_mass)
      .def("group_id", &ParticleView::group_id, nb::arg("key"))
      .def_prop_ro("reference", &ParticleView::reference,
                   nb::rv_policy::automatic)
      .def_prop_ro("position", &ParticleView::position,
                   nb::rv_policy::automatic)
      .def_prop_ro("displacement", &ParticleView::displacement,
                   nb::rv_policy::automatic)
      .def_prop_ro("velocity", &ParticleView::velocity,
                   nb::rv_policy::automatic)
      .def_prop_ro("force", &ParticleView::force,
                   nb::rv_policy::automatic)
      .def_prop_ro("volume", &ParticleView::volume,
                   nb::rv_policy::automatic)
      .def_prop_ro("fixity", &ParticleView::fixity,
                   nb::rv_policy::automatic)
      .def_prop_ro("damage", &ParticleView::damage,
                   nb::rv_policy::automatic)
      .def("__repr__", &ParticleView::repr);

  // --- simulation
  nb::class_<Simulation>(
      m, "Simulation",
      "A PeriDEM model built from a deck.\n\n"
      "The field properties displacement, velocity and the rest are views\n"
      "into the model and not copies. Assigning into one changes the\n"
      "simulation. Call copy() to take a value that does not change.\n"
      "setup() resizes the arrays, which invalidates any view taken before\n"
      "it.")
      .def_static("from_json", &Simulation::from_json, nb::arg("text"),
                  nb::arg("workdir") = "")
      .def_static("from_file", &Simulation::from_file, nb::arg("path"))
      .def("setup", &Simulation::setup)
      .def("run", &Simulation::run)
      .def("step", &Simulation::step)
      .def("integrate", &Simulation::integrate,
           "Run the time loop from the current state (no re-init).")
      .def("close", &Simulation::close)
      .def("apply_initial_condition", &Simulation::apply_initial_condition)
      .def("apply_displacement_bc", &Simulation::apply_displacement_bc)
      .def("compute_forces", &Simulation::compute_forces)
      .def("apply_rigid_body_constraint",
           &Simulation::apply_rigid_body_constraint)
      .def("check_stop", &Simulation::check_stop)
      .def("set_current_dt", &Simulation::set_current_dt, nb::arg("dt"))
      .def_prop_ro("stopped", &Simulation::stopped)
      .def_prop_ro("perform_output", &Simulation::perform_output)
      .def_prop_ro("should_output", &Simulation::should_output)
      .def_prop_ro("n_steps", &Simulation::n_steps)
      .def_prop_ro("dt", &Simulation::dt)
      .def("write_output", &Simulation::write_output)
      .def("deck_json", &Simulation::deck_json)
      .def("n_pd_neighbors", &Simulation::n_pd_neighbors, nb::arg("node"),
           "Number of peridynamic bonds at a node (after setup()).")
      .def("pd_neighbors",
           [](nb::object self, size_t i) {
             return nb::cast<Simulation &>(self).pd_neighbors(i, self);
           },
           nb::arg("node"), "Global node ids bonded to this node.")
      .def("bond_broken", &Simulation::bond_broken, nb::arg("node"),
           nb::arg("bond"))
      .def("set_bond_broken", &Simulation::set_bond_broken, nb::arg("node"),
           nb::arg("bond"), nb::arg("broken") = true)
      .def("break_bonds_in_slots", &Simulation::break_bonds_in_slots,
           nb::arg("x_centers"), nb::arg("width"), nb::arg("y_lo"),
           nb::arg("y_hi"), nb::arg("particle_id") = size_t(0),
           "Breaks the bonds entering the given notch slots.")
      .def("break_bonds_crossing_vertical",
           &Simulation::break_bonds_crossing_vertical, nb::arg("x_lines"),
           nb::arg("y_lo"), nb::arg("y_hi"), nb::arg("particle_id") = size_t(0),
           "Breaks the bonds crossing the given vertical lines.")
      .def_prop_ro("workdir", &Simulation::workdir)
      .def_prop_ro("n_nodes", &Simulation::n_nodes)
      .def_prop_ro("step_index", &Simulation::step_index)
      .def_prop_ro("time", &Simulation::time)
      .def_prop_ro("n_particles", &Simulation::n_particles)
      .def_prop_ro("n_walls", &Simulation::n_walls)
      .def_prop_ro("n_particles_all", &Simulation::n_particles_all)
      .def("particle",
           [](nb::object self, size_t i) {
             return make_view(self, nb::cast<Simulation &>(self).particle_only(i));
           },
           nb::arg("i"), "Grain i (walls excluded).")
      .def("wall",
           [](nb::object self, size_t i) {
             return make_view(self, nb::cast<Simulation &>(self).wall(i));
           },
           nb::arg("i"))
      .def("particle_all",
           [](nb::object self, size_t i) {
             return make_view(self, nb::cast<Simulation &>(self).particle_all(i));
           },
           nb::arg("i"), "Entry i of the combined grain + wall list.")
      .def_prop_ro("particles",
                   [](nb::object self) {
                     auto &s = nb::cast<Simulation &>(self);
                     nb::list out;
                     for (size_t i = 0; i < s.n_particles(); ++i)
                       out.append(make_view(self, s.particle_only(i)));
                     return out;
                   })
      .def_prop_ro("walls",
                   [](nb::object self) {
                     auto &s = nb::cast<Simulation &>(self);
                     nb::list out;
                     for (size_t i = 0; i < s.n_walls(); ++i)
                       out.append(make_view(self, s.wall(i)));
                     return out;
                   })
      // Whole-model node fields. These are writable views into the model, not
      // copies: re-fetch them after setup(), and copy if you need a snapshot.
      .def_prop_ro("reference",
                   [](nb::object self) {
                     return point_view(nb::cast<Simulation &>(self).model().d_xRef,
                                       self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro("position",
                   [](nb::object self) {
                     return point_view(nb::cast<Simulation &>(self).model().d_x,
                                       self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro("displacement",
                   [](nb::object self) {
                     return point_view(nb::cast<Simulation &>(self).model().d_u,
                                       self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro("velocity",
                   [](nb::object self) {
                     return point_view(nb::cast<Simulation &>(self).model().d_v,
                                       self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro("force",
                   [](nb::object self) {
                     return point_view(nb::cast<Simulation &>(self).model().d_f,
                                       self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro("volume",
                   [](nb::object self) {
                     auto &m = nb::cast<Simulation &>(self).model();
                     return scalar_view<double>(m.d_vol, 0, m.d_vol.size(), self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro("fixity",
                   [](nb::object self) {
                     auto &m = nb::cast<Simulation &>(self).model();
                     return scalar_view<uint8_t>(m.d_fix, 0, m.d_fix.size(), self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro("particle_id",
                   [](nb::object self) {
                     auto &m = nb::cast<Simulation &>(self).model();
                     return scalar_view<size_t>(m.d_ptId, 0, m.d_ptId.size(),
                                                self);
                   }, nb::rv_policy::automatic)
      .def_prop_ro(
          "damage",
          [](nb::object self) {
            auto &m = nb::cast<Simulation &>(self).model();
            return copy_damage(m.d_Z, 0, m.d_Z.size());
          },
          nb::rv_policy::automatic);
}
