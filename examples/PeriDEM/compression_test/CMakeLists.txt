# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# 40 circular particles on a grid; in-process deck + DEM run.

set(EXEC_NAME example_compression_test)

add_executable(${EXEC_NAME} main.cpp)

target_link_libraries(${EXEC_NAME} PUBLIC
    Model
)

target_include_directories(${EXEC_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${GMSH_INCLUDE_DIRS}
    ${MPI_CXX_INCLUDE_PATH}
)

set_target_properties(${EXEC_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
