/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#pragma once

#include "util/point.h"  // definition of Point
#include "util/transformationFunctions.h"
#include "util/io.h"

#include <cstring> // string and size_t type
#include <vector>

namespace geom {

/*! @brief A struct that stores transformation parameters and provides method
 * to transform the particle. Basically, given a reference particle, this
 * provides method to translate, rotate, and scale the reference particle.
 */
struct ParticleTransform {

  /*! @brief Translational vector */
  util::Point d_translation;

  /*! @brief Axis of rotation */
  util::Point d_axis;

  /*! @brief Angle of rotation */
  double d_theta;

  /*! @brief Volumetric scaling factor */
  double d_scale;

  /*!
   * @brief Constructor
   *
   * Default constructor creates identity transformation, i.e., transform(x) = x.
   */
  ParticleTransform()
      : d_translation(util::Point()), d_axis(util::Point()),
        d_theta(0.), d_scale(1.){};

  /*!
   * @brief Constructor
   *
   * @param translate Translation vector
   * @param axis Axis of rotation
   * @param theta Angle of rotation
   * @param scale Volumetric scaling
   */
  ParticleTransform(util::Point translate, util::Point axis, double theta,
                    double scale = 1.)
      : d_translation(translate), d_axis(axis / axis.length()), d_theta(theta),
        d_scale(scale){};

  /*!
   * @brief Copy constructor
   * @param t Another ParticleTransform object
   */
  ParticleTransform(const ParticleTransform &t)
      : d_translation(t.d_translation), d_axis(t.d_axis),
        d_theta(t.d_theta), d_scale(t.d_scale){};

  /*!
   * @brief Returns the transformed vector. We assume that the passed vector
   * passes through origin.
   *
   * Let B(0, R) is the ball centered at origin. Let v is a point in ball B
   * (0,R).
   *
   * Suppose we want to transform v so that it is now in ball B(x, r), where
   * x is the point in space, r is the radius of new ball.
   *
   * Further, suppose we also want to rotate the v by angle theta about axis a
   * and scale v by amount s.
   *
   * To do this, we assume that this class was constructed with x, a, theta,
   * and s, i.e. ParticleTransform(x, a, theta, s).
   *
   * Following transformation is applied on vector v
   *
   * 1. Rotation by angle theta about axis a
   *
   * 2. Next, scale the vector
   *
   * 3. Finally, translate the vector
   *
   * @param v Vector v in ball
   * @return vector Transformed vector
   */
  util::Point apply(const util::Point &v) const {

    return d_translation +
           d_scale * util::rotate(v, d_theta, d_axis);

    // return d_translation + d_scale * util::Point(v.d_x, -v.d_y, 0.);
    // return d_translation + v;
  };

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
    oss << tabS << "------- ParticleTransform --------" << std::endl << std::endl;
    oss << tabS << "Scale = " << d_scale << std::endl;
    oss << tabS << "Angle = " << d_theta << std::endl;
    oss << tabS << "Translation = " << d_translation.printStr() << std::endl;
    oss << tabS << "Axis = " << d_axis.printStr() << std::endl;
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

} // namespace geom
