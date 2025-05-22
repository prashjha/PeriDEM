/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef INP_INPUT_H
#define INP_INPUT_H

#include "deckIncludes.h"
#include <string>
#include <vector>
#include <memory>

/*!
 * @brief Collection of methods and database related to input
 *
 * This namespace provides methods and data members specific to reading input
 * data. We partition the input data into number of small collection of input
 * data referred to as decks, e.g. inp::FractureDeck, inp::MeshDeck, etc. We
 * use structs to define the deck.
 * *
 * Each deck is unique and is designed to initialize the higher level object
 * associated to it without needing information from other decks. For example,
 * fe::Mesh is initialized by inp::MeshDeck.
 *
 * The namespace consists of Input and Policy member classes. Input class is
 * the main class responsible of reading input data into various decks.
 */
namespace inp {

// // forward declarations of decks
// struct MaterialDeck;
// struct MeshDeck;
// struct ModelDeck;
// struct OutputDeck;
// struct RestartDeck;
// struct ParticleDeck;
// struct ParticleZone;
// struct WallZone;
// struct Zone;
// struct ContactDeck;
// struct ContactPairDeck;
// struct TestDeck;

/**
 * \defgroup Input Input
 *
 * Group which reads and stores input data
 */
/**@{*/

/*!
 * @brief A class to read input file
 *
 * In this class we read input file and read the data into various decks.
 * Input file is a YAML file.
 */
class Input {

public:
  /*!
   * @brief Constructor
   * @param filename Filename of input file
   */
  explicit Input(const json &j = json({}));

  /**
   * @name Accessor methods
   */
  /**@{*/

  /*!
   * @brief Get the pointer to model deck
   * @return Pointer to ModelDeck
   */
  std::shared_ptr<inp::ModelDeck> getModelDeck() {return d_modelDeck_p;};

  /*!
   * @brief Get the pointer to output deck
   * @return Pointer to OutputDeck
   */
  std::shared_ptr<inp::OutputDeck> getOutputDeck() {return d_outputDeck_p;};

  /*!
   * @brief Get the pointer to restart deck
   * @return Pointer to RestartDeck
   */
  std::shared_ptr<inp::RestartDeck> getRestartDeck() {return d_restartDeck_p;};

  /*!
   * @brief Get the pointer to particle deck
   * @return Pointer to ParticleDeck
   */
  std::shared_ptr<inp::ParticleDeck> getParticleDeck() {return d_particleDeck_p;};

  std::shared_ptr<inp::TestDeck> getTestDeck() {return d_testDeck_p;};

  std::shared_ptr<inp::BCDeck> getBCDeck() {return d_bcDeck_p;};

  /*!
   * @brief Get particle simulation type
   * @return bool True if Multi_Particle else false
   */
  bool isMultiParticle() {return d_modelDeck_p->d_particleSimType == "Multi_Particle";};

  /*!
   * @brief Specify if PeriDEM model should be run
   * @return bool True if PeriDEM is active
   */
  bool isPeriDEM() {
    if (d_modelDeck_p->d_particleSimType == "Multi_Particle" or
        d_modelDeck_p->d_particleSimType == "Single_Particle")
      return true;

    return false;
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
    oss << tabS << "------- Input --------" << std::endl << std::endl;
    oss << tabS << "Particle sim type = " << d_modelDeck_p->d_particleSimType << std::endl;
    oss << tabS << d_modelDeck_p->printStr(nt+1, lvl) << std::endl;
    oss << tabS << d_outputDeck_p->printStr(nt+1, lvl) << std::endl;
    oss << tabS << d_restartDeck_p->printStr(nt+1, lvl) << std::endl;
    oss << tabS << d_testDeck_p->printStr(nt+1, lvl) << std::endl;
    oss << tabS << d_bcDeck_p->printStr(nt+1, lvl) << std::endl;
    oss << tabS << d_particleDeck_p->printStr(nt+1, lvl) << std::endl;
    return oss.str();
  }

  /*!
   * @brief Prints the information about the object
   *
   * @param nt Number of tabs to append before printing
   * @param lvl Information level (higher means more information)
   */
  void print(int nt = 0, int lvl = 0) const { std::cout << printStr(nt, lvl); }
  /** @}*/

  /**
   * @name Decks
   */
  /**@{*/

  /*! @brief Pointer to deck holding problem related data */
  std::shared_ptr<inp::ModelDeck> d_modelDeck_p;

  /*! @brief Pointer to deck holding output related data */
  std::shared_ptr<inp::OutputDeck> d_outputDeck_p;

  /*! @brief Pointer to deck holding restart related data */
  std::shared_ptr<inp::RestartDeck> d_restartDeck_p;

  /*! @brief Test deck */
  std::shared_ptr<inp::TestDeck> d_testDeck_p;

  /*! @brief Boundary condition deck */
  std::shared_ptr<inp::BCDeck> d_bcDeck_p;

  /*! @brief Pointer to deck holding particle related data */
  std::shared_ptr<inp::ParticleDeck> d_particleDeck_p;
  /** @}*/
};

/** @}*/

} // namespace inp

#endif // INP_INPUT_H
