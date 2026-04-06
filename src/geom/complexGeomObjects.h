/*
* -------------------------------------------
* Copyright (c) 2021 - 2024 Prashant K. Jha
* -------------------------------------------
* PeriDEM https://github.com/prashjha/PeriDEM
*
* Distributed under the Boost Software License, Version 1.0. (See accompanying
* file LICENSE)
*/

#ifndef GEOM_COMPLEXGEOMOBJECTS_H
#define GEOM_COMPLEXGEOMOBJECTS_H

#include <iostream>
#include <utility>
#include <memory>
#include <map>

#include "geomObjects.h"
#include "geomObjectsUtil.h"

namespace geom {

/*!
  * @brief Defines annulus rectangle
  */
  class AnnulusGeomObject : public GeomObject {
  public:
      /*! @brief Outer object */
    GeomObject *d_outObj_p;

    /*! @brief Inner object */
    GeomObject *d_inObj_p;

    /*! @brief Dimension objects live in */
    size_t d_dim;

  public:
    /*!
     * @brief Constructor
     */
    AnnulusGeomObject()
      : GeomObject("annulus_object", ""),
        d_outObj_p(nullptr),
        d_inObj_p(nullptr),
        d_dim(0) {
    };

    /*!
     * @brief Constructor
     *
     * @param in Inner object
     * @param out Outer object
     * @param description Description of object (e.g., further classification or any tag)
     */
    AnnulusGeomObject(GeomObject *in, GeomObject *out, std::string description = "")
      : GeomObject("annulus_object", description),
        d_outObj_p(out),
        d_inObj_p(in),
        d_dim(0) {
      d_dim = getGeomTypeToDim(d_outObj_p->d_name); // assume all types have same dim
    };

    /*!
     * @brief Copy constructor
     * @param other Object to copy from
     * @note This performs a deep copy of the inner and outer objects
     */
    AnnulusGeomObject(const AnnulusGeomObject &other)
      : GeomObject(other.d_name, other.d_description),
        d_outObj_p(nullptr),
        d_inObj_p(nullptr),
        d_dim(other.d_dim) {
      // Copy tags from base class
      d_tags = other.d_tags;

      // Deep copy inner and outer objects using utility function
      d_inObj_p = createGeomDeepCopy(other.d_inObj_p);
      d_outObj_p = createGeomDeepCopy(other.d_outObj_p);
    }

    /*!
     * @brief Destructor
     */
    ~AnnulusGeomObject() {
      delete d_inObj_p;
      delete d_outObj_p;
    }

    /*!
     * @brief Assignment operator
     * @param other Object to copy from
     * @return Reference to this object
     */
    AnnulusGeomObject& operator=(const AnnulusGeomObject& other) {
      if (this != &other) {
        // Clean up existing objects
        delete d_inObj_p;
        delete d_outObj_p;
        d_inObj_p = nullptr;
        d_outObj_p = nullptr;

        // Copy base class members
        d_tags = other.d_tags;
        d_dim = other.d_dim;

        // Deep copy inner and outer objects using utility function
        d_inObj_p = createGeomDeepCopy(other.d_inObj_p);
        d_outObj_p = createGeomDeepCopy(other.d_outObj_p);
      }
      return *this;
    }

    /*!
     * @copydoc GeomObject::transform
     * If @p rotationPoint is null and both inner and outer exist, pivot is `center()` (composite
     * centroid of the annular solid). If only one child is present, that child’s `center()` is used.
     */
    void transform(const util::Point &translation, const double &scale, 
                  const double &angle, const util::Point &axis,
                  const util::Point *rotationPoint) override {
      const util::Point *pivot = rotationPoint;
      util::Point pivotStore;
      if (pivot == nullptr) {
        if (d_inObj_p && d_outObj_p) {
          pivotStore = center();
          pivot = &pivotStore;
        } else if (d_inObj_p) {
          pivotStore = d_inObj_p->center();
          pivot = &pivotStore;
        } else if (d_outObj_p) {
          pivotStore = d_outObj_p->center();
          pivot = &pivotStore;
        }
      }
      if (d_inObj_p) d_inObj_p->transform(translation, scale, angle, axis, pivot);
      if (d_outObj_p) d_outObj_p->transform(translation, scale, angle, axis, pivot);
    }

    /*!
     * @copydoc GeomObject::volume() const
     */
    double volume() const override;

    /*!
     * @copydoc GeomObject::center() const
     */
    util::Point center() const override;

    /*!
     * @copydoc GeomObject::box() const
     */
    std::pair<util::Point, util::Point> box() const override;

    /*!
     * @copydoc GeomObject::box(const double &tol) const
     */
    std::pair<util::Point, util::Point>
    box(const double &tol) const override;

