/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_CONTACTPAIRDECK_H
#define INP_CONTACTPAIRDECK_H


#include "deckField.h"
#include "util/io.h"
#include "util/json.h"

namespace inp {
  /**
   * \ingroup Input
   */
  /**@{*/

  /*! @brief Structure to read and store particle-particle contact related input
   * data */
  struct ContactPairDeck {

    /*! @brief contact radius */
    double d_contactR;

    /*! @brief Flag that indicates whether contact radius is to be computed */
    bool d_computeContactR;

    ///@{
    /*! @brief parameters for normal force */
    double d_vMax;
    double d_deltaMax;
    double d_Kn;
    double d_KnFactor;
    ///@}

    ///@{
    /*! @brief parameters for normal damping force */
    double d_eps;
    double d_betan;
    bool d_dampingOn;
    double d_betanFactor;
    ///@}

    ///@{
    /*! @brief parameters for frictional force */
    double d_mu;
    bool d_frictionOn;
    double d_K;
    ///@}

    /*!
     * @brief Constructor
     */
    ContactPairDeck(const json &j = json({}))
      : d_contactR(0.), d_computeContactR(true), d_vMax(0.), d_deltaMax(0.),
        d_Kn(0.), d_eps(1.), d_betan(0.), d_mu(0.), d_dampingOn(true),
        d_frictionOn(true), d_KnFactor(1.), d_betanFactor(1.), d_K(0.) {
      readFromJson(j);
    };

    /*!
     * @brief Constructor
     */
    ContactPairDeck(double contactR, bool computeContactR = true,
                    bool dampingOn = true, bool frictionOn = true,
                    double Kn = 0., double eps = 1., double mu = 0.,
                    double KnFactor = 1., double betanFactor = 1.,
                    double deltaMax = 1., double vMax = 0.)
        : d_contactR(contactR), d_computeContactR(computeContactR), d_vMax(vMax), d_deltaMax(deltaMax),
          d_Kn(Kn), d_eps(eps), d_betan(0.), d_mu(mu), d_dampingOn(dampingOn),
          d_frictionOn(frictionOn), d_KnFactor(KnFactor), d_betanFactor(betanFactor), d_K(0.) {
    };

    /*!
     * @brief Copy constructor
     *
     * @param cd Another ContactPairDeck object
     */
    ContactPairDeck(const ContactPairDeck &cd)
      : d_contactR(cd.d_contactR), d_computeContactR(cd.d_computeContactR),
        d_vMax(cd.d_vMax), d_deltaMax(cd.d_deltaMax),
        d_Kn(cd.d_Kn), d_eps(cd.d_eps), d_betan(cd.d_betan),
        d_mu(cd.d_mu), d_dampingOn(cd.d_dampingOn),
        d_frictionOn(cd.d_frictionOn), d_KnFactor(cd.d_KnFactor),
        d_betanFactor(cd.d_betanFactor), d_K(cd.d_K) {
    };

    /*!
     * @brief The fields of this deck, declared once
     *
     * The contact radius and the contact stiffness are each given in one of
     * two ways and are resolved in readFromJson. What remains maps one key to
     * one member.
     *
     * @return fields The field table
     */
    static const std::vector<Field<ContactPairDeck>> &fields() {
      static const std::vector<Field<ContactPairDeck>> f = {
          field(&ContactPairDeck::d_KnFactor, "Kn_Factor", 1.,
                "Normal stiffness is scaled by this"),
          field(&ContactPairDeck::d_dampingOn, "Damping_On", true,
                "Apply the normal damping force"),
          field(&ContactPairDeck::d_eps, "Epsilon", 1.,
                "Coefficient of restitution", {{}, 0., 1.}),
          field(&ContactPairDeck::d_betanFactor, "Beta_n_Factor", 1.,
                "Damping coefficient is scaled by this"),
          field(&ContactPairDeck::d_frictionOn, "Friction_On", true,
                "Apply the tangential friction force"),
          field(&ContactPairDeck::d_mu, "Friction_Coeff", 0.,
                "Coefficient of friction"),
          // Left out when it is not set, because a pair without friction
          // does not use it.
          field<ContactPairDeck, double>(
              &ContactPairDeck::d_K, "K", 0.,
              "Bulk modulus the tangential force is built from", {},
              [](const double &v) { return v > 1.E-10; }),
      };
      return f;
    }

