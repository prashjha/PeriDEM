/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_MATERIALDECK_H
#define INP_MATERIALDECK_H

#include "util/io.h"
#include <cmath>
#include <string>
#include <vector>

namespace inp {

/*! @brief Structure for elastic properties and fracture properties */
struct MatData {

  /**
   * @name Elastic material properties
   */
  /**@{*/

  /*! @brief Young's elastic modulus */
  double d_E;

  /*! @brief Shear modulus or Lame second parameter */
  double d_G;

  /*! @brief Bulk modulus */
  double d_K;

  /*! @brief Poisson's ratio */
  double d_nu;

  /*! @brief Lame first parameter */
  double d_lambda;

  /*! @brief Lame second parameter */
  double d_mu;

  /** @}*/

  /**
   * @name Fracture properties
   */
  /**@{*/

  /*! @brief Critical stress intensity factor */
  double d_KIc;

  /*! @brief Critical energy release rate */
  double d_Gc;

  /** @}*/

  /*!
   * @brief Constructor
   */
  MatData()
      : d_E(-1.), d_G(-1.), d_K(-1.), d_nu(-1.), d_lambda(-1.), d_mu(-1.),
        d_KIc(-1.), d_Gc(-1.){};

  /*!
   * @brief Copy constructor
   *
   * @param md Another MatData object
   */
  MatData(const MatData &md)
      : d_E(md.d_E), d_G(md.d_G), d_K(md.d_K), d_nu(md.d_nu),
        d_lambda(md.d_lambda), d_mu(md.d_mu), d_KIc(md.d_KIc), d_Gc(md.d_Gc){};

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
    oss << tabS << "------- MatData --------" << std::endl << std::endl;
    oss << tabS << "Young's modulus = " << d_E << std::endl;
    oss << tabS << "Shear modulus = " << d_G << std::endl;
    oss << tabS << "Bulk modulus = " << d_K << std::endl;
    oss << tabS << "Poisson ratio = " << d_nu << std::endl;
    oss << tabS << "Lame parameter Lambda = " << d_lambda << std::endl;
    oss << tabS << "Lame parameter Mu = " << d_mu << std::endl;
    oss << tabS << "Critical stress intensity factor = " << d_KIc << std::endl;
    oss << tabS << "Critical energy release rate = " << d_Gc << std::endl;
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

  /**
   * @name Conversion methods
   */
  /**@{*/

  /*!
   * @brief Compute Poisson's ratio from Lame parameters
   * @param lambda Lame first parameter
   * @param mu Lame second parameter
   * @return nu Poisson's ratio
   */
  double toNu(double lambda, double mu) { return lambda * 0.5 / (lambda + mu); }

  /*!
   * @brief Compute Poisson's ratio from Young's and Shear modulus
   * @param E Youngs modulus
   * @param G Shear modulus
   * @return nu Poisson's ratio
   */
  double toNuEG(double E, double G) { return E * 0.5 / G - 1.; }

  /*!
   * @brief Compute Young's modulus E from Bulk modulus K and Poisson's ratio nu
   * @param K Bulk modulus
   * @param nu Poisson's ratio
   * @return E Young's modulus
   */
  double toE(double K, double nu) { return K * (3. * (1. - 2. * nu)); }

  /*!
   * @brief Compute Bulk modulus K from Young's modulus K and Poisson's ratio nu
   * @param E Young's modulus
   * @param nu Poisson's ratio
   * @return K Bulk modulus
   */
  double toK(double E, double nu) { return E / (3. * (1. - 2. * nu)); }

  /*!
   * @brief Compute Lame first parameter lambda from Young's modulus E
   * and Poisson's ratio nu
   * @param E Young's modulus
   * @param nu Poisson's ratio
   * @return lambda Lame first parameter
   */
  double toLambdaE(double E, double nu) {
    return E * nu / ((1. + nu) * (1. - 2. * nu));
  }

  /*!
   * @brief Compute Lame first parameter lambda from Bulk modulus K and
   * Poisson's ratio nu
   * @param K Bulk modulus
   * @param nu Poisson's ratio
   * @return lambda Lame first parameter
   */
  double toLambdaK(double K, double nu) { return 3. * K * nu / (1. + nu); }

  /*!
   * @brief Compute shear modulus from Young's modulus E and Poisson's ratio nu
   * @param E Young's modulus
   * @param nu Poisson's ratio
   * @return G Shear modulus
   */
  double toGE(double E, double nu) { return E / (2. * (1. + nu)); }

  /*!
   * @brief Compute shear modulus from Bulk modulus K and Poisson's ratio nu
   * @param K Bulk modulus
   * @param nu Poisson's ratio
   * @return G Shear modulus
   */
  double toGK(double K, double nu) {
    return 3. * K * (1. - 2. * nu) / (2. * (1. + nu));
  }

  /*!
   * @brief Compute Young's modulus E from Lame first parameter lambda and
   * Poisson's ratio nu
   * @param lambda Lame first parameter
   * @param nu Poisson's ratio
   * @return E Young's modulus
   */
  double toELambda(double lambda, double nu) {
    return lambda * (1. + nu) * (1. - 2. * nu) / nu;
  }

  /*!
   * @brief Compute critical energy release rate Gc from critical
   * stress-intensity factor KIc, Poisson's ratio nu, and Young's modulus E
   *
   * Below conversion from KIc to Gc assumes **plane-stress** condition. For
   * **plane-stress** condition, we need to modify the Young's modulus \f$
   * E\f$ to \f$ \frac{E}{1 - \nu^2} \f$ where \f$ \nu\f$ is the Poisson's
   * ratio.
   * @param KIc Critical stress-intensity factor
   * @param nu Poisson's ratio
   * @param E Young's modulus
   * @return Gc Critical energy release rate
   */
  double toGc(double KIc, double nu, double E) { return KIc * KIc / E; }

