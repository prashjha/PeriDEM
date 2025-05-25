# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

add_executable(TestMeshGen testMeshGen.cpp)

target_link_libraries(TestMeshGen PUBLIC
        MeshGen
        Util
        Mesh
        ${GMSH_LIBRARIES})

# Add test
add_test(NAME TestMeshGen COMMAND TestMeshGen)
set_tests_properties(TestMeshGen PROPERTIES LABELS "MeshGen") 