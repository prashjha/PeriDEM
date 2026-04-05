# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

set(MESH_GEN_SOURCES
    builtinGmshGeometry.cpp
    gmshMeshPipeline.cpp
    meshGenerator.cpp
)

add_library(MeshGen ${MESH_GEN_SOURCES})

target_include_directories(MeshGen PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${GMSH_INCLUDE_DIRS}
)

target_link_libraries(MeshGen PUBLIC
    Util
    Mesh
    Geom
    ${GMSH_LIBRARIES}) 