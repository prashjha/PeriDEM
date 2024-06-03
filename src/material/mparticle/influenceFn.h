/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef MATERIAL_PD_INFLUENCEFN_H
#define MATERIAL_PD_INFLUENCEFN_H

#include "util/io.h"
#include <cstring>
#include <iostream>
#include <vector>

namespace material {

/*! @brief A base class for computing influence function */
class BaseInfluenceFn {

public:
  /*! @brief Constructor */
  BaseInfluenceFn() = default;

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
   * i^{th} \f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return moment Moment
   */
  virtual double getMoment(const size_t &i) const = 0;

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
    oss << tabS << "------- BaseInfluenceFn --------" << std::endl << std::endl;
    oss << tabS << "Provides abstraction for different influence function "
                   "types" << std::endl;
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
};

/*! @brief A class to implement constant influence function */
class ConstInfluenceFn : public BaseInfluenceFn {

public:
  /*!
   * @brief Constructor
   * @param params List of parameters
   * @param dim Dimension
   */
  ConstInfluenceFn(const std::vector<double> &params, const size_t &dim);

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  double getInfFn(const double &r) const override;

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return moment Moment
   */
  double getMoment(const size_t &i) const override;

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
    oss << tabS << "------- ConstInfluenceFn --------" << std::endl << std::endl;
    oss << tabS << "Constant function with constant = " << d_a0 << std::endl;
    oss << tabS << "First moment = " << getMoment(1)
              << ", second moment = " << getMoment(2)
              << ", third moment = " << getMoment(3) << std::endl;
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
  /*! @brief Constant such that J(r) = Constant */
  double d_a0;
};

/*! @brief A class to implement linear influence function
 *
 * \f$ J(r) = a0 + a1 r \f$
 */
class LinearInfluenceFn : public BaseInfluenceFn {

public:
  /*!
   * @brief Constructor
   * @param params List of parameters
   * @param dim Dimension
   */
  LinearInfluenceFn(const std::vector<double> &params, const size_t &dim);

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  double getInfFn(const double &r) const override;

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return moment Moment
   */
  double getMoment(const size_t &i) const override;

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
    oss << tabS << "------- LinearInfluenceFn --------" << std::endl << std::endl;
    oss << tabS << "Linear function a0 + a1*r with constants: a0 = "
                << d_a0 << ", a1 = " << d_a1 << std::endl;
    oss << tabS << "First moment = " << getMoment(1)
        << ", second moment = " << getMoment(2)
        << ", third moment = " << getMoment(3) << std::endl;
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
  /*! @brief Constants such that J(r) = d_a0 + d_a1 * r */
  double d_a0;

  /*! @brief Constants such that J(r) = d_a0 + d_a1 * r */
  double d_a1;
};

/*! @brief A class to implement Gaussian influence function
 *
 * \f$ J(r) = \alpha \exp(-r^2/\beta) \f$
 */
class GaussianInfluenceFn : public BaseInfluenceFn {

public:
  /*!
   * @brief Constructor
   * @param params List of parameters
   * @param dim Dimension
   */
  GaussianInfluenceFn(const std::vector<double> &params, const size_t &dim);

  /*!
   * @brief Returns the value of influence function
   *
   * @param r Reference (initial) bond length
   * @return value Influence function at r
   */
  double getInfFn(const double &r) const override;

  /*!
   * @brief Returns the moment of influence function
   *
   * If \f$ J(r) \f$ is the influence function for \f$ r\in [0,1)\f$ then \f$
   * i^{th}\f$ moment is given by \f[ M_i = \int_0^1 J(r) r^i dr. \f]
   *
   * @param i ith moment
   * @return moment Moment
   */
  double getMoment(const size_t &i) const override;

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
    oss << tabS << "------- GaussianInfluenceFn --------" << std::endl << std::endl;
    oss << tabS << "Gaussian function a0 * exp(-r*r / a1) with constants: a0 = "
                << d_alpha << ", a1 = " << d_beta << std::endl;
    oss << tabS << "First moment = " << getMoment(1)
        << ", second moment = " << getMoment(2)
        << ", third moment = " << getMoment(3) << std::endl;
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
  /*! @brief Constants */
  double d_alpha;

  /*! @brief Constants */
  double d_beta;
};

} // namespace material

#endif // MATERIAL_PD_INFLUENCEFN_H
