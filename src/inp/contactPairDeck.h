/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_CONTACTPAIRDECK_H
#define INP_CONTACTPAIRDECK_H

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
     * @brief Returns example JSON object for ModelDeck configuration
     * @return JSON object with example configuration
     */
    static json getExampleJson(double contactR = 0., bool computeContactR = true,
        bool dampingOn = true, bool frictionOn = true,
        double Kn = 0., double eps = 1., double mu = 0.,
        double KnFactor = 1., double betanFactor = 1.,
        double deltaMax = 1., double vMax = 0., double K = 0.) {

      auto j = json({});

      if (computeContactR) {
        if (contactR < 1E-10) {
          throw std::runtime_error("Conctar radius factor can not be zero.");
        }
        j["Contact_Radius_Factor"] = contactR;
      } else {
        j["Contact_Radius"] = contactR;
      }

      if (Kn < 1.E-10) {
        if (vMax < 1.E-10) throw std::runtime_error("Need V_Max parameter for contact force.");
        else j["V_Max"] = vMax;

        if (deltaMax < 1.E-10) deltaMax = 1.;
        j["Delta_Max"] = deltaMax;
      } else {
        j["Kn"] = Kn;
      }

      if (dampingOn and betanFactor < 1.E-10) dampingOn = false;
      if (!dampingOn) betanFactor = 0.;

      if (frictionOn and mu < 1.E-10) {
        throw std::runtime_error("Friction coefficient can not be zero.");
      }

      if (K > 1.E-10) 
        j["K"] = K;

      j["Damping_On"] = dampingOn;
      j["Epsilon"] = eps;

      j["Friction_On"] = frictionOn;
      j["Friction_Coeff"] = mu;

      j["Kn_Factor"] = KnFactor;
      j["Beta_n_Factor"] = betanFactor;

      return j;
    }

    /*!
     * @brief Reads from json object
     */
    void readFromJson(const json &j) {

      if (j.empty())
        return;

      if (j.find("Contact_Radius_Factor") != j.end()) {
        d_computeContactR = true;
        d_contactR = j.at("Contact_Radius_Factor");
      } else {
        if (j.find("Contact_Radius") != j.end()) throw std::runtime_error("Need Contact_Radius or Contact_Radius_Factor.");

        d_computeContactR = false;
        d_contactR = j.at("Contact_Radius");
      }

      if (j.find("Kn") != j.end()) {
        d_Kn = j.at("Kn");
        d_deltaMax = 1.;
        d_vMax = std::sqrt(d_Kn);
      } else {
        if (j.find("V_Max") == j.end()) throw std::runtime_error("V_Max is needed for contact.");

        d_vMax = j.at("V_Max");
        d_deltaMax = j.value("Delta_Max", 1.);
      }
      d_KnFactor = j.value("Kn_Factor", 1.);

      d_dampingOn = j.value("Damping_On", true);
      d_eps = j.value("Epsilon", 1.);
      d_betanFactor = j.value("Beta_n_Factor", 1.);
      if (d_betanFactor < 1.E-8)
        d_dampingOn = false;

      if (!d_dampingOn)
        d_betanFactor = 0.;

      d_frictionOn = j.value("Friction_On", true);
      d_mu = j.value("Friction_Coeff", 0.);
      d_K = j.value("K", 0.);

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
