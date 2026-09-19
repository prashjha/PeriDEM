# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

AUX_SOURCE_DIRECTORY(./ SOURCES)
list(FILTER SOURCES EXCLUDE REGEX "createParticles\\.cpp$")

add_library(Particle ${SOURCES})

target_link_libraries(Particle PUBLIC Mesh Material Fracture Util)

target_include_directories(Particle
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)