    /*!
     * @copydoc GeomObject::inscribedRadius() const
     */
    double inscribedRadius() const override;

    /*!
     * @copydoc GeomObject::boundingRadius() const
     */
    double boundingRadius() const override;

    /**
     * @name Interaction with point
     */
    /** @{*/

    /*!
     * @copydoc GeomObject::isInside(const util::Point &x) const
     */
    bool isInside(const util::Point &x) const override;

    /*!
     * @copydoc GeomObject::isOutside(const util::Point &x) const
     */
    bool isOutside(const util::Point &x) const override;

    /*!
     * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
     */
    bool isNear(const util::Point &x, const double &tol) const override;

    /*!
     * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                        const bool &within) cons
     */
    bool isNearBoundary(const util::Point &x, const double &tol,
                        const bool &within) const override;

    /*!
     * @copydoc GeomObject::doesIntersect(const util::Point &x) const
     */
    bool doesIntersect(const util::Point &x) const override;

    /** @}*/

    /**
     * @name Interaction with box
     */
    /** @{*/

    /*!
     * @copydoc GeomObject::isInside(
            const std::pair<util::Point, util::Point> &box) const
     */
    bool
    isInside(
      const std::pair<util::Point, util::Point> &box) const override;

    /*!
     * @copydoc GeomObject::isOutside(
            const std::pair<util::Point, util::Point> &box) const
     */
    bool
    isOutside(
      const std::pair<util::Point, util::Point> &box) const override;

    /*!
     * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                const double &tol) const
     */
    bool isNear(const std::pair<util::Point, util::Point> &box,
                const double &tol) const override;

    /*!
     * @copydoc GeomObject::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const
     */
    bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

    /** @}*/

    /*!
     * @copydoc GeomObject::printStr(int nt, int lvl) const
     */
    std::string printStr(int nt, int lvl) const override;

    /*!
     * @copydoc GeomObject::print(int nt, int lvl) const
     */
    void print(int nt, int lvl) const override {
      std::cout << printStr(nt, lvl);
    };

    /*!
     * @copydoc GeomObject::print() const
     */
    void print() const override { print(0, 0); };
  };

  /*!
  * @brief Defines complex geometrical object
  */
  class ComplexGeomObject : public GeomObject {
  
  public:
    /*! @brief Object */
    std::vector<std::shared_ptr<GeomObject> > d_obj;

    /*!
     * @brief Object flag
     *
     * Ordering of objects is important. To describe a rectangle with circular
     * hole, we will have d_obj = {rectangle, circle} and have flag = {plus,
     * minus}. This means final object is rectangle - circle
     *
     */
    std::vector<std::string> d_objFlag;

    /*!
     * @brief Object integer flags. Here, +1 means object is filling and -1 means object is void
     */
    std::vector<int> d_objFlagInt;

    /*! @brief Dimension objects live in */
    size_t d_dim;

  public:
    /*!
     * @brief Constructor
     */
    ComplexGeomObject() : GeomObject("complex", ""), d_dim(0) {
    };

    /*!
     * @brief Constructor
     *
     * @param obj Vector of geometrical objects
     * @param obj_flag Specifies which objects are filling and which are void
     * @param description Description of object (e.g., further classification or any tag)
     */
    ComplexGeomObject(
      std::vector<std::shared_ptr<GeomObject> > &obj,
      std::vector<std::string> obj_flag, std::string description = "")
      : GeomObject("complex", description),
        d_obj(obj),
        d_objFlag(obj_flag),
        d_dim(0) {

      d_dim = getGeomTypeToDim(d_obj[0]->d_name); // assume all types have same dim

      for (const auto &s: d_objFlag)
        if (s == "plus")
          d_objFlagInt.push_back(1);
        else if (s == "minus")
          d_objFlagInt.push_back(-1);
        else {
          std::cerr
              << "Error: Check object flag " + s +
              " passed to create ComplexGeomObject\n";
          exit(1);
        }
    };

    /*!
     * @brief Copy constructor
     * @param other Object to copy from
     */
    ComplexGeomObject(const ComplexGeomObject &other)
      : GeomObject(other.d_name, other.d_description),
        d_objFlag(other.d_objFlag),
        d_objFlagInt(other.d_objFlagInt),
        d_dim(other.d_dim) {
      d_tags = other.d_tags;

      // Deep copy each geometric object
      d_obj.reserve(other.d_obj.size());
      for (const auto& obj : other.d_obj) {
        if (obj) {
          // Create a raw pointer copy first
          GeomObject* raw_copy = createGeomDeepCopy(obj.get());
          // Convert to shared_ptr and store
          d_obj.push_back(std::shared_ptr<GeomObject>(raw_copy));
        } else {
          d_obj.push_back(nullptr);
        }
      }
    }

