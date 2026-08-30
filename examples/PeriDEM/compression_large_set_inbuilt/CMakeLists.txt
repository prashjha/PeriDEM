# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# In-process deck generator + DEM run (same role as
# test/test_data/peridem/compression_large_set_inbuilt, separate binary name).

set(EXEC_NAME example_compression_large_set_inbuilt)

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
