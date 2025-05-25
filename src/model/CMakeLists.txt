# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

AUX_SOURCE_DIRECTORY(./ SOURCES)
AUX_SOURCE_DIRECTORY(./dem SOURCES)

add_library(Model ${SOURCES})

target_include_directories(Model PUBLIC ${VTK_INCLUDE_DIRS})

target_link_libraries(Model PUBLIC Util RW Inp FE Mesh Material Loading Fracture Geom Particle NSearch)
