# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

add_library(PeriDEMModel periDEMModel.cpp
  ${PROJECT_SOURCE_DIR}/src/particle/createParticles.cpp
  ${PROJECT_SOURCE_DIR}/src/rw/particleOutput.cpp
)

target_include_directories(PeriDEMModel PUBLIC ${VTK_INCLUDE_DIRS})

target_link_libraries(PeriDEMModel PUBLIC
  Data Util RW Inp FE Mesh Material Loading Fracture Geom Particle NSearch
  MeshGen Contact Pd TimeInt Postprocess
)

target_include_directories(PeriDEMModel
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external>
    $<INSTALL_INTERFACE:include>
)

add_executable(PeriDEM main.cpp)

target_link_libraries(PeriDEM PUBLIC PeriDEMModel)
