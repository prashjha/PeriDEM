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

  /*! @brief Rigid translation \f$\mathbf t\f$ after rotate+scale about the pivot (matches `GeomObject::transform`). */
  util::Point d_translation;

  /*! @brief Unit axis of rotation */
  util::Point d_axis;

  /*! @brief Angle of rotation (radians) */
  double d_theta;

  /*! @brief Uniform scale factor */
  double d_scale;

  /*! @brief Pivot \f$\mathbf p\f$ for rotation and scale (same role as `rotationPoint` in `GeomObject::transform`). Default origin for legacy meshes centered at zero. */
  util::Point d_rotationPoint;

  /*!
   * @brief Constructor
   *
   * Default constructor creates identity transformation, i.e. transform(x) = x.
   */
  ParticleTransform()
      : d_translation(util::Point()), d_axis(util::Point()),
        d_theta(0.), d_scale(1.), d_rotationPoint(util::Point()){};

  /*!
   * @brief Constructor with pivot at origin (reference mesh centered at origin).
   *
   * @param translate Translation vector \f$\mathbf t\f$
   * @param axis Axis of rotation
   * @param theta Angle of rotation
   * @param scale Uniform scale
   */
  ParticleTransform(util::Point translate, util::Point axis, double theta,
                    double scale = 1.)
      : d_translation(translate), d_axis(axis / axis.length()), d_theta(theta),
        d_scale(scale), d_rotationPoint(util::Point()){};

  /*!
   * @brief Constructor with explicit rotation pivot (consistent with `GeomObject::transform`).
   */
  ParticleTransform(util::Point translate, util::Point axis, double theta, double scale,
                    util::Point rotation_point)
      : d_translation(translate), d_axis(axis / axis.length()), d_theta(theta),
        d_scale(scale), d_rotationPoint(rotation_point){};

  /*!
   * @brief Copy constructor
   * @param t Another ParticleTransform object
   */
  ParticleTransform(const ParticleTransform &t)
      : d_translation(t.d_translation), d_axis(t.d_axis),
        d_theta(t.d_theta), d_scale(t.d_scale), d_rotationPoint(t.d_rotationPoint){};

  /*!
   * @brief Maps a reference-configuration point \f$\mathbf v\f$ to world coordinates using the
   * same rule as `geom::mapSimilarity` / `GeomObject::transform`:
   * \f$\mathbf y = \mathbf p + s\,\mathbf R(\mathbf v-\mathbf p) + \mathbf t\f$.
   *
   * When \f$\mathbf p = \mathbf 0\f$, this reduces to
   * \f$\mathbf y = s\,\mathbf R(\mathbf v) + \mathbf t\f$ (legacy reference-mesh-at-origin case).
   *
   * @param v Node position in reference particle frame
   * @return Transformed position
   */
  util::Point apply(const util::Point &v) const {
    return d_rotationPoint +
           d_scale * util::rotate(v - d_rotationPoint, d_theta, d_axis) +
           d_translation;
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
    oss << tabS << "Rotation pivot = " << d_rotationPoint.printStr() << std::endl;
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
