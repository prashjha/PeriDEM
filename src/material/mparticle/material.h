/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef MATERIAL_PARTILCE_MATERIAL_H
#define MATERIAL_PARTILCE_MATERIAL_H

#include "influenceFn.h"
#include "inp/materialDeck.h"
#include "util/function.h"
#include "util/point.h" // definition of Point
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

/*! @brief Dimension of the domain */
size_t dimension = 0;

/*! @brief Is plane-stress condition active */
bool is_plane_strain = false;

/*! @brief Store pointer to influence function globally */
std::shared_ptr<material::BaseInfluenceFn> influence_fn;

/*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
double getGlobalInfFn(const double &r) {
    return influence_fn->getInfFn(r);
}

/*!
 * @brief Returns the moment of influence function
 *
 * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
 * i^{th} \f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
 *
 * @param i ith moment
 * @return moment Moment
 */
double getGlobalMoment(const size_t &i) {return influence_fn->getMoment(i);}
}

namespace material {

/*!
 * @brief Collection of methods and database related to peridynamic material
 *
 * At present we have implemented both bond-based and state-based model. We
 * consider \b RNP regularized potential proposed and studied in [Lipton
 * 2016](https://link.springer.com/article/10.1007/s10659-015-9564-z),
 * [Jha and Lipton 2018](https://doi.org/10.1137/17M1112236), [Diehl et al
 * 2018](https://arxiv.org/abs/1806.06917),
 * [Jha and Lipton 2019](https://doi.org/10.1016/j.cma.2019.03.024). We have
 * also implemented PMB material model (Prototypical micro-elastic brittle
 * material), see [Silling 2000](https://www.sciencedirect
 * .com/science/article/pii/S0022509699000290).
 */

/*!
 * @brief A class providing methods to compute energy density and force of
 * peridynamic material
 */
class Material {

public:
  /*!
   * @brief Constructor
   * @param name Name of material model
   */
  explicit Material(std::string name = "") : d_name(name) {}

  /*!
   * @brief Destructor
   *
   * Make it virtual so that class inheriting from it are destroyed properly.
   */
  virtual ~Material() {}

  /*!
   * @brief Returns name of the material
   * @return string Name
   */
  std::string name() { return d_name; }

  /*!
   * @brief Returns dimension of the problem
   * @return dim Dimension
   */
  size_t getDimension() { return dimension; }

  /*!
   * @brief Returns plane-strain condition
   * @return bool True if plane-strain active
   */
  bool isPlaneStrain() { return is_plane_strain; }

  /*!
   * @brief Returns true if state-based potential is active
   * @return bool True/false
   */
  virtual bool isStateActive() const = 0;

  /*!
   * @brief Returns energy and force between bond due to pairwise interaction
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param break_bonds Flag to specify whether bonds are allowed to break or not
   * @return value Pair of energy and force
   */
  virtual std::pair<double, double>
  getBondEF(const double &r, const double &s, bool &fs,
            const bool &break_bonds) const = 0;

  /*!
   * @brief Returns energy and force between bond due to state-based model
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param mx Weighted volume at node
   * @param thetax Dilation
   * @return value Pair of energy and force
   */
  virtual std::pair<double, double>
  getBondEF(const double &r, const double &s, bool &fs, const double
  &mx, const double &thetax) const = 0;

  /*!
   * @brief Returns the unit vector along which bond-force acts
   *
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return vector Unit vector
   */
  virtual util::Point getBondForceDirection(const util::Point &dx,
                                             const util::Point &du) const = 0;

  /*!
   * @brief Returns the bond strain
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return strain Bond strain \f$ S = \frac{du \cdot dx}{|dx|^2} \f$
   */
  virtual double getS(const util::Point &dx, const util::Point &du) const = 0;

  /*!
   * @brief Returns critical bond strain
   *
   * @param r Reference length of bond
   * @return strain Critical strain
   */
  virtual double getSc(const double &r) const = 0;

  /*!
   * @brief Returns the density of the material
   * @return density Density of the material
   */
  virtual double getDensity() const = 0;

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  virtual double getInfFn(const double &r) const = 0;

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return value Moment
   */
  virtual double getMoment(const size_t &i) const = 0;

  /*!
   * @brief Returns horizon
   *
   * @return horizon Horizon
   */
  virtual double getHorizon() const = 0;

  /*!
   * @brief Computes elastic and fracture material properties and returns the
   * data
   *
   * @param dim Dimension of the problem
   * @return Data Material data
   */
  virtual inp::MatData computeMaterialProperties(const size_t &dim) const = 0;

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  virtual std::string printStr(int nt, int lvl) const {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- particle::Material --------" << std::endl
        << std::endl;
    oss << tabS << "Abstract class of peridynamic materials" << std::endl;
    oss << tabS << "See RnpMaterial and PmbMaterial for implementation"
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
  virtual void print(int nt, int lvl) const { std::cout << printStr(nt, lvl); }

  /*! @brief Prints the information about the object */
  virtual void print() const { print(0, 0); }

private:
  /*! @brief Name of the material */
  std::string d_name;
};

/*!
 * @brief A class providing methods to compute energy density and force of
 * peridynamic material
 */
class RnpMaterial : public Material {

public:
  /*!
   * @brief Constructor
   * @param deck Input deck which contains user-specified information
   * @param dim Dimension
   * @param horizon Horizon
   */
  RnpMaterial(inp::MaterialDeck &deck, const size_t &dim, const double &horizon)
      : Material("RNPBond"), d_horizon(horizon), d_density(deck.d_density),
        d_C(0.), d_beta(0.), d_rbar(0.), d_invFactor(0.),
        d_factorSc(deck.d_checkScFactor),
        d_irrevBondBreak(deck.d_irreversibleBondBreak) {

    // set global fields
    if (dimension != dim)
      dimension = dim;

    if (is_plane_strain != deck.d_isPlaneStrain)
      is_plane_strain = deck.d_isPlaneStrain;

    // create influence function
    if (deck.d_influenceFnType == 0) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::ConstInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 1) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::LinearInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 2) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::GaussianInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else {
      std::cerr << "Error: Influence function type = "
                << deck.d_influenceFnType
                << " is invalid.\n";
      exit(1);
    }

