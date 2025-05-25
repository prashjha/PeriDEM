# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

# Set executable name
set(EXEC_NAME Test_PeriDEM_twop_circ)


# Create executable
add_executable(${EXEC_NAME} main.cpp)

# Link with required libraries
target_link_libraries(${EXEC_NAME} PUBLIC
    MeshGen
    Model
)

# Add include directories
target_include_directories(${EXEC_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${GMSH_INCLUDE_DIRS}
    ${MPI_CXX_INCLUDE_PATH}
)

# Set output directory to current directory
set_target_properties(${EXEC_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add test
add_test(NAME ${EXEC_NAME} COMMAND ${EXEC_NAME})
set_tests_properties(${EXEC_NAME} PROPERTIES LABELS "PeriDEM;Examples") 