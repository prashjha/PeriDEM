/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_CONTACTDECK_H
#define INP_CONTACTDECK_H

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
  ///@}

  ///@{
  /*! @brief parameters for normal damping force */
  double d_eps;
  double d_betan;
  ///@}

  ///@{
  /*! @brief parameters for frictional force */
  double d_mu;
  bool d_dampingOn;
  bool d_frictionOn;
  double d_KnFactor;
  double d_betanFactor;
  double d_kappa;
  ///@}

  /*!
   * @brief Constructor
   */
  ContactPairDeck()
      : d_contactR(0.), d_computeContactR(true), d_vMax(0.), d_deltaMax(0.),
        d_Kn(0.), d_eps(0.), d_betan(0.), d_mu(0.), d_dampingOn(true),
        d_frictionOn(true), d_KnFactor(1.), d_betanFactor(1.), d_kappa(1.) {};

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
        d_betanFactor(cd.d_betanFactor), d_kappa(cd.d_kappa){};

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
    oss << tabS << "Bulk modulus = " << d_kappa << std::endl;
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

  /*!
   * @brief Get contact force factor
   *
   * @param v1 volume of node 1
   * @param v2 volume of node 2
   * @return factor Contact force factor
   */
  double getKn(const double &v1, const double &v2) const {

    return d_Kn * (v1 * v2) / (v1 + v2);
  }

  /*! @copydoc getKn(const double &v1, const double &v2) const */
  double getKn(const double &v1, const double &v2) {

    return d_Kn * (v1 * v2) / (v1 + v2);
  }

  /*!
   * @brief Get contact force factor for particle-wall
   *
   * @param v volume of node in particle
   * @return factor Contact force factor
   */
  double getWKn(const double &v) const {

    return d_Kn * v;
  }

  /*! @copydoc getWKn(const double &v) const */
  double getWKn(const double &v) {

    return d_Kn * v;
  }

  /*!
   * @brief Get damping force factor for particle-wall
   *
   * @param v1 volume of node 1
   * @param v2 volume of node 2
   * @return factor Damping force factor
   */
  double getBetan(const double &v1, const double &v2) const {

    return d_betan * std::sqrt((v1 * v2) / (v1 + v2));
  }

  /*! @copydoc getBetan(const double &v1, const double &v2) const */
  double getBetan(const double &v1, const double &v2) {

    return d_betan * std::sqrt((v1 * v2) / (v1 + v2));
  }

  /*!
   * @brief Get damping force factor for particle-wall
   *
   * @param v volume of node in particle
   * @return factor Damping force factor
   */
  double getWBetan(const double &v) const {

    return d_betan * std::sqrt(v);
  }

  /*! @copydoc getWBetan(const double &v) const */
  double getWBetan(const double &v) {

    return d_betan * std::sqrt(v);
  }
};

/*! @brief Structure to read and store particle-particle contact related input
 * data */
struct ContactDeck {

  /*!
   * @brief Store contact parameters for each pair of zone
   */
  std::vector<std::vector<ContactPairDeck>> d_data;

  /*!
   * @brief Returns the contact data
   *
   * @param i Zone i
   * @param j Zone j
   * @return data Contact data between zone i and j
   */
  const ContactPairDeck &getContact(const size_t &i, const size_t &j) const {
    //return d_data[i < j ? i : j][i < j ? j : i];
    return d_data[i][j];
  }

  /*! @copydoc getContact(const size_t &i, const size_t &j) const */
  ContactPairDeck &getContact(const size_t &i, const size_t &j) {
    //return d_data[i < j ? i : j][i < j ? j : i];
    return d_data[i][j];
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
    oss << tabS << "------- ContactDeck --------" << std::endl << std::endl;
    for (size_t i =0; i<d_data.size(); i++) {
      for (size_t j = 0; j < d_data.size(); j++) {
        oss << tabS << "ContactPairDeck id = (" << i << "," << j << ") info:"
                  << std::endl;
        oss << getContact(i,j).printStr(nt+2, lvl);
      }
    }

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

#endif // INP_CONTACTDECK_H