    if (dim == 1)
      d_invFactor = std::pow(horizon, 2) * 2.;
    else if (dim == 2)
      d_invFactor = std::pow(horizon, 3) * M_PI;
    else if (dim == 3)
      d_invFactor = std::pow(horizon, 4) * 4. * M_PI / 3.;

    // check if we need to compute the material parameters
    if (deck.d_computeParamsFromElastic)
      computeParameters(deck, dim);
    else {
      d_C = deck.d_bondPotentialParams[0];
      d_beta = deck.d_bondPotentialParams[1];
      d_rbar = std::sqrt(0.5 / d_beta);
    }
  };

  /*! @copydoc Material::isStateActive() const */
  bool isStateActive() const override { return false; };

  /*!
   * @brief Returns energy and force between bond due to pairwise interaction
   *
   * Peridynamic energy at point \f$ x \f$ is
   * \f[ e(x) = \frac{1}{|B_\epsilon(0)|} \int_{B_\epsilon(x)}
   * \frac{J^\epsilon(|y-x|)}{\epsilon} \psi(|y-x|S^2) dy \f]
   * and force at point x is
   * \f[ f(x) = \frac{4}{|B_\epsilon(0)|} \int_{B_\epsilon(x)}
   * \frac{J^\epsilon(|y-x|)}{\epsilon} \psi'(|y-x|S^2) S \frac{y-x}{|y-x|}
   * dy, \f]
   * where \f$ \psi(r) = C(1-\exp(-\beta r))\f$.
   *
   * For given initial bond length \f$ r \f$ and bond strain \f$ s\f$, this
   * function returns pair of
   * \f[ \hat{e} =  \frac{J^\epsilon(r)}{\epsilon |B_\epsilon(0)|} \psi(r s^2)
   * \f]
   * and
   * \f[ \hat{f} = \frac{4 J^\epsilon(r) s}{\epsilon |B_\epsilon(0)|}
   * \psi'(r s^2). \f]
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param break_bonds Flag to specify whether bonds are allowed to break or not
   * @return value Pair of energy and force
   */
  std::pair<double, double> getBondEF(const double &r, const double &s,
                                      bool &fs,
                                      const bool &break_bonds) const override {

    if (break_bonds) {
      // check if fracture state of the bond need to be updated
      if (d_irrevBondBreak && !fs &&
          util::isGreater(std::abs(s), d_factorSc * getSc(r)))
        fs = true;

      // if bond is not fractured, return energy and force from nonlinear
      // potential otherwise return energy of fractured bond, and zero force
      if (!fs)
        return std::make_pair(
            getInfFn(r) * d_C *
                (1. - std::exp(-d_beta * r * s * s)) / d_invFactor,
            getInfFn(r) * 4. * s * d_C * d_beta *
                std::exp(-d_beta * r * s * s) / d_invFactor);
      else
        return std::make_pair(d_C / d_invFactor, 0.);
    } else {
      return std::make_pair(getInfFn(r) * d_C * d_beta *
                                r * s * s / d_invFactor,
                            getInfFn(r) * 4. * s * d_C *
                                d_beta / d_invFactor);
    }
  };

  /*!
   * @brief Returns energy and force between bond due to state-based model
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param mx Weighted volume at node
   * @param thetax Dilation
   * @return value Pair of energy and force
   */
  std::pair<double, double>
  getBondEF(const double &r, const double &s, bool &fs, const double
  &mx, const double &thetax) const override {

    return this->getBondEF(r, s, fs, true);
  };

  /*!
   * @brief Returns the unit vector along which bond-force acts
   *
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return vector Unit vector
   */
  util::Point getBondForceDirection(const util::Point &dx,
                                     const util::Point &du) const override {
    return dx / dx.length();
  };

  /*!
   * @brief Returns the bond strain
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return strain Bond strain \f$ S = \frac{du \cdot dx}{|dx|^2} \f$
   */
  double getS(const util::Point &dx, const util::Point &du) const override {
    return dx.dot(du) / dx.dot(dx);
  };

  /*!
   * @brief Returns critical bond strain
   *
   * @param r Reference length of bond
   * @return strain Critical strain
   */
  double getSc(const double &r) const override {
    return d_rbar / std::sqrt(r);
  };

  /*!
   * @brief Returns the density of the material
   * @return density Density of the material
   */
  double getDensity() const override { return d_density; };

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  double getInfFn(const double &r) const override {
    return getGlobalInfFn(r / d_horizon);
  };

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return value Moment
   */
  double getMoment(const size_t &i) const override {
    return getGlobalMoment(i);
  };

  /*!
   * @brief Returns horizon
   * @return horizon Horizon
   */
  double getHorizon() const override { return d_horizon; };

  /*!
   * @brief Computes elastic and fracture material properties and returns the
   * data
   *
   * @param dim Dimension of the problem
   * @return Data Material data
   */
  inp::MatData computeMaterialProperties(const size_t &dim) const override {

    auto data = inp::MatData();

    // set Poisson's ratio to 1/4
    data.d_nu = 0.25;

    // get moment of influence function
    double M = getMoment(dim);

    // compute peridynamic parameters
    if (dim == 2) {
      data.d_Gc = 4. * M * d_C / M_PI;
      data.d_lambda = d_C * M * d_beta / 4.;
    } else if (dim == 3) {
      data.d_Gc = 3. * M * d_C / 2.;
      data.d_lambda = d_C * M * d_beta / 5.;
    }
    data.d_mu = data.d_lambda;
    data.d_G = data.d_lambda;
    data.d_E = data.toELambda(data.d_lambda,
                              data.d_nu);
    data.d_K =
        data.toK(data.d_E, data.d_nu);
    data.d_KIc = data.toKIc(
        data.d_Gc, data.d_nu, data.d_E);

    return data;
  };

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt, int lvl) const override {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- particle::RnpMaterial --------" << std::endl
        << std::endl;
    oss << tabS << "State active = " << 0 << std::endl;
    oss << tabS << "Horizon = " << d_horizon << std::endl;
    oss << tabS << "Influence fn address = " << influence_fn.get() << std::endl;
    oss << tabS << "Influence fn info: " << std::endl;
    oss << influence_fn->printStr(nt + 1, lvl);
    oss << tabS << "Peridynamic parameters: " << std::endl;
    oss << tabS << "  C = " << d_C << std::endl;
    oss << tabS << "  beta = " << d_beta << std::endl;
    oss << tabS << "  r_bar = " << d_rbar << std::endl;
    oss << tabS << "  inv_factor = " << d_invFactor << std::endl;
    oss << tabS << "  factor_Sc = " << d_factorSc << std::endl;
    oss << tabS << "  irrev_bond_breaking = " << d_irrevBondBreak << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  }

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  }

  /*! @brief Prints the information about the object */
  void print() const override { print(0, 0); }

