/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_MODELDECK_H
#define INP_MODELDECK_H

#include <string>
#include <stdexcept>
#include "deckField.h"
#include "util/io.h"
#include "util/json.h"

namespace inp {
  /**
   * \ingroup Input
   */
  /**@{*/

  /*! @brief Structure to read and store model related input data */
  struct ModelDeck {
    /*!
     * @brief Simulation type
     *
     * List of allowed values are:
     * - \a explicit
     * - \a implicit
     */
    std::string d_simType;

    /*! @brief Flag indicating if this restart problem */
    bool d_isRestartActive;

    /*!
     * @brief Tag for spatial discretization
     *
     * List of allowed values are:
     * - \a **finite_difference**
     * - \a **weak_finite_element**
     * - \a **nodal_finite_element**
     * - \a **truss_finite_element**
     */
    std::string d_spatialDiscretization;

    /*!
     * @brief Tag for time discretization
     *
     * List of allowed values are:
     * - *empty string*
     * - \a **central_difference**
     * - \a **velocity_verlet**
     */
    std::string d_timeDiscretization;

    /*! @brief Flag to indicate if we should populate element-node connectivity data in meshes */
    bool d_populateElementNodeConnectivity;

    /*! @brief Order of quadrature approximation for strain and stress computation (default is 1) */
    size_t d_quadOrder;

    /*! @brief Specify if this is single or multi particle simulation
     * Expected value is either 'Single_Particle' or 'Multi_Particle'.
     *
     * This flag is used to populate input deck data. For the case when
     * we consider single particle and its deformation, one do not have to specify data such as zones and contact.
     * */
    std::string d_particleSimType;

    /*!
     * @brief MPI domain-decomposition strategy (independent of Particle_Sim_Type).
     *
     * Allowed: `auto` (default), `none`, `particle`, `dof`.
     * - auto: Multi_Particle → particle; Single_Particle → dof
     * - none: no domain split (use one MPI rank)
     * - particle: distribute whole particles across ranks (Particle-MPI)
     * - dof: distribute nodes/DOFs across ranks (DOF-MPI)
     */
    std::string d_mpiStrategy;

    /*!
     * @brief Bond failure criterion: `tension` (default) or `absolute_stretch`.
     */
    std::string d_bondBreak;

    /*!
     * @brief Rigid translating particles: `{"Id": <id>, "Mass": <mass>}`.
     * Nodal force densities are set to density * (net force / Mass).
     */
    std::vector<std::pair<size_t, double>> d_rigidParticles;

    /*!
     * @brief Intra-body self-contact law name (default `broken_bond_kn`).
     */
    std::string d_selfContact;

    /*!
     * @brief Wall contact representation: `meshed` (default) or `analytical_plane`.
     */
    std::string d_wallContact;

    /*! @brief Dimension */
    size_t d_dim;

    /*! @brief Final simulation time */
    double d_tFinal;

    /*! @brief Size of time steps */
    double d_dt;

    /*! @brief Number of time steps */
    size_t d_Nt;

    /*! @brief Seed for random calculations (if any) */
    int d_seed;

    /*!
     * @brief Constructor
     */
    ModelDeck(const json &j = json({}))
        : d_dim(0), d_isRestartActive(false), d_populateElementNodeConnectivity(false),
          d_tFinal(0.), d_dt(0.), d_Nt(0),
          d_particleSimType(""), d_mpiStrategy("auto"),
          d_bondBreak("tension"), d_selfContact("broken_bond_kn"),
          d_wallContact("meshed"), d_seed(0), d_quadOrder(1) {

      readFromJson(j);
    };

    ModelDeck(size_t dim, double tFinal = 1.0, size_t Nt = 10,
              std::string spatialDiscretization = "finite_difference",
              std::string timeDiscretization = "central_difference",
              bool populateElementNodeConnectivity = true,
              size_t quadOrder = 2,
              std::string particleSimType = "Multi_Particle",
              int seed = 0)
        : d_dim(dim), d_isRestartActive(false),
          d_spatialDiscretization(spatialDiscretization),
          d_timeDiscretization(timeDiscretization),
          d_populateElementNodeConnectivity(populateElementNodeConnectivity),
          d_tFinal(tFinal), d_dt(0.), d_Nt(Nt),
          d_particleSimType(particleSimType), d_mpiStrategy("auto"),
          d_bondBreak("tension"), d_selfContact("broken_bond_kn"),
          d_wallContact("meshed"), d_seed(seed), d_quadOrder(quadOrder) {

      if (d_timeDiscretization == "central_difference" or d_timeDiscretization == "velocity_verlet")
        d_simType = "explicit";

      if (std::abs(d_tFinal) < 1.0E-10 or d_Nt <= 0) {
        throw std::runtime_error(
            util::io::Msg()
            << "Error: Check Final_Time and Time_Steps data.\n");
      }

      d_dt = d_tFinal / d_Nt;
    };

    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    static json getExampleJson(size_t dim = 2, double tFinal = 1.0, size_t Nt = 10,
                               std::string spatialDiscretization = "finite_difference",
                               std::string timeDiscretization = "central_difference",
                               bool populateElementNodeConnectivity = true,
                               size_t quadOrder = 2,
                               std::string particleSimType = "Multi_Particle",
                               int seed = 0) {
      return getExampleJson(
          json{{"Dimension", dim},
               {"Final_Time", tFinal},
               {"Time_Steps", Nt},
               {"Discretization_Type",
                json{{"Spatial", spatialDiscretization},
                     {"Time", timeDiscretization}}},
               {"Populate_ElementNodeConnectivity",
                populateElementNodeConnectivity},
               {"Quad_Approximation_Order", quadOrder},
               {"Particle_Sim_Type", particleSimType},
               {"Seed", seed}});
    }

