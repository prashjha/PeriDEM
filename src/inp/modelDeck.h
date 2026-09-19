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
      return json{
          {"Dimension",                        dim},
          {"Final_Time",                       tFinal},
          {"Time_Steps",                       Nt},
          {"Discretization_Type",              {{"Spatial", spatialDiscretization},
                                                   {"Time", timeDiscretization}}},
          {"Populate_ElementNodeConnectivity", populateElementNodeConnectivity},
          {"Quad_Approximation_Order",         quadOrder},
          {"Particle_Sim_Type",                particleSimType},
          {"MPI_Strategy",                     "auto"},
          {"Bond_Break",                       "tension"},
          {"Self_Contact",                     "broken_bond_kn"},
          {"Wall_Contact",                     "meshed"},
          {"Seed",                             seed}
      };
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {
      if (j.empty())
        return;

      d_dim = j.at("Dimension");
      d_tFinal = j.at("Final_Time");
      d_Nt = j.at("Time_Steps");
      d_spatialDiscretization = j.at("Discretization_Type").at("Spatial");
      d_timeDiscretization = j.at("Discretization_Type").at("Time");
      if (d_timeDiscretization == "central_difference" or d_timeDiscretization == "velocity_verlet")
        d_simType = "explicit";
      d_populateElementNodeConnectivity = j.value("Populate_ElementNodeConnectivity", true);
      d_quadOrder = j.value("Quad_Approximation_Order", size_t(2));
      d_particleSimType = j.value("Particle_Sim_Type", "Multi_Particle");
      d_mpiStrategy = j.value("MPI_Strategy", "auto");
      d_bondBreak = j.value("Bond_Break", "tension");
      d_selfContact = j.value("Self_Contact", "broken_bond_kn");
      d_wallContact = j.value("Wall_Contact", "meshed");
      d_seed = j.value("Seed", 0);

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

      if (d_mpiStrategy != "auto" && d_mpiStrategy != "none" &&
          d_mpiStrategy != "particle" && d_mpiStrategy != "dof") {
        throw std::runtime_error(
            util::io::Msg()
            << "Error: Model.MPI_Strategy must be auto|none|particle|dof.\n");
      }
      if (d_bondBreak != "tension" && d_bondBreak != "absolute_stretch") {
        throw std::runtime_error(
            util::io::Msg()
            << "Error: Model.Bond_Break must be tension|absolute_stretch.\n");
      }
      if (d_selfContact != "broken_bond_kn" &&
          d_selfContact != "reference_gap" && d_selfContact != "none") {
        throw std::runtime_error(
            util::io::Msg()
            << "Error: Model.Self_Contact must be "
            "broken_bond_kn|reference_gap|none.\n");
      }
      if (d_wallContact != "meshed" && d_wallContact != "analytical_plane") {
        throw std::runtime_error(
            util::io::Msg()
            << "Error: Model.Wall_Contact must be "
            "meshed|analytical_plane.\n");
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
      oss << tabS << "Simulation type = " << d_simType << std::endl;
      oss << tabS << "Restart active = " << d_isRestartActive << std::endl;
      oss << tabS << "Populate element-node connectivity data = " << d_populateElementNodeConnectivity <<
          std::endl;
      oss << tabS << "Order of quad approximation = " << d_quadOrder << std::endl;
      oss << tabS << "Spatial discretization type = " << d_spatialDiscretization
          << std::endl;
      oss << tabS << "Time discretization type = " << d_timeDiscretization
          << std::endl;
      oss << tabS << "Particle simulation type = " << d_particleSimType << std::endl;
      oss << tabS << "MPI strategy = " << d_mpiStrategy << std::endl;
      oss << tabS << "Bond break = " << d_bondBreak << std::endl;
      oss << tabS << "Self contact = " << d_selfContact << std::endl;
      oss << tabS << "Wall contact = " << d_wallContact << std::endl;
      oss << tabS << "Dimension = " << d_dim << std::endl;
      oss << tabS << "Final time = " << d_tFinal << std::endl;
      oss << tabS << "Time step size = " << d_dt << std::endl;
      oss << tabS << "Number of time step = " << d_Nt << std::endl;
      oss << tabS << "Seed = " << d_seed << std::endl;
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