private:
  /*!
   * @brief Compute material model parameters
   *
   * @param deck MaterialDeck
   * @param dim Dimension of the domain
   */
  void computeParameters(inp::MaterialDeck &deck, const size_t &dim) {
    //
    // Need following elastic and fracture properties
    // 1. E or K
    // 2. Gc or KIc
    // For bond-based, Poisson's ratio is fixed to 1/4
    //
    if (util::isLess(deck.d_matData.d_E, 0.) &&
        util::isLess(deck.d_matData.d_K, 0.)) {
      std::cerr << "Error: Require either Young's modulus E or Bulk modulus K"
                   " to compute the RNP bond-based peridynamic parameters.\n";
      exit(1);
    }
    if (util::isGreater(deck.d_matData.d_E, 0.) &&
        util::isGreater(deck.d_matData.d_K, 0.)) {
      std::cout << "Warning: Both Young's modulus E and Bulk modulus K are "
                   "provided.\n";
      std::cout << "Warning: To compute the RNP bond-based peridynamic "
                   "parameters, we only require one of those.\n";
      std::cout
          << "Warning: Selecting Young's modulus to compute parameters.\n";
    }

    if (util::isLess(deck.d_matData.d_Gc, 0.) &&
        util::isLess(deck.d_matData.d_KIc, 0.)) {
      std::cerr << "Error: Require either critical energy release rate Gc or "
                   "critical stress intensity factor KIc to compute the RNP "
                   "bond-based peridynamic parameters.\n";
      exit(1);
    } else if (util::isGreater(deck.d_matData.d_Gc, 0.) &&
               util::isGreater(deck.d_matData.d_KIc, 0.)) {
      std::cout << "Warning: Both critical energy release rate Gc and critical "
                   "stress intensity factor KIc are provided.\n";
      std::cout << "Warning: To compute the RNP bond-based peridynamic "
                   "parameters, we only require one of those.\n";
      std::cout << "Warning: Selecting critical energy release rate Gc to "
                   "compute parameters.\n";
    }

    // set Poisson's ratio to 1/4
    deck.d_matData.d_nu = 0.25;

    // compute E if not provided or K if not provided
    if (deck.d_matData.d_E > 0.)
      deck.d_matData.d_K =
          deck.d_matData.toK(deck.d_matData.d_E, deck.d_matData.d_nu);

    if (deck.d_matData.d_K > 0. && deck.d_matData.d_E < 0.)
      deck.d_matData.d_E =
          deck.d_matData.toE(deck.d_matData.d_K, deck.d_matData.d_nu);

    if (deck.d_matData.d_Gc > 0.)
      deck.d_matData.d_KIc = deck.d_matData.toKIc(
          deck.d_matData.d_Gc, deck.d_matData.d_nu, deck.d_matData.d_E);

    if (deck.d_matData.d_KIc > 0. && deck.d_matData.d_Gc < 0.)
      deck.d_matData.d_Gc = deck.d_matData.toGc(
          deck.d_matData.d_KIc, deck.d_matData.d_nu, deck.d_matData.d_E);

    // compute lame parameter
    deck.d_matData.d_lambda =
        deck.d_matData.toLambdaE(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_G =
        deck.d_matData.toGE(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_mu = deck.d_matData.d_G;

    // get moment of influence function
    double M = getMoment(dim);

    // compute peridynamic parameters
    if (dim == 2) {
      d_C = M_PI * deck.d_matData.d_Gc / (4. * M);
      d_beta = 4. * deck.d_matData.d_lambda / (d_C * M);
    } else if (dim == 3) {
      d_C = 2. * deck.d_matData.d_Gc / (3. * M);
      d_beta = 5. * deck.d_matData.d_lambda / (d_C * M);
    }

    d_rbar = std::sqrt(0.5 / d_beta);
  };

private:

  /*! @brief Horizon */
  double d_horizon;

  /*! @brief Density */
  double d_density;

  /**
   * @name Material parameters
   */
  /**@{*/

  /*! @brief Parameter C */
  double d_C;

  /*! @brief Parameter \f$ \beta \f$ */
  double d_beta;

  /** @}*/

  /*! @brief Inflection point of nonlinear function = \f$ 1/\sqrt{2\beta}\f$ */
  double d_rbar;

  /*! @brief Inverse of factor = \f$ \epsilon |B_\epsilon(0)|\f$ */
  double d_invFactor;

  /*! @brief Factor to multiply to critical strain to check if bond is
   * fractured
   *
   * For nonlinear model, we consider bond is broken when it
   * exceeds 10 times of critical strain. Typical value of factor is 10.
   */
  double d_factorSc;

  /*! @brief Flag which indicates if the breaking of bond is irreversible */
  bool d_irrevBondBreak;
};

/*!
 * @brief A class providing methods to compute energy density and force of
 * peridynamic material
 */
class PmbMaterial : public Material {

public:
  /*!
   * @brief Constructor
   * @param deck Input deck which contains user-specified information
   * @param dim Dimension
   * @param horizon Horizon
   */
  PmbMaterial(inp::MaterialDeck &deck, const size_t &dim, const double &horizon)
      : Material("PMBBond"), d_horizon(horizon), d_density(deck.d_density),
        d_c(0.), d_s0(0.) {

    // set global fields
    if (dimension != dim)
      dimension = dim;

    if (is_plane_strain != deck.d_isPlaneStrain)
      is_plane_strain = deck.d_isPlaneStrain;

    // create influence function
    if (deck.d_influenceFnType == 0) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::ConstInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 1) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::LinearInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 2) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::GaussianInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else {
      std::cerr << "Error: Influence function type = "
                << deck.d_influenceFnType
                << " is invalid.\n";
      exit(1);
    }

    // check if we need to compute the material parameters
    if (deck.d_computeParamsFromElastic)
      computeParameters(deck, dim);
    else {
      d_c = deck.d_bondPotentialParams[0];
      d_s0 = deck.d_bondPotentialParams[1];
    }
  };

  /*!
   * @brief Returns true if state-based potential is active
   * @return bool True/false
   */
  bool isStateActive() const override { return false; };

  /*!
   * @brief Returns energy and force between bond due to pairwise interaction
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param break_bonds Flag to specify whether bonds are allowed to break or not
   * @return value Pair of energy and force
   */
  std::pair<double, double> getBondEF(const double &r, const double &s,
                                      bool &fs,
                                      const bool &break_bonds) const override {

    if (!break_bonds)
      return std::make_pair(getInfFn(r) * 0.5 * d_c * s *
                                s * r,
                            getInfFn(r) * d_c * s);

    // Fracture: literature PMB is tension-only (s > s0). Absolute stretch is
    // applied in pdForce when Model.Bond_Break = absolute_stretch.
    if (!fs && util::isGreater(s, d_s0 + 1.0e-10))
      fs = true;

    // if bond is not fractured, return energy and force from nonlinear
    // potential otherwise return energy of fractured bond, and zero force
    if (!fs)
      return std::make_pair(getInfFn(r) * 0.5 * d_c * s *
                                s * r,
                            getInfFn(r) * d_c * s);
    else
      return std::make_pair(
          getInfFn(r) * 0.5 * d_c * d_s0 * d_s0 * r, 0.);
  };

  /*!
   * @brief Returns energy and force between bond due to state-based model
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param mx Weighted volume at node
   * @param thetax Dilation
   * @return value Pair of energy and force
   */
  std::pair<double, double>
  getBondEF(const double &r, const double &s, bool &fs, const double
  &mx, const double &thetax) const override {

    return this->getBondEF(r, s, fs, true);
  };

  /*!
   * @brief Returns the unit vector along which bond-force acts
   *
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return vector Unit vector
   */
  util::Point getBondForceDirection(const util::Point &dx,
                                     const util::Point &du) const override {
    return (dx + du) / (dx + du).length();
  };

  /*!
   * @brief Returns the bond strain
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return strain Bond strain \f$ S = \frac{du \cdot dx}{|dx|^2} \f$
   */
  double getS(const util::Point &dx, const util::Point &du) const override {
    return ((dx + du).length() - dx.length()) / dx.length();
  };

  /*!
   * @brief Returns critical bond strain
   *
   * @param r Reference length of bond
   * @return strain Critical strain
   */
  double getSc(const double &r) const override { return d_s0; };

  /*!
   * @brief Returns the density of the material
   * @return density Density of the material
   */
  double getDensity() const override { return d_density; };

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  double getInfFn(const double &r) const override {
    return getGlobalInfFn(r / d_horizon);
  };

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return value Moment
   */
  double getMoment(const size_t &i) const override {
    return getGlobalMoment(i);
  };

  /*!
   * @brief Returns horizon
   *
   * @return horizon Horizon
   */
  double getHorizon() const override { return d_horizon; };

  /*!
   * @brief Computes elastic and fracture material properties and returns the
   * data
   *
   * @param dim Dimension of the problem
   * @return inp::MatData Material data
   */
  inp::MatData computeMaterialProperties(const size_t &dim) const override {

    auto data = inp::MatData();
    data.d_nu = 0.25; // bond-based PMB (Trask / Emmrich)

    const double h = d_horizon;
    // Invert assuming J≡1 in the stored c (caller already divided by J_scale).
    if (dim == 2) {
      // c = 72 κ / (5 π δ³)  ⇒  κ = 5 π δ³ c / 72
      data.d_K = 5.0 * M_PI * std::pow(h, 3.0) * d_c / 72.0;
      data.d_E = data.toE(data.d_K, data.d_nu);
      data.d_G = data.toGE(data.d_E, data.d_nu);
      const double dens =
          (6.0 * data.d_G / M_PI +
           16.0 * (data.d_K - 2.0 * data.d_G) / (9.0 * M_PI * M_PI)) *
          h;
      data.d_Gc = d_s0 * d_s0 * dens;
    } else if (dim == 3) {
      data.d_K = d_c * (M_PI * std::pow(h, 4.0)) / 18.0;
      data.d_E = data.toE(data.d_K, data.d_nu);
      data.d_G = data.toGE(data.d_E, data.d_nu);
      const double dens =
          (3.0 * data.d_G +
           std::pow(3.0 / 4.0, 4) * (data.d_K - 5.0 * data.d_G / 3.0)) *
          h;
      data.d_Gc = d_s0 * d_s0 * dens;
    }
    data.d_lambda = data.toLambdaE(data.d_E, data.d_nu);
    data.d_mu = data.d_G;
    data.d_KIc = data.toKIc(data.d_Gc, data.d_nu, data.d_E);
    return data;
  };

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt, int lvl) const override {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- particle::PmbMaterial --------" << std::endl
        << std::endl;
    oss << tabS << "State active = " << 0 << std::endl;
    oss << tabS << "Horizon = " << d_horizon << std::endl;
    oss << tabS << "Influence fn address = " << influence_fn.get() << std::endl;
    oss << tabS << "Influence fn info: " << std::endl;
    oss << influence_fn->printStr(nt + 1, lvl);
    oss << tabS << "Peridynamic parameters: " << std::endl;
    oss << tabS << "  c = " << d_c << std::endl;
    oss << tabS << "  s0 = " << d_s0 << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  };

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };

  /*! @brief Prints the information about the object */
  void print() const override { print(0, 0); };

