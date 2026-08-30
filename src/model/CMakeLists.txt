# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

AUX_SOURCE_DIRECTORY(./ SOURCES)
AUX_SOURCE_DIRECTORY(./dem SOURCES)

list(FILTER SOURCES EXCLUDE REGEX "createParticles\\.cpp$")

add_library(Model ${SOURCES})

target_include_directories(Model PUBLIC ${VTK_INCLUDE_DIRS})

target_sources(Model PRIVATE
  ${PROJECT_SOURCE_DIR}/src/particle/createParticles.cpp
  ${PROJECT_SOURCE_DIR}/src/rw/particleOutput.cpp
)

target_link_libraries(Model PUBLIC Util RW Inp FE Mesh Material Loading Fracture Geom Particle NSearch MeshGen Contact Pd TimeInt Postprocess)

target_include_directories(Model
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external>
    $<INSTALL_INTERFACE:include>
)