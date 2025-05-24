/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_PARTICLEDECK_H
#define INP_PARTICLEDECK_H

#include "meshDeck.h"
#include "materialDeck.h"
#include "contactDeck.h"
#include "pNeighborDeck.h"
#include "pGenDeck.h"
#include "geom/geomIncludes.h"
#include <memory>

namespace inp {

/**
 * \ingroup Input
 */
/**@{*/

/*! @brief Structure to read and store particle related input data */
struct ParticleDeck {

  /*! @brief Specify if this is single or multi particle simulation
   * Expected value is either 'Single_Particle' or 'Multi_Particle'.
   *
   * This flag is used to populate input deck data. For the case when
   * we consider single particle and its deformation, one do not have to specify data such as zones and contact.
 * */
  std::string d_particleSimType;

  /*! @brief Particle geometry data */
  std::vector<geom::GeomData> d_pGeomVec;

  /*! @brief Particle mesh data */
  std::vector<inp::MeshDeck> d_pMeshVec;

  /*! @brief Particle material data */
  std::vector<inp::MaterialDeck> d_pMaterialVec;

  /*! @brief Particle contact data */
  inp::ContactDeck d_contactDeck;

  /*! @brief Neighbor search data */
  inp::PNeighborDeck d_pNeighDeck;

  /*! @brief Particle generation data */
  inp::PGenDeck d_pGenDeck;

  /*!
   * @brief Constructor
   */
  ParticleDeck(const json &j = json({}), std::string particleSimType = "Multi_Particle") {
    d_particleSimType = particleSimType;
    readFromJson(j);
  };

  void readFromJson(const json &j) {

    if (j.find("Particle") != j.end())
      readParticleGeomFromJson(j.at("Particle"));

    if (j.find("Mesh") != j.end())
      readParticleMeshFromJson(j.at("Mesh"));

    if (j.find("Material") != j.end())
      readParticleMaterialFromJson(j.at("Material"));

    if (j.find("Contact") != j.end())
      readParticleContactFromJson(j.at("Contact"));

    if (j.find("Neighbor") != j.end())
      readParticleNeighborFromJson(j.at("Neighbor"));

    if (j.find("Particle_Generation") != j.end())
      readParticleGenFromJson(j.at("Particle_Generation"));

    if (d_pGeomVec.size() != d_pMeshVec.size())
      throw std::runtime_error("Number of particle geometry groups must be equal to number of particle mesh groups");
  }

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getParticleGeomExampleJson(std::vector<geom::GeomData> pGeomVec = std::vector<geom::GeomData>()) {

    auto nSets = pGeomVec.size();

    if (nSets == 0)
      return json({});

    auto j = json({{"Sets", nSets}});

    for (size_t i = 0; i < nSets; i++) {
      auto js = json({});
      geom::writeGeometry(js, pGeomVec[i]);
      std::string set_name = "Set_" + std::to_string(i+1);
      j[set_name] = js;
    }

    return j;
  }

  /*!
   * @brief Reads from json object
   */
  void readParticleGeomFromJson(const json &j) {

    if (j.empty())
      return;

    if (d_particleSimType == "Multi_Particle") {
      auto nSets = j.value("Sets", size_t(0));

      d_pGeomVec.resize(nSets);

      for (size_t i = 0; i < nSets; i++) {
        auto set_name = "Set_" + std::to_string(i + 1);
        if (j.find(set_name) == j.end())
          throw std::runtime_error("Set " + set_name + " not found in particle geometry");

        auto js = j.at(set_name);

        // read
        geom::readGeometry(js, d_pGeomVec[i]);
        // create
        geom::createGeomObject(d_pGeomVec[i]);
      }
    } else if (d_particleSimType == "Single_Particle") {
      d_pGeomVec.resize(1);
      auto js = j.find("Set_1") == j.end()? j : j.at("Set_1");
      if (js.find("Type") != j.end()) {
        geom::readGeometry(js, d_pGeomVec[0]);
        geom::createGeomObject(d_pGeomVec[0]);
      }
    }
  }

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getParticleMeshExampleJson(std::vector<std::string> filenameVec = std::vector<std::string>(),
      std::vector<double> meshSizesVec = std::vector<double>()) {

    auto nSets = filenameVec.size();

    if (nSets == 0)
      return json({});

    auto j = json({{"Sets", nSets}});
    for (size_t i = 0; i < nSets; i++) {
      double h = meshSizesVec.size() > i ? meshSizesVec[i] : -1.;
      auto js = inp::MeshDeck::getExampleJson(filenameVec[i], h);

      std::string set_name = "Set_" + std::to_string(i+1);
      j[set_name] = js;
    }

    return j;
  }