private:
  /*!
   * @brief Compute material model parameters
   *
   * @param deck MaterialDeck
   * @param dim Dimension of the domain
   */
  void computeParameters(inp::MaterialDeck &deck, const size_t &dim) {
    //
    // Need following elastic and fracture properties
    // 1. E or K
    // 2. Gc or KIc
    // Bond-based Poisson ratio is locked by dimension / plane-strain flag
    // (Silling & Askari 2005; Madenci & Oterkus 2014; Ha & Bobaru 2010).
    //
    if (util::isLess(deck.d_matData.d_E, 0.) &&
        util::isLess(deck.d_matData.d_K, 0.)) {
      std::cerr << "Error: Require either Young's modulus E or Bulk modulus K"
                   " to compute the PMB bond-based peridynamic parameters.\n";
      exit(1);
    }
    if (util::isGreater(deck.d_matData.d_E, 0.) &&
        util::isGreater(deck.d_matData.d_K, 0.)) {
      std::cout << "Warning: Both Young's modulus E and Bulk modulus K are "
                   "provided.\n";
      std::cout << "Warning: Selecting Young's modulus E for PMB calibration.\n";
    }

    if (util::isLess(deck.d_matData.d_Gc, 0.) &&
        util::isLess(deck.d_matData.d_KIc, 0.)) {
      std::cerr << "Error: Require either critical energy release rate Gc or "
                   "critical stress intensity factor KIc to compute the PMB "
                   "bond-based peridynamic parameters.\n";
      exit(1);
    } else if (util::isGreater(deck.d_matData.d_Gc, 0.) &&
               util::isGreater(deck.d_matData.d_KIc, 0.)) {
      std::cout << "Warning: Both Gc and KIc provided; selecting Gc.\n";
    }

    // Bond-based PMB locks ν=1/4 (Silling–Askari / Emmrich / Trask).
    deck.d_matData.d_nu = 0.25;

    // Prefer E; else recover E from K with locked ν.
    if (deck.d_matData.d_E < 0. && deck.d_matData.d_K > 0.)
      deck.d_matData.d_E =
          deck.d_matData.toE(deck.d_matData.d_K, deck.d_matData.d_nu);
    if (deck.d_matData.d_E > 0.)
      deck.d_matData.d_K =
          deck.d_matData.toK(deck.d_matData.d_E, deck.d_matData.d_nu);

    if (deck.d_matData.d_Gc > 0.)
      deck.d_matData.d_KIc = deck.d_matData.toKIc(
          deck.d_matData.d_Gc, deck.d_matData.d_nu, deck.d_matData.d_E);

    if (deck.d_matData.d_KIc > 0. && deck.d_matData.d_Gc < 0.)
      deck.d_matData.d_Gc = deck.d_matData.toGc(
          deck.d_matData.d_KIc, deck.d_matData.d_nu, deck.d_matData.d_E);

    deck.d_matData.d_lambda =
        deck.d_matData.toLambdaE(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_G =
        deck.d_matData.toGE(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_mu = deck.d_matData.d_G;

    // Micromodulus c and critical stretch s0.
    // Force: f = J(r/δ) * c * s. Literature c assumes J≡1 (Trask/Emmrich).
    // ConstInfluenceFn default a0=dim+1 → scale c /= a0 (or set Parameters=[1]).
    const double Gc = deck.d_matData.d_Gc;
    const double h = d_horizon;
    const double kappa = deck.d_matData.d_K;
    const double mu = deck.d_matData.d_G;

    double J_scale = 1.0;
    if (deck.d_influenceFnType == 0) {
      J_scale = deck.d_influenceFnParams.empty() ? double(dim + 1)
                                                 : deck.d_influenceFnParams[0];
    } else if (deck.d_influenceFnType == 1 || deck.d_influenceFnType == 2) {
      J_scale = getMoment(0);
      std::cout << "Warning: PMB non-constant influence: scaling c by M0="
                << J_scale
                << ". For Trask/Silling match use Influence Type 0, "
                   "Parameters=[1].\n";
    }
    if (!(J_scale > 0.)) {
      std::cerr << "Error: PMB influence scale must be > 0.\n";
      exit(1);
    }

    if (dim == 2) {
      // Trask 2019 / Emmrich: c = 72κ/(5πδ³), s0 = Madenci 2D density
      d_c = 72.0 * kappa / (5.0 * M_PI * std::pow(h, 3.0));
      const double dens =
          (6.0 * mu / M_PI +
           16.0 * (kappa - 2.0 * mu) / (9.0 * M_PI * M_PI)) *
          h;
      d_s0 = std::sqrt(Gc / dens);
    } else if (dim == 3) {
      // Trask / Silling–Askari: c = 18κ/(πδ⁴), s0 = Madenci 3D density
      d_c = 18.0 * kappa / (M_PI * std::pow(h, 4.0));
      const double dens =
          (3.0 * mu + std::pow(3.0 / 4.0, 4) * (kappa - 5.0 * mu / 3.0)) * h;
      d_s0 = std::sqrt(Gc / dens);
    } else {
      std::cerr << "Error: PMB computeParameters: unsupported dim=" << dim
                << "\n";
      exit(1);
    }
    d_c /= J_scale;
  };

private:
  /*! @brief Horizon */
  double d_horizon;

  /*! @brief Density */
  double d_density;

  /**
   * @name Material parameters
   */
  /**@{*/

  /*! @brief Parameter C */
  double d_c;

  /*! @brief Parameter \f$ \beta \f$ */
  double d_s0;

  /** @}*/
};

/*!
 * @brief A class providing methods to compute energy density and force of
 * peridynamic material
 */
class PdElastic : public Material {

public:
  /*!
   * @brief Constructor
   * @param deck Input deck which contains user-specified information
   * @param dim Dimension
   * @param horizon Horizon
   */
  PdElastic(inp::MaterialDeck &deck, const size_t &dim, const double &horizon)
      : Material("PDElasticBond"), d_horizon(horizon),
        d_density(deck.d_density), d_c(0.) {

    // set global fields
    if (dimension != dim)
      dimension = dim;

    if (is_plane_strain != deck.d_isPlaneStrain)
      is_plane_strain = deck.d_isPlaneStrain;

    // create influence function
    if (deck.d_influenceFnType == 0) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::ConstInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 1) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::LinearInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 2) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::GaussianInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else {
      std::cerr << "Error: Influence function type = "
                << deck.d_influenceFnType
                << " is invalid.\n";
      exit(1);
    }

    // check if we need to compute the material parameters
    if (deck.d_computeParamsFromElastic)
      computeParameters(deck, dim);
    else
      d_c = deck.d_bondPotentialParams[0];
  };

  /*!
   * @brief Returns true if state-based potential is active
   * @return bool True/false
   */
  bool isStateActive() const override { return false; };

  /*!
   * @brief Returns energy and force between bond due to pairwise interaction
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param break_bonds Flag to specify whether bonds are allowed to break or not
   * @return value Pair of energy and force
   */
  std::pair<double, double> getBondEF(const double &r, const double &s,
                                      bool &fs,
                                      const bool &break_bonds) const override {

    return std::make_pair(getInfFn(r) * 0.5 * d_c * s *
                            s * r,
                          getInfFn(r) * d_c * s);
  };

  /*!
   * @brief Returns energy and force between bond due to state-based model
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param mx Weighted volume at node
   * @param thetax Dilation
   * @return value Pair of energy and force
   */
  std::pair<double, double>
  getBondEF(const double &r, const double &s, bool &fs, const double
  &mx, const double &thetax) const override {

    return this->getBondEF(r, s, fs, true);
  };

  /*!
   * @brief Returns the unit vector along which bond-force acts
   *
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return vector Unit vector
   */
  util::Point getBondForceDirection(const util::Point &dx,
                                     const util::Point &du) const override {
    return (dx + du) / (dx + du).length();
  };

  /*!
   * @brief Returns the bond strain
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return strain Bond strain \f$ S = \frac{du \cdot dx}{|dx|^2} \f$
   */
  double getS(const util::Point &dx, const util::Point &du) const override {
    return ((dx + du).length() - dx.length()) / dx.length();
  };

  /*!
   * @brief Returns critical bond strain
   *
   * @param r Reference length of bond
   * @return strain Critical strain
   */
  double getSc(const double &r) const override { return std::numeric_limits<double>::max(); };

  /*!
   * @brief Returns the density of the material
   * @return density Density of the material
   */
  double getDensity() const override { return d_density; };

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  double getInfFn(const double &r) const override {
    return getGlobalInfFn(r / d_horizon);
  };

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return value Moment
   */
  double getMoment(const size_t &i) const override {
    return getGlobalMoment(i);
  };

  /*!
   * @brief Returns horizon
   *
   * @return horizon Horizon
   */
  double getHorizon() const override { return d_horizon; };

  /*!
   * @brief Computes elastic and fracture material properties and returns the
   * data
   *
   * @param dim Dimension of the problem
   * @return Data Material data
   */
  inp::MatData computeMaterialProperties(const size_t &dim) const override {

    auto data = inp::MatData();

    if (dim == 2 && !is_plane_strain)
      data.d_nu = 1.0 / 3.0;
    else
      data.d_nu = 0.25;

    const double h = d_horizon;
    if (dim == 2 && !is_plane_strain) {
      data.d_E = d_c * (M_PI * std::pow(h, 3.0)) / 9.0;
    } else if (dim == 2 && is_plane_strain) {
      data.d_E = d_c * (5.0 * M_PI * std::pow(h, 3.0)) / 48.0;
    } else if (dim == 3) {
      data.d_K = d_c * (M_PI * std::pow(h, 4.0)) / 18.0;
      data.d_E = data.toE(data.d_K, data.d_nu);
    }
    if (dim == 2)
      data.d_K = data.toK(data.d_E, data.d_nu);
    data.d_lambda = data.toLambdaE(data.d_E, data.d_nu);
    data.d_mu = data.toGE(data.d_E, data.d_nu);
    data.d_G = data.d_mu;

    return data;
  };

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt, int lvl) const override {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- particle::PdElastic --------" << std::endl
        << std::endl;
    oss << tabS << "State active = " << 0 << std::endl;
    oss << tabS << "Horizon = " << d_horizon << std::endl;
    oss << tabS << "Influence fn address = " << influence_fn.get() << std::endl;
    oss << tabS << "Influence fn info: " << std::endl;
    oss << influence_fn->printStr(nt + 1, lvl);
    oss << tabS << "Peridynamic parameters: " << std::endl;
    oss << tabS << "  c = " << d_c << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  };

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };

  /*! @brief Prints the information about the object */
  void print() const override { print(0, 0); };