    /*!
     * @brief Assignment operator
     * @param other Object to copy from
     * @return Reference to this object
     */
    ComplexGeomObject& operator=(const ComplexGeomObject& other) {
      if (this != &other) {
        // Copy base class members
        d_tags = other.d_tags;
        
        // Copy flags and dimension
        d_objFlag = other.d_objFlag;
        d_objFlagInt = other.d_objFlagInt;
        d_dim = other.d_dim;

        // Deep copy each geometric object
        d_obj.clear();
        d_obj.reserve(other.d_obj.size());
        for (const auto& obj : other.d_obj) {
          if (obj) {
            // Create a raw pointer copy first
            GeomObject* raw_copy = createGeomDeepCopy(obj.get());
            // Convert to shared_ptr and store
            d_obj.push_back(std::shared_ptr<GeomObject>(raw_copy));
          } else {
            d_obj.push_back(nullptr);
          }
        }
      }
      return *this;
    }

    /*!
     * If @p rotationPoint is null, pivot is `center()` (signed-volume centroid of the composite).
     */
    void transform(const util::Point &translation, const double &scale, 
                  const double &angle, const util::Point &axis,
                  const util::Point *rotationPoint) override {
      const util::Point *pivot = rotationPoint;
      util::Point pivotStore;
      if (pivot == nullptr) {
        pivotStore = center();
        pivot = &pivotStore;
      }
      for (auto &obj : d_obj) {
        if (obj) obj->transform(translation, scale, angle, axis, pivot);
      }
    }

    /*!
     * @copydoc GeomObject::volume() const
     */
    double volume() const override;

    /*!
     * @copydoc GeomObject::center() const
     */
    util::Point center() const override;

    /*!
     * @copydoc GeomObject::box() const
     */
    std::pair<util::Point, util::Point> box() const override;

    /*!
     * @copydoc GeomObject::box(const double &tol) const
     */
    std::pair<util::Point, util::Point>
    box(const double &tol) const override;

    /*!
     * @copydoc GeomObject::inscribedRadius() const
     */
    double inscribedRadius() const override;

    /*!
     * @copydoc GeomObject::boundingRadius() const
     */
    double boundingRadius() const override;

    /**
     * @name Interaction with point
     */
    /** @{*/

    /*!
     * @copydoc GeomObject::isInside(const util::Point &x) const
     */
    bool isInside(const util::Point &x) const override;

    /*!
     * @copydoc GeomObject::isOutside(const util::Point &x) const
     */
    bool isOutside(const util::Point &x) const override;

    /*!
     * @copydoc GeomObject::isNear(const util::Point &x, const double &tol) const
     */
    bool isNear(const util::Point &x, const double &tol) const override;

    /*!
     * @copydoc GeomObject::isNearBoundary(const util::Point &x, const double &tol,
                        const bool &within) cons
     */
    bool isNearBoundary(const util::Point &x, const double &tol,
                        const bool &within) const override;

    /*!
     * @copydoc GeomObject::doesIntersect(const util::Point &x) const
     */
    bool doesIntersect(const util::Point &x) const override;

    /** @}*/

    /**
     * @name Interaction with box
     */
    /** @{*/

    /*!
     * @copydoc GeomObject::isInside(
            const std::pair<util::Point, util::Point> &box) const
     */
    bool
    isInside(
      const std::pair<util::Point, util::Point> &box) const override;

    /*!
     * @copydoc GeomObject::isOutside(
            const std::pair<util::Point, util::Point> &box) const
     */
    bool
    isOutside(
      const std::pair<util::Point, util::Point> &box) const override;

    /*!
     * @copydoc GeomObject::isNear(const std::pair<util::Point, util::Point> &box,
                const double &tol) const
     */
    bool isNear(const std::pair<util::Point, util::Point> &box,
                const double &tol) const override;

    /*!
     * @copydoc GeomObject::doesIntersect(
            const std::pair<util::Point, util::Point> &box) const
     */
    bool doesIntersect(
      const std::pair<util::Point, util::Point> &box) const override;

    /** @}*/

    /*!
     * @copydoc GeomObject::printStr(int nt, int lvl) const
     */
    std::string printStr(int nt, int lvl) const override;

    /*!
     * @copydoc GeomObject::print(int nt, int lvl) const
     */
    void print(int nt, int lvl) const override {
      std::cout << printStr(nt, lvl);
    };

    /*!
     * @copydoc GeomObject::print() const
     */
    void print() const override { print(0, 0); };
  };

  } // namespace geom

#endif // GEOM_COMPLEXGEOMOBJECTS_H