  void readParticleMeshFromJson(const json &j) {
    if (j.empty())
      return;

    if (d_particleSimType == "Multi_Particle") {
      auto nSets = j.value("Sets", size_t(0));
      d_pMeshVec.resize(nSets);

      for (size_t i = 0; i < nSets; i++) {
        auto set_name = "Set_" + std::to_string(i + 1);

        if (j.find(set_name) == j.end())
          throw std::runtime_error("Set " + set_name + " not found in particle mesh");

        auto js = j.at(set_name);

        // copy this set from a previous set?
        int copy_set = js.value("Copy_Data", int(-1));
        if (copy_set != -1) {
          std::string copy_set_tag = "Set_" + std::to_string(copy_set);
          auto js_copy = j.at(copy_set_tag);
          d_pMeshVec[i].readFromJson(js_copy);
        } else
          d_pMeshVec[i].readFromJson(js);
      }
    } else if (d_particleSimType == "Single_Particle") {
      d_pMeshVec.resize(1);
      auto js = j.find("Set_1") == j.end()? j : j.at("Set_1");
      d_pMeshVec[0].readFromJson(js);
    }
  }

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getParticleMaterialExampleJson(size_t nSets = 0) {

    if (nSets == 0)
      return json({});

    auto j = json({{"Sets", nSets}});
    for (size_t i = 0; i < nSets; i++) {
      std::string set_name = "Set_" + std::to_string(i+1);
      j[set_name] = {};
    }

    return j;
  }

  void readParticleMaterialFromJson(const json &j) {
    if (j.empty())
      return;

    if (d_particleSimType == "Multi_Particle") {
      auto nSets = j.value("Sets", size_t(0));
      d_pMaterialVec.resize(nSets);

      for (size_t i = 0; i < nSets; i++) {
        auto set_name = "Set_" + std::to_string(i + 1);
        if (j.find(set_name) == j.end())
          throw std::runtime_error("Set " + set_name + " not found in particle mesh");

        auto js = j.at(set_name);

        // copy this set from previous set?
        int copy_set = js.value("Copy_Data", int(-1));
        if (copy_set != -1) {
          std::string copy_set_tag = "Set_" + std::to_string(copy_set);
          auto js_copy = j.at(copy_set_tag);
          d_pMaterialVec[i].readFromJson(js_copy);
        } else {
          d_pMaterialVec[i].readFromJson(js);
        }
      } // loop over sets
    } else if (d_particleSimType == "Single_Particle") {
      d_pMaterialVec.resize(1);
      auto js = j.find("Set_1") == j.end()? j : j.at("Set_1");
      d_pMaterialVec[0].readFromJson(js);
    }
  }

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getParticleContactExampleJson(size_t nSets = 0) {
    return inp::ContactDeck::getExampleJson(nSets);
  }

  void readParticleContactFromJson(const json &j) {
    d_contactDeck.readFromJson(j);
  }

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getParticleNeighborExampleJson(std::string updateCriteria = "simple_all", double sFactor = 1.,
                                             size_t neighUpdateInterval = 1) {
    return inp::PNeighborDeck::getExampleJson(updateCriteria, sFactor, neighUpdateInterval);
  }

  void readParticleNeighborFromJson(const json &j) {
    d_pNeighDeck.readFromJson(j);
  }

  /*!
   * @brief Returns example JSON object for ModelDeck configuration
   * @return JSON object with example configuration
   */
  static json getParticleGenExampleJson(std::string genMethod = "From_File") {
    return inp::PGenDeck::getExampleJson(genMethod);
  }

  void readParticleGenFromJson(const json &j) {
    d_pGenDeck.readFromJson(j);
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
    oss << tabS << "------- ParticleDeck --------" << std::endl << std::endl;

    oss << tabS << "Number of particle geometry groups  = " << d_pGeomVec.size() << std::endl;
    oss << tabS << "Number of particle mesh groups  = " << d_pMeshVec.size() << std::endl;
    oss << tabS << "Number of particle material groups  = " << d_pMaterialVec.size() << std::endl;

    oss << tabS << "Particle geometry data:" << std::endl;
    for (size_t i = 0; i < d_pGeomVec.size(); i++) {
      oss << tabS << "Particle geometry data for group = " << i << std::endl;
      oss << d_pGeomVec[i].printStr(nt + 1, lvl);
    }


    oss << tabS << "Particle mesh data:" << std::endl;
    for (size_t i = 0; i < d_pMeshVec.size(); i++) {
      oss << tabS << "Particle mesh data for group = " << i << std::endl;
      oss << d_pMeshVec[i].printStr(nt + 1, lvl);
    }

    oss << tabS << "Particle material data:" << std::endl;
    for (size_t i = 0; i < d_pMaterialVec.size(); i++) {
      oss << tabS << "Particle material data for group = " << i << std::endl;
      oss << d_pMaterialVec[i].printStr(nt + 1, lvl);
    }

    oss << tabS << "Contact data:" << std::endl;
    oss << d_contactDeck.printStr(nt + 1, lvl);

    oss << tabS << "Neighbor data:" << std::endl;
    oss << d_pNeighDeck.printStr(nt+1, lvl);

    oss << tabS << "Particle generation data:" << std::endl;
    oss << d_pGenDeck.printStr(nt+1, lvl);

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

#endif // INP_PARTICLEDECK_H