private:
  /*!
   * @brief Compute material model parameters
   *
   * @param deck MaterialDeck
   * @param dim Dimension of the domain
   */
  void computeParameters(inp::MaterialDeck &deck, const size_t &dim) {
    // Elastic-only bond-based micromodulus (same δ powers as PMB).
    if (util::isLess(deck.d_matData.d_E, 0.) &&
        util::isLess(deck.d_matData.d_K, 0.)) {
      std::cerr << "Error: Require either Young's modulus E or Bulk modulus K"
                   " to compute the PdElastic parameters.\n";
      exit(1);
    }
    if (util::isGreater(deck.d_matData.d_E, 0.) &&
        util::isGreater(deck.d_matData.d_K, 0.)) {
      std::cout << "Warning: Both E and K provided; selecting E for PdElastic.\n";
    }

    // Emmrich / Trask micromodulus (J≡1). Scale if ConstInfluence a0≠1.
    deck.d_matData.d_nu = 0.25;
    if (deck.d_matData.d_E < 0. && deck.d_matData.d_K > 0.)
      deck.d_matData.d_E =
          deck.d_matData.toE(deck.d_matData.d_K, deck.d_matData.d_nu);
    if (deck.d_matData.d_E > 0.)
      deck.d_matData.d_K =
          deck.d_matData.toK(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_lambda =
        deck.d_matData.toLambdaE(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_G =
        deck.d_matData.toGE(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_mu = deck.d_matData.d_G;

    double J_scale = 1.0;
    if (deck.d_influenceFnType == 0)
      J_scale = deck.d_influenceFnParams.empty() ? double(dim + 1)
                                                 : deck.d_influenceFnParams[0];
    else if (deck.d_influenceFnType == 1 || deck.d_influenceFnType == 2)
      J_scale = std::max(getMoment(0), 1.0e-30);

    const double h = d_horizon;
    const double kappa = deck.d_matData.d_K;
    if (dim == 2)
      d_c = 72.0 * kappa / (5.0 * M_PI * std::pow(h, 3.0));
    else if (dim == 3)
      d_c = 18.0 * kappa / (M_PI * std::pow(h, 4.0));
    else {
      std::cerr << "Error: PdElastic computeParameters: unsupported dim=" << dim
                << "\n";
      exit(1);
    }
    d_c /= J_scale;
  };

private:
  /*! @brief Horizon */
  double d_horizon;

  /*! @brief Density */
  double d_density;

  /**
   * @name Material parameters
   */
  /**@{*/

  /*! @brief Parameter C */
  double d_c;

  /** @}*/
};

/*!
 * @brief A class providing methods to compute energy density and force of
 * peridynamic material
 */
class PdState : public Material {

public:
  /*!
   * @brief Constructor
   * @param deck Input deck which contains user-specified information
   * @param dim Dimension
   * @param horizon Horizon
   */
  PdState(inp::MaterialDeck &deck, const size_t &dim, const double &horizon)
      : Material("PDState"), d_horizon(horizon), d_density(deck.d_density),
        d_K(0.), d_G(0.), d_s0(0.) {

    // set global fields
    if (dimension != dim)
      dimension = dim;

    if (is_plane_strain != deck.d_isPlaneStrain)
      is_plane_strain = deck.d_isPlaneStrain;

    // create influence function
    if (deck.d_influenceFnType == 0) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::ConstInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 1) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::LinearInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else if (deck.d_influenceFnType == 2) {
      if (influence_fn == nullptr)
        influence_fn = std::make_shared<material::GaussianInfluenceFn>(
            deck.d_influenceFnParams, dim);
    }
    else {
      std::cerr << "Error: Influence function type = "
                << deck.d_influenceFnType
                << " is invalid.\n";
      exit(1);
    }

    // check if we need to compute the material parameters
    if (deck.d_computeParamsFromElastic)
      computeParameters(deck, dim);
    else {
      d_K = deck.d_bondPotentialParams[0];
      d_G = deck.d_bondPotentialParams[1];
      d_s0 = deck.d_bondPotentialParams[2];
    }
  };

  /*!
   * @brief Returns true if state-based potential is active
   * @return bool True/false
   */
  bool isStateActive() const override { return true; };

  /*!
   * @brief Returns energy and force between bond due to pairwise interaction
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param break_bonds Flag to specify whether bonds are allowed to break or not
   * @return value Pair of energy and force
   */
  std::pair<double, double> getBondEF(const double &r, const double &s,
                                      bool &fs,
                                      const bool &break_bonds) const override {

    return {0., 0.};
  };

  /*!
   * @brief Returns energy and force between bond due to state-based model
   *
   * @param r Reference (initial) bond length
   * @param s Bond strain
   * @param fs Bond fracture state
   * @param mx Weighted volume at node
   * @param thetax Dilation
   * @return value Pair of energy and force
   */
  std::pair<double, double>
  getBondEF(const double &r, const double &s, bool &fs, const double
  &mx, const double &thetax) const override {

    if (fs)
      return {0., 0.};

    double J = getInfFn(r);
    double change_length = s * r;

    double alpha = 15. * d_G / mx;
    double factor = (3. * d_K / mx) - alpha / 3.;

    return {0., J * (r * thetax * factor + change_length * alpha)};
  };

  /*!
   * @brief Returns the unit vector along which bond-force acts
   *
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return vector Unit vector
   */
  util::Point getBondForceDirection(const util::Point &dx,
                                     const util::Point &du) const override {
    return (dx + du) / (dx + du).length();
  };

  /*!
   * @brief Returns the bond strain
   * @param dx Reference bond vector
   * @param du Difference of displacement
   * @return strain Bond strain \f$ S = \frac{du \cdot dx}{|dx|^2} \f$
   */
  double getS(const util::Point &dx, const util::Point &du) const override {
    return ((dx + du).length() - dx.length()) / dx.length();
  };

  /*!
   * @brief Returns critical bond strain
   *
   * @param r Reference length of bond
   * @return strain Critical strain
   */
  double getSc(const double &r) const override { return d_s0; };

  /*!
   * @brief Returns the density of the material
   * @return density Density of the material
   */
  double getDensity() const override { return d_density; };

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  double getInfFn(const double &r) const override {
    return getGlobalInfFn(r / d_horizon);
  };

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return value Moment
   */
  double getMoment(const size_t &i) const override {
    return getGlobalMoment(i);
  };

  /*!
   * @brief Returns horizon
   *
   * @return horizon Horizon
   */
  double getHorizon() const override { return d_horizon; };

  /*!
   * @brief Computes elastic and fracture material properties and returns the
   * data
   *
   * @param dim Dimension of the problem
   * @return Data Material data
   */
  inp::MatData computeMaterialProperties(const size_t &dim) const override {

    auto data = inp::MatData();

    // we already have G and K
    data.d_G = d_G;
    data.d_K = d_K;

    // get Poisson ratio and Young's modulus
    data.d_nu = (3. * d_K - 2. * d_G) / (2. * (3. * d_K + d_G));
    data.d_E = data.toE(d_K, data.d_nu);

    // get lame parameters
    data.d_lambda = data.toLambdaE(data.d_E, data.d_nu);
    data.d_mu = d_G;

    // get Gc from s0 (inverse of computeParameters)
    double dens = 0.;
    if (dim == 2) {
      // Madenci & Oterkus OSB 2D (plane stress form used as default)
      dens = (6.0 * d_G / M_PI +
              16.0 * (d_K - 2.0 * d_G) / (9.0 * M_PI * M_PI)) *
             d_horizon;
    } else {
      dens = (3. * d_G + std::pow(3. / 4., 4) * (d_K - 5. * d_G / 3.)) *
             d_horizon;
    }
    data.d_Gc = d_s0 * d_s0 * dens;

    // KIc
    data.d_KIc = data.toKIc(
        data.d_Gc, data.d_nu, data.d_E);

    return data;
  };

  /*!
   * @brief Returns the string containing printable information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   * @return string String containing printable information about the object
   */
  std::string printStr(int nt, int lvl) const override {

    auto tabS = util::io::getTabS(nt);
    std::ostringstream oss;
    oss << tabS << "------- particle::PdState --------" << std::endl
        << std::endl;
    oss << tabS << "State active = " << 1 << std::endl;
    oss << tabS << "Horizon = " << d_horizon << std::endl;
    oss << tabS << "Influence fn address = " << influence_fn.get() << std::endl;
    oss << tabS << "Influence fn info: " << std::endl;
    oss << influence_fn->printStr(nt + 1, lvl);
    oss << tabS << "Peridynamic parameters: " << std::endl;
    oss << tabS << "  K = " << d_K << std::endl;
    oss << tabS << "  G = " << d_G << std::endl;
    oss << tabS << "  s0 = " << d_s0 << std::endl;
    oss << tabS << std::endl;

    return oss.str();
  };

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt, int lvl) const override {
    std::cout << printStr(nt, lvl);
  };

  /*! @brief Prints the information about the object */
  void print() const override { print(0, 0); };

private:
  /*!
   * @brief Compute material model parameters
   *
   * @param deck MaterialDeck
   * @param dim Dimension of the domain
   */
  void computeParameters(inp::MaterialDeck &deck, const size_t &dim) {
    //
    // Need following elastic and fracture properties
    // 1. E or K
    // 2. Poisson ratio or shear modulus
    // 3. Gc or KIc
    //
    bool found_E = false;
    bool found_K = false;
    bool found_G = false;
    bool found_nu = false;
    size_t num_props = 0;

    found_E = util::isGreater(deck.d_matData.d_E, 0.);
    if (found_E)
      num_props++;

    found_K = util::isGreater(deck.d_matData.d_K, 0.);
    if (found_K)
      num_props++;

    found_G = util::isGreater(deck.d_matData.d_G, 0.);
    if (found_G)
      num_props++;

    found_nu = util::isGreater(deck.d_matData.d_nu, 0.);
    if (found_nu)
      num_props++;

    if (num_props != 2) {
      std::ostringstream oss;
      oss << "Error: Require two different elastic properties for the "
                   "PdState material. Pairs supported are (E, K), (E, G), "
                   "(E, nu), (K, G).\n";
      oss << deck.printStr(0, 0) << "\n";
      std::cout << oss.str();
      exit(1);
    }

    if (util::isLess(deck.d_matData.d_Gc, 0.) &&
        util::isLess(deck.d_matData.d_KIc, 0.)) {
      std::cerr << "Error: Require either critical energy release rate Gc or "
                   "critical stress intensity factor KIc to compute the RNP "
                   "bond-based peridynamic parameters.\n";
      exit(1);
    } else if (util::isGreater(deck.d_matData.d_Gc, 0.) &&
               util::isGreater(deck.d_matData.d_KIc, 0.)) {
      std::cout << "Warning: Both critical energy release rate Gc and critical "
                   "stress intensity factor KIc are provided.\n";
      std::cout << "Warning: To compute the RNP bond-based peridynamic "
                   "parameters, we only require one of those.\n";
      std::cout << "Warning: Selecting critical energy release rate Gc to "
                   "compute parameters.\n";
    }

    // compute nu if not provided
    if (!found_nu) {
      if (found_E and found_G)
        deck.d_matData.d_nu =
            (0.5 * deck.d_matData.d_E / deck.d_matData.d_G) - 1.;

      if (found_E and found_K)
        deck.d_matData.d_nu =
            (3. * deck.d_matData.d_K - deck.d_matData.d_E) /
            (6. * deck.d_matData.d_K);

      if (found_G and found_K)
        deck.d_matData.d_nu =
            (3. * deck.d_matData.d_K - 2. * deck.d_matData.d_G) /
            (2. * (3. * deck.d_matData.d_K + deck.d_matData.d_G));
    }
    found_nu = true;

    // compute E if not provided
    if (!found_E) {
      if (found_K)
        deck.d_matData.d_E =
            deck.d_matData.toE(deck.d_matData.d_K, deck.d_matData.d_nu);

      if (found_G)
        deck.d_matData.d_E =
            2. * deck.d_matData.d_G * (1. + deck.d_matData.d_nu);
    }
    found_E = true;

    // compute K if not provided
    if (!found_K)
      deck.d_matData.d_K =
          deck.d_matData.toK(deck.d_matData.d_E, deck.d_matData.d_nu);

    found_K = true;


    // compute G if not provided
    if (!found_G)
      deck.d_matData.d_G =
          deck.d_matData.toGE(deck.d_matData.d_E, deck.d_matData.d_nu);

    found_G = true;

    // compute Gc (if not provided) and KIc (if not provided)
    if (deck.d_matData.d_Gc > 0.)
      deck.d_matData.d_KIc = deck.d_matData.toKIc(
          deck.d_matData.d_Gc, deck.d_matData.d_nu, deck.d_matData.d_E);

    if (deck.d_matData.d_KIc > 0. && deck.d_matData.d_Gc < 0.)
      deck.d_matData.d_Gc = deck.d_matData.toGc(
          deck.d_matData.d_KIc, deck.d_matData.d_nu, deck.d_matData.d_E);

    // compute lame parameter
    deck.d_matData.d_lambda =
        deck.d_matData.toLambdaE(deck.d_matData.d_E, deck.d_matData.d_nu);
    deck.d_matData.d_mu = deck.d_matData.d_G;

    // compute peridynamic parameters
    d_K = deck.d_matData.d_K;
    d_G = deck.d_matData.d_G;

    // Critical stretch: OSB energy release (Madenci & Oterkus).
    // Density factor scales as δ¹ in both 2D and 3D, but coefficients differ.
    double dens = 0.;
    if (dim == 2) {
      dens = (6.0 * d_G / M_PI +
              16.0 * (d_K - 2.0 * d_G) / (9.0 * M_PI * M_PI)) *
             d_horizon;
    } else if (dim == 3) {
      dens = (3. * d_G + std::pow(3. / 4., 4) * (d_K - 5. * d_G / 3.)) *
             d_horizon;
    } else {
      std::cerr << "Error: PdState computeParameters: unsupported dim=" << dim
                << "\n";
      exit(1);
    }
    d_s0 = std::sqrt(deck.d_matData.d_Gc / dens);
  };

private:
  /*! @brief Horizon */
  double d_horizon;

  /*! @brief Density */
  double d_density;

  /**
   * @name Material parameters
   */
  /**@{*/

  /*! @brief Bulk modulus */
  double d_K;

  /*! @brief Shear modulus */
  double d_G;

  /*! @brief Critical stretch */
  double d_s0;

  /** @}*/
};

} // namespace material

#endif // MATERIAL_PD_MATERIAL_H