    /*!
     * @brief Quantities this deck accepts in more than one form
     * @return groups The groups
     */
    static const std::vector<OneOf> &groups() {
      static const std::vector<OneOf> g = {
          {"the contact radius",
           {"Contact_Radius", "Contact_Radius_Factor"}},
          {"the contact stiffness", {"Kn", "V_Max"}},
      };
      return g;
    }

    /*!
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    /*!
     * @brief Returns the block with the given fields set
     *
     * The contact radius is given either as Contact_Radius, an absolute
     * length, or as Contact_Radius_Factor, a multiple of the mesh size. The
     * contact stiffness is given either as Kn, or as V_Max with an optional
     * Delta_Max, the speed and overlap the stiffness is derived from. Naming
     * one of each pair is what selects it; there is no flag and no value
     * standing in for "not set".
     *
     * @param given Field names and values to set, checked against the table
     * @return JSON object for this deck
     */
    static json getExampleJson(const json &given = json::object()) {
      const std::vector<std::string> extra = {"Contact_Radius",
                                              "Contact_Radius_Factor", "Kn",
                                              "V_Max", "Delta_Max"};
      checkKeys(given, fields(), extra);

      json g = given;

      // Damping with no coefficient is no damping.
      bool dampingOn = g.value("Damping_On", true);
      double betanFactor = g.value("Beta_n_Factor", 1.);
      if (dampingOn and betanFactor < 1.E-10)
        dampingOn = false;
      if (!dampingOn)
        betanFactor = 0.;
      g["Damping_On"] = dampingOn;
      g["Beta_n_Factor"] = betanFactor;

      if (g.value("Friction_On", true) and
          g.value("Friction_Coeff", 0.) < 1.E-10)
        throw std::runtime_error("Friction coefficient can not be zero.");

      if (g.find("Contact_Radius_Factor") != g.end() and
          g.at("Contact_Radius_Factor").get<double>() < 1.E-10)
        throw std::runtime_error("Contact radius factor can not be zero.");

      if (g.find("Kn") != g.end() and g.at("Kn").get<double>() < 1.E-10)
        throw std::runtime_error(
            "Kn can not be zero. Give V_Max instead to derive it.");

      // The speed is what the stiffness is derived from, so the overlap it
      // is measured against goes with it.
      if (g.find("V_Max") != g.end() and
          (g.find("Delta_Max") == g.end() or
           g.at("Delta_Max").get<double>() < 1.E-10))
        g["Delta_Max"] = 1.;

      json j = applyGiven(g, fields(), extra);
      checkGroups(j, groups());
      return j;
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {

      if (j.empty())
        return;

      checkGroups(j, groups());
      readFields(*this, j, fields());

      // A factor is applied to the mesh size, an absolute radius is not.
      d_computeContactR = j.find("Contact_Radius_Factor") != j.end();
      d_contactR = d_computeContactR ? j.at("Contact_Radius_Factor").get<double>()
                                     : j.at("Contact_Radius").get<double>();

      // Given a stiffness, the speed that produces the reference overlap
      // follows from it. Given that speed instead, the stiffness follows in
      // the contact force.
      if (j.find("Kn") != j.end()) {
        d_Kn = j.at("Kn");
        d_deltaMax = 1.;
        d_vMax = std::sqrt(d_Kn);
      } else {
        d_vMax = j.at("V_Max");
        d_deltaMax = j.value("Delta_Max", 1.);
      }

      if (d_betanFactor < 1.E-8)
        d_dampingOn = false;

      if (!d_dampingOn)
        d_betanFactor = 0.;

      if (d_frictionOn and d_mu < 1.E-10) {
        throw std::runtime_error("Friction coefficient can not be zero.");
      }
      if (d_frictionOn and d_K < 1.E-10) {
        throw std::runtime_error("Bulk modulus can not be zero.");
      }
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
      oss << tabS << "------- ContactPairDeck --------" << std::endl << std::endl;
      oss << tabS << "Contact radius = " << d_contactR << std::endl;
      oss << tabS << "v_max = " << d_vMax << ", Delta_max = " << d_deltaMax
          << ", Kn = " << d_Kn << std::endl;
      oss << tabS << "epsilon = " << d_eps << ", Beta_n = " << d_betan << std::endl;
      oss << tabS << "Friction coefficient = " << d_mu << std::endl;
      oss << tabS << "Damping status = " << d_dampingOn << std::endl;
      oss << tabS << "Kn factor = " << d_KnFactor
          << ", Beta n factor = " << d_betanFactor << std::endl;
      oss << tabS << "Bulk modulus = " << d_K << std::endl;
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

#endif // INP_CONTACTPAIRDECK_H
