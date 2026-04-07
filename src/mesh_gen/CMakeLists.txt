# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

set(MESH_GEN_SOURCES
    annulusMesh2D.cpp
    annulusMesh3D.cpp
    builtinGmshGeometry.cpp
    gmshMeshPipeline.cpp
    meshGenerator.cpp
    openBoundaryWalls2D.cpp
    openBoundaryWalls3D.cpp
    primitiveOccMesh.cpp
)

add_library(MeshGen ${MESH_GEN_SOURCES})

target_link_libraries(MeshGen PUBLIC
    Util
    Mesh
    Geom
    ${GMSH_LIBRARIES}) 

target_include_directories(MeshGen
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external>
    $<INSTALL_INTERFACE:include>
)