  /*!
   * @brief Compute critical stress-intensity factor KIc from critical energy
   * release rate Gc, Poisson's ratio \f$ nu\f$, and Young's modulus E
   *
   * Below conversion from Gc to KIc assumes **plane-stress** condition. For
   * **plane-stress** condition, we need to modify the Young's modulus \f$
   * E\f$ to \f$ \frac{E}{1 - \nu^2} \f$ where \f$ \nu\f$ is the Poisson's
   * ratio.
   * @param Gc Critical energy release rate
   * @param nu Poisson's ratio
   * @param E Young's modulus
   * @return KIc Critical stress-intensity factor
   */
  double toKIc(double Gc, double nu, double E) { return std::sqrt(Gc * E); }

  /** @}*/
};

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store material related data */
struct MaterialDeck {

  /*!
   * @brief Indicates if the 2-d simulation is of plane-strain type (thick
   * material) or plane-stress type (thin material)
   */
  bool d_isPlaneStrain;

  /*! @brief Material type */
  std::string d_materialType;

  /*! @brief Type of pairwise (bond-based) potential */
  size_t d_bondPotentialType;

  /*! @brief Type of hydrostatic (state-based) potential */
  size_t d_statePotentialType;

  /*! @brief Type of influence function */
  size_t d_influenceFnType;

  /*! @brief List of parameters for pairwise potential */
  std::vector<double> d_bondPotentialParams;

  /*! @brief List of parameters for hydrostatic potential */
  std::vector<double> d_statePotentialParams;

  /*! @brief List of parameters for influence function */
  std::vector<double> d_influenceFnParams;

  /*!
   * @brief Flag for irreversible breaking of bonds.
   *
   * True means bond breaking is irreversible.
   */
  bool d_irreversibleBondBreak;

  /*! @brief Flag for contribution to hydrostatic force from the broken bond */
  bool d_stateContributionFromBrokenBond;

  /*! @brief Factor to check if bond is broken */
  double d_checkScFactor;

  /*! @brief Compute Peridynamic material properties from elastic properties */
  bool d_computeParamsFromElastic;

  /*! @brief List of elastic and fracture properties */
  inp::MatData d_matData;

  /*! @brief Density of material */
  double d_density;

  /*! @brief Horizon for peridynamic interaction */
  double d_horizon;

  /*! @brief Horizon to mesh ratio */
  double d_horizonMeshRatio;

  /*!
   * @brief Constructor
   */
  MaterialDeck()
      : d_isPlaneStrain(false), d_bondPotentialType(0), d_statePotentialType(0),
        d_influenceFnType(0), d_irreversibleBondBreak(true),
        d_stateContributionFromBrokenBond(true), d_checkScFactor(1.),
        d_computeParamsFromElastic(true), d_matData(inp::MatData()),
        d_density(1.), d_horizon(-1.), d_horizonMeshRatio(-1.){};

  /*!
   * @brief Copy constructor
   *
   * @param md Another MaterialDeck object
   */
  MaterialDeck(const MaterialDeck &md)
      : d_isPlaneStrain(md.d_isPlaneStrain),
        d_materialType(md.d_materialType),
        d_bondPotentialType(md.d_bondPotentialType),
        d_statePotentialType(md.d_statePotentialType),
        d_influenceFnType(md.d_influenceFnType),
        d_bondPotentialParams(md.d_bondPotentialParams),
        d_statePotentialParams(md.d_statePotentialParams),
        d_influenceFnParams(md.d_influenceFnParams),
        d_irreversibleBondBreak(md.d_irreversibleBondBreak),
        d_stateContributionFromBrokenBond(md.d_stateContributionFromBrokenBond),
        d_checkScFactor(md.d_checkScFactor),
        d_computeParamsFromElastic(md.d_computeParamsFromElastic),
        d_matData(md.d_matData), d_density(md.d_density),
        d_horizon(md.d_horizon), d_horizonMeshRatio(md.d_horizonMeshRatio){};

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
    oss << tabS << "------- MaterialDeck --------" << std::endl << std::endl;
    oss << tabS << "Is plain strain = " << d_isPlaneStrain << std::endl;
    oss << tabS << "Material type = " << d_materialType << std::endl;
    oss << tabS << "Bond potential type = " << d_bondPotentialType << std::endl;
    oss << tabS << "Bond potential params = ["
        << util::io::printStr<double>(d_bondPotentialParams, 0) << "]"
        << std::endl;
    oss << tabS << "State potential type = " << d_statePotentialType
        << std::endl;
    oss << tabS << "State potential params = ["
        << util::io::printStr<double>(d_statePotentialParams, 0) << "]"
        << std::endl;
    oss << tabS << "Influence function type = " << d_influenceFnType
        << std::endl;
    oss << tabS << "Influence function params = ["
        << util::io::printStr<double>(d_influenceFnParams, 0) << "]"
        << std::endl;
    oss << tabS
        << "Irreversible bond breaking enabled = " << d_irreversibleBondBreak
        << std::endl;
    oss << tabS << "State contribution from broken bond enabled = "
        << d_stateContributionFromBrokenBond << std::endl;
    oss << tabS << "Check Sc factor = " << d_checkScFactor << std::endl;
    oss << tabS << "Compute parameters from elastic properties = "
        << d_computeParamsFromElastic << std::endl;
    oss << d_matData.printStr(nt + 1, lvl);
    oss << tabS << "Density = " << d_density << std::endl;
    oss << tabS << "Horizon = " << d_horizon << std::endl;
    oss << tabS << "Horizon to mesh ratio = " << d_horizonMeshRatio
        << std::endl;
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

#endif // INP_MATERIALDECK_H
