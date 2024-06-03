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

// forward declarations of decks
struct MaterialDeck;
struct MeshDeck;
struct ModelDeck;
struct OutputDeck;
struct RestartDeck;
struct ParticleDeck;
struct ParticleZone;
struct WallZone;
struct Zone;
struct ContactDeck;
struct ContactPairDeck;

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
  explicit Input(const std::string &filename);

  /**
   * @name Accessor methods
   */
  /**@{*/

  /*!
   * @brief Get the pointer to material deck
   * @return Pointer to MaterialDeck
   */
  std::shared_ptr<inp::MaterialDeck> getMaterialDeck();

  /*!
   * @brief Get the pointer to mesh deck
   * @return Pointer to GeometryDeck
   */
  std::shared_ptr<inp::MeshDeck> getMeshDeck();

  /*!
   * @brief Get the pointer to model deck
   * @return Pointer to ModelDeck
   */
  std::shared_ptr<inp::ModelDeck> getModelDeck();

  /*!
   * @brief Get the pointer to output deck
   * @return Pointer to OutputDeck
   */
  std::shared_ptr<inp::OutputDeck> getOutputDeck();

  /*!
   * @brief Get the pointer to restart deck
   * @return Pointer to RestartDeck
   */
  std::shared_ptr<inp::RestartDeck> getRestartDeck();

  /*!
   * @brief Get the pointer to particle deck
   * @return Pointer to ParticleDeck
   */
  std::shared_ptr<inp::ParticleDeck> getParticleDeck();

  /*!
   * @brief Get the pointer to contact deck
   * @return Pointer to ContactDeck
   */
  std::shared_ptr<inp::ContactDeck> getContactDeck();

  /*!
   * @brief Specify if this is PeriDEM simulation
   *
   * @return True If it is PeriDEM simulation
   */
  bool isPeriDEM();

  /** @}*/

private:
  /**
   * @name Setter methods
   *
   * Reads input file into the respective decks
   */
  /**@{*/

  /*!
   * @brief Read data into material deck and store its pointer
   */
  void setMaterialDeck();

  /*!
   * @brief Read data into mesh deck and store its pointer
   */
  void setMeshDeck();

  /*!
   * @brief Read data into model deck and store its pointer
   */
  void setModelDeck();

  /*!
   * @brief Read data into output deck and store its pointer
   */
  void setOutputDeck();

  /*!
   * @brief Read data into restart deck and store its pointer
   */
  void setRestartDeck();

  /*!
   * @brief Read data into particle deck and store its pointer
   */
  void setParticleDeck();

  /*!
   * @brief Read data into material deck and store its pointer
   *
   * @param s_config Config file to read data
   * @param m_deck Pointer to material deck
   * @param zone_id Id of zone
   */
  void setZoneMaterialDeck(std::vector<std::string> s_config,
      inp::MaterialDeck *m_deck, size_t zone_id);

  /*!
   * @brief Read data into mesh deck and store its pointer
   *
   * @param s_config Config file to read data
   * @param mesh_deck Pointer to mesh deck
   */
  void setZoneMeshDeck(std::vector<std::string> s_config,
                       inp::MeshDeck *mesh_deck);

  /*!
   * @brief Read zone data
   *
   * @param s_config Config file to read data
   * @param zone_data Pointer to Zone object
   */
  void setZoneData(std::vector<std::string> s_config,
                               inp::Zone *zone_data);

  /*!
   * @brief Read particle data
   *
   * @param string_zone String associated with zone to get the data from YAML file
   * @param particle_data Pointer to particle data
   */
  void setParticleData(std::string string_zone,
                                   inp::ParticleZone *particle_data);

  /*!
   * @brief Read wall data
   *
   * @param string_zone String associated with zone to get the data from YAML file
   * @param wall_data Pointer to wall data
   */
  void setWallData(std::string string_zone,
                       inp::WallZone *wall_data);

  /*!
   * @brief Read data into particle deck and store its pointer
   */
  void setContactDeck();

  /** @}*/

  /**
   * @name Internal data
   */
  /**@{*/

  /*! @brief Name of input file */
  std::string d_inputFilename;

  /** @}*/

  /**
   * @name Decks
   */
  /**@{*/

  /*!
   * @brief Pointer to deck holding material related data
   *
   * E.g. type of material, influence function information, parameters, etc
   */
  std::shared_ptr<inp::MaterialDeck> d_materialDeck_p;

  /*!
   * @brief Pointer to deck holding geometry related data
   *
   * E.g. dimension, discretization type, mesh file, etc
   */
  std::shared_ptr<inp::MeshDeck> d_meshDeck_p;

  /*!
   * @brief Pointer to deck holding problem related data
   *
   * E.g. type of simulation (central-difference, velocity-verlet, implicit)
   * etc
   */
  std::shared_ptr<inp::ModelDeck> d_modelDeck_p;

  /*!
   * @brief Pointer to deck holding output related data
   *
   * E.g. output frequency, output file format, output element-node
   * connectivity flag, etc
   */
  std::shared_ptr<inp::OutputDeck> d_outputDeck_p;

  /*!
   * @brief Pointer to deck holding restart related data such as restart
   * filename and restart time step
   */
  std::shared_ptr<inp::RestartDeck> d_restartDeck_p;

  /*!
   * @brief Pointer to deck holding particle related data
   *
   * E.g. particle geometry, size, initial arrangement of particle, etc
   */
  std::shared_ptr<inp::ParticleDeck> d_particleDeck_p;

  /*!
   * @brief Pointer to deck holding particle related data
   *
   * E.g. particle geometry, size, initial arrangement of particle, etc
   */
  std::shared_ptr<inp::ContactDeck> d_contactDeck_p;

  /** @}*/
};

/** @}*/

} // namespace inp

#endif // INPUT_H
