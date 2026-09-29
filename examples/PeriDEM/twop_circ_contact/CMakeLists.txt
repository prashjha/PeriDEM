# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Two circles with enough integration time that the falling particle
# contacts the fixed one. Same source as Test_PeriDEM_twop_circ_inbuilt;
# ctest keeps the short 0.002 s window.

set(EXEC_NAME example_twop_circ_contact)

add_executable(${EXEC_NAME}
  ${PROJECT_SOURCE_DIR}/test/test_data/peridem/twop_circ_inbuilt/main.cpp
)

target_compile_definitions(${EXEC_NAME} PRIVATE TWOP_CONTACT_EXAMPLE)

target_link_libraries(${EXEC_NAME} PUBLIC
    PeriDEMModel
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
