# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

set(EXEC_NAME Test_PeriDEM_jha2021_comp_n50)

add_executable(${EXEC_NAME} main.cpp)

target_link_libraries(${EXEC_NAME} PUBLIC
    PeriDEMModel
)

target_include_directories(${EXEC_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${GMSH_INCLUDE_DIRS}
    ${MPI_CXX_INCLUDE_PATH}
)

target_compile_definitions(${EXEC_NAME} PRIVATE
    JHA2021_COMP_MESH_DIR="${CMAKE_SOURCE_DIR}/test/test_data/peridem/jha2021_comp_n50"
)

set_target_properties(${EXEC_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
