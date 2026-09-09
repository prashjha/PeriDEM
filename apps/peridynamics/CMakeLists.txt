# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(Peridynamics main.cpp)

target_link_libraries(Peridynamics PUBLIC Inp PeriDEMModel)

if (${Enable_CMAKE_Debug_Build})
    message(STATUS "Copying apps/peridynamics example directory to build directory")
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/example/
        DESTINATION ${EXECUTABLE_OUTPUT_PATH}/example/)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run_mpi_dof_compare.sh
  ${EXECUTABLE_OUTPUT_PATH}/run_mpi_dof_compare.sh
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run_mpi_output_art_dof.sh
  ${EXECUTABLE_OUTPUT_PATH}/run_mpi_output_art_dof.sh
  COPYONLY
)
