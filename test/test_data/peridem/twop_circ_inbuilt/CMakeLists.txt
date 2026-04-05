# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

set(EXEC_NAME Test_PeriDEM_twop_circ_inbuilt)

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

add_test(NAME ${EXEC_NAME}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${EXEC_NAME}${CMAKE_EXECUTABLE_SUFFIX}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(${EXEC_NAME} PROPERTIES LABELS "PeriDEM;Examples")