    /*!
     * @brief The fields of this deck, declared once
     *
     * Reading, writing, printing and the schema are generated from this
     * table. The discretization block, the simulation type it implies, the
     * time step and the rigid particles are read in readDerived.
     *
     * @return fields The field table
     */
    static const std::vector<Field<ModelDeck>> &fields() {
      static const std::vector<Field<ModelDeck>> f = {
          field(&ModelDeck::d_dim, "Dimension", size_t(2),
                "Spatial dimension", {{size_t(1), size_t(2), size_t(3)}, {}, {}}),
          field(&ModelDeck::d_tFinal, "Final_Time", 1.0,
                "End of the integration window"),
          field(&ModelDeck::d_Nt, "Time_Steps", size_t(10),
                "Number of time steps", {{}, size_t(1), {}}),
          field(&ModelDeck::d_populateElementNodeConnectivity,
                "Populate_ElementNodeConnectivity", true,
                "Build the element to node map, which strain output needs"),
          field(&ModelDeck::d_quadOrder, "Quad_Approximation_Order", size_t(2),
                "Order of the quadrature rule"),
          field(&ModelDeck::d_particleSimType, "Particle_Sim_Type",
                std::string("Multi_Particle"), "One body or many",
                {{"Multi_Particle", "Single_Particle"}, {}, {}}),
          field(&ModelDeck::d_mpiStrategy, "MPI_Strategy", std::string("auto"),
                "What is distributed across ranks",
                {{"auto", "none", "particle", "dof"}, {}, {}}),
          field(&ModelDeck::d_bondBreak, "Bond_Break", std::string("tension"),
                "Criterion for breaking a bond",
                {{"tension", "absolute_stretch"}, {}, {}}),
          field(&ModelDeck::d_selfContact, "Self_Contact",
                std::string("broken_bond_kn"),
                "Contact across broken bonds inside one body",
                {{"broken_bond_kn", "reference_gap", "none"}, {}, {}}),
          field(&ModelDeck::d_wallContact, "Wall_Contact",
                std::string("meshed"), "How a wall is represented for contact",
                {{"meshed", "analytical_plane"}, {}, {}}),
          field(&ModelDeck::d_seed, "Seed", 0,
                "Seed of the random number generator"),
      };
      return f;
    }

    /*!
     * @brief Returns the block with the given fields set
     * @param given Field names and values to set, checked against the table
     * @return JSON object for this deck
     */
    static json getExampleJson(const json &given) {
      json j = applyGiven(given, fields(),
                          {"Discretization_Type", "Rigid_Particles"});
      if (j.find("Discretization_Type") == j.end())
        j["Discretization_Type"] = json{{"Spatial", "finite_difference"},
                                        {"Time", "central_difference"}};
      return j;
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;

      readFields(*this, j, fields());

      d_spatialDiscretization = j.at("Discretization_Type").at("Spatial");
      d_timeDiscretization = j.at("Discretization_Type").at("Time");
      if (d_timeDiscretization == "central_difference" or
          d_timeDiscretization == "velocity_verlet")
        d_simType = "explicit";

      d_rigidParticles.clear();
      if (j.find("Rigid_Particles") != j.end()) {
        for (const auto &r : j.at("Rigid_Particles")) {
          const double mass = r.value("Mass", -1.);
          if (!(mass > 0.)) {
            throw std::runtime_error(
                util::io::Msg()
                << "Error: Model.Rigid_Particles entries need Mass > 0.\n");
          }
          d_rigidParticles.emplace_back(r.at("Id").get<size_t>(), mass);
        }
      }

      if (std::abs(d_tFinal) < 1.0E-10 or d_Nt <= 0) {
        throw std::runtime_error(
            util::io::Msg()
            << "Error: Check Final_Time and Time_Steps data.\n");
      }

      d_dt = d_tFinal / d_Nt;
    }

    /*!
     * @brief Returns the string containing printable information about the object
     *
     * @param nt Number of tabs to append before printing
     * @param lvl Information level (higher means more information)
     * @return string String containing printable information about the object
     */
    std::string printStr(int nt = 0, int lvl = 0) const {
      auto tabS = util::io::getTabS(nt);
      std::ostringstream oss;
      oss << tabS << "------- ModelDeck --------" << std::endl << std::endl;
      printFields(*this, oss, fields(), tabS);
      oss << tabS << "Discretization_Type.Spatial = " << d_spatialDiscretization
          << std::endl;
      oss << tabS << "Discretization_Type.Time = " << d_timeDiscretization
          << std::endl;
      oss << tabS << "Simulation type = " << d_simType << std::endl;
      oss << tabS << "Time step size = " << d_dt << std::endl;
      oss << tabS << "Restart active = " << d_isRestartActive << std::endl;
      oss << tabS << std::endl;

      return oss.str();
    }

    /*!
     * @brief Prints the information about the object
     *
     * @param nt Number of tabs to append before printing
     * @param lvl Information level (higher means more information)
     */
    void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
  };

  /** @}*/
} // namespace inp

#endif // INP_MODELDECK_H
