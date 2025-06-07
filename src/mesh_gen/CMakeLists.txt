# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

AUX_SOURCE_DIRECTORY(./ SOURCES)

add_library(MeshGen ${SOURCES})

target_include_directories(MeshGen PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${GMSH_INCLUDE_DIRS}
)

target_link_libraries(MeshGen PUBLIC
    Util
    Mesh
    ${GMSH_LIBRARIES}) 