////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Prashant K. Jha
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ////////////////////////////////////////////////////////////////////////////////

#include "complexGeomObjects.h"
#include "geomUtilFunctions.h"
#include "util/function.h"
#include "util/vecMethods.h"
#include "util/io.h"
#include <vector>
#include <set>

namespace {
  std::string printErrMsg(const std::string &geom_type,
                          const std::vector<double> &params,
                          const std::vector<size_t> &num_params_needed) {

    std::ostringstream oss;

    oss <<  "Error: Number of parameters needed to create geometry = "
        << geom_type << " are "
        << util::io::printStr(num_params_needed, 0)
        << ". But the number of parameters provided are "
        << params.size()
        << " and the parameters are "
        << util::io::printStr(params, 0)
        << ". Exiting.\n";

    return oss.str();
  }
};

//
// AnnulusGeomObject
//
namespace geom {
    double AnnulusGeomObject::volume() const {
      return d_outObj_p->volume() - d_inObj_p->volume();
    }

    util::Point AnnulusGeomObject::center() const {

      // we use the formula for centroid of composite objects
      // x = sum_i sign(i) V_i x_i / sum_i sign(i) V_i
      // where sign(i) = +1 if object is filling
      // sign(i) = -1 if object is empty
      auto vol = volume();
      if (util::isGreater(vol, 0.))
        return (1./vol) * (d_outObj_p->volume() * d_outObj_p->center()
                - d_inObj_p->volume() * d_inObj_p->center());
      else
        return d_outObj_p->center();
    }

    std::pair<util::Point, util::Point> AnnulusGeomObject::box
            () const {
      return d_outObj_p->box();
    }

    std::pair<util::Point, util::Point> AnnulusGeomObject::box
            (const double &tol) const {
      return d_outObj_p->box(tol);
    }

    double AnnulusGeomObject::inscribedRadius() const {

      return d_outObj_p->inscribedRadius();
    }

    double AnnulusGeomObject::boundingRadius() const {

      return d_outObj_p->boundingRadius();
    }

    bool
    AnnulusGeomObject::isInside(const util::Point &x) const {

      // should be outside inner object and inside outer object
      return !d_inObj_p->isInside(x) && d_outObj_p->isInside(x);
    }

    bool
    AnnulusGeomObject::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool AnnulusGeomObject::isNear(const util::Point &x,
                                                   const double &tol) const {

      return d_outObj_p->isNear(x, tol) || d_inObj_p->isNear(x, tol);
    }

    bool AnnulusGeomObject::isNearBoundary(const util::Point &x,
                                                           const double &tol,
                                                           const bool
                                                           &within) const {

      return d_outObj_p->isNearBoundary(x, tol, within) ||
             d_inObj_p->isNearBoundary(x, tol, within);
    }

    bool AnnulusGeomObject::doesIntersect(
            const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool AnnulusGeomObject::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(d_dim, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool AnnulusGeomObject::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(d_dim, box))
        intersect = this->isInside(p);

      return !intersect;
    }

    bool AnnulusGeomObject::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      return d_outObj_p->isNear(box, tol) || d_inObj_p->isNear(box, tol);
    }

    bool AnnulusGeomObject::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(d_dim, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string
    AnnulusGeomObject::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- AnnulusGeomObject --------" << std::endl
          << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Center = " << center().printStr() << std::endl;
      oss << tabS << "Inner object info:" << std::endl;
      oss << d_inObj_p->printStr(nt + 1, lvl);
      oss << tabS << "Outer object info:" << std::endl;
      oss << d_outObj_p->printStr(nt + 1, lvl);

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }

}// AnnulusGeomObject

//
// ComplexGeomObject
//
namespace geom {
    double ComplexGeomObject::volume() const {

      double volume = 0.;
      for (size_t i = 0; i < d_objFlag.size(); i++)
        volume += d_obj[i]->volume() * d_objFlagInt[i];

      return volume;
    }

    util::Point ComplexGeomObject::center() const {

      // use formula for centroid of composite objects
      auto vol = volume();
      if (util::isGreater(vol, 0.)) {
        auto center = util::Point();
        for (size_t i = 0; i < d_objFlag.size(); i++)
          center += d_obj[i]->volume() * d_objFlagInt[i] * d_obj[i]->center();
        return (1./vol) * center;
      }
      else {

        // find biggest object that has positive d_objFlagInt
        // (that is it is a filling and not void object)
        std::vector<double> vol_vec(d_obj.size());
        for (size_t i = 0; i < d_obj.size(); i++)
          vol_vec[i] = d_obj[i]->volume() * d_objFlagInt[i];

        auto max_vol_obj = util::methods::maxIndex(vol_vec);
        return d_obj[max_vol_obj]->center();
      }
    }

    std::pair<util::Point, util::Point> ComplexGeomObject::box
            () const {

      return box(0.);
    }

    std::pair<util::Point, util::Point> ComplexGeomObject::box
            (const double &tol) const {

      auto p1 = d_obj[0]->box(tol).first;
      auto p2 = d_obj[0]->box(tol).second;

      for (size_t i = 1; i < d_objFlag.size(); i++) {

        auto q1 = d_obj[i]->box(tol).first;
        auto q2 = d_obj[i]->box(tol).second;

        for (size_t i = 0; i < 3; i++) {
          if (q1[i] < p1[i])
            p1[i] = q1[i];
          if (q2[i] > p2[i])
            p2[i] = q2[i];
        }
      }

      return std::make_pair(p1, p2);
    }

    double ComplexGeomObject::inscribedRadius() const {

      auto box = this->box();
      return 0.5 * (box.first - box.second).length();
    }

    double ComplexGeomObject::boundingRadius() const {

      auto box = this->box();
      return 0.5 * (box.first - box.second).length();
    }

    bool
    ComplexGeomObject::isInside(const util::Point &x) const {

      // point inside means x should be inside in the object with plus flag and
      // outside in the object with minus flag
      bool point_inside = d_obj[0]->isInside(x);
      for (size_t i = 1; i < d_objFlag.size(); i++) {

        const auto &obj_i = d_obj[i];
        if (d_objFlagInt[i] < 0)
          point_inside = point_inside and !obj_i->isInside(x);
        else
          point_inside = point_inside or obj_i->isInside(x);
      }

      return point_inside;
    }

    bool
    ComplexGeomObject::isOutside(const util::Point &x) const {
      return !isInside(x);
    }

    bool ComplexGeomObject::isNear(const util::Point &x,
                                                   const double &tol) const {

      bool is_near = d_obj[0]->isNear(x, tol);
      for (size_t i = 1; i < d_objFlag.size(); i++) {

        const auto &obj_i = d_obj[i];
        is_near = is_near or obj_i->isNear(x, tol);
      }

      return is_near;
    }

    bool ComplexGeomObject::isNearBoundary(const util::Point &x,
                                                           const double &tol,
                                                           const bool
                                                           &within) const {

      bool is_near = d_obj[0]->isNearBoundary(x, tol, within);
      for (size_t i = 1; i < d_objFlag.size(); i++) {

        const auto &obj_i = d_obj[i];
        is_near = is_near or obj_i->isNearBoundary(x, tol, within);
      }

      return is_near;
    }

    bool ComplexGeomObject::doesIntersect(
            const util::Point &x) const {

      return isNearBoundary(x, 1.0E-8, false);
    }

    bool ComplexGeomObject::isInside(
            const std::pair<util::Point, util::Point> &box) const {

      for (auto p: geom::getCornerPoints(d_dim, box))
        if (!this->isInside(p))
          return false;

      return true;
    }

    bool ComplexGeomObject::isOutside(
            const std::pair<util::Point, util::Point> &box) const {

      bool intersect = false;
      for (auto p: geom::getCornerPoints(d_dim, box))
        intersect = this->isInside(p);

      return !intersect;
    }

    bool ComplexGeomObject::isNear(
            const std::pair<util::Point, util::Point> &box,
            const double &tol) const {

      bool is_near = d_obj[0]->isNear(box, tol);
      for (size_t i = 1; i < d_objFlag.size(); i++) {

        const auto &obj_i = d_obj[i];
        is_near = is_near or obj_i->isNear(box, tol);
      }

      return is_near;
    }

    bool ComplexGeomObject::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const {

      // need to check all four corner points
      for (auto p: geom::getCornerPoints(d_dim, box))
        if (this->isInside(p))
          return true;

      return false;
    }

    std::string
    ComplexGeomObject::printStr(int nt, int lvl) const {

      auto tabS = util::io::getTabS(nt);

      std::ostringstream oss;

      oss << tabS << "------- ComplexGeomObject --------" << std::endl
          << std::endl;
      oss << tabS << "Name = " << d_name << std::endl;
      oss << tabS << "Center = " << center().printStr() << std::endl;
      oss << tabS << "Object info:" << std::endl;
      auto ocount = 0;
      for (const auto &p: d_obj) {
        oss << tabS << "Object id: " << ocount << std::endl;
        oss << tabS << "Object flag: " << d_objFlag[ocount] << std::endl;
        oss << tabS << "Object int flag: " << d_objFlagInt[ocount] << std::endl;
        oss << p->printStr(nt + 1, lvl);
        ocount++;
      }

      if (lvl > 0)
        oss << tabS << "Bounding box: "
            << util::io::printBoxStr(box(0.), nt + 1);

      if (lvl == 0)
        oss << std::endl;

      return oss.str();
    }

}// ComplexGeomObject