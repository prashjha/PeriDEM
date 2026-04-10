# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

# ---------------------------------------------------------------------------
# Layout
# ---------------------------------------------------------------------------
# - test_exec/     : unit / library test binaries (TestUtil, TestFE, ...)
# - test_data/     : inputs for those binaries and optional PeriDEM example
#                    executables built from test_data/peridem/*/CMakeLists.txt
# - add_test below : ctest entries (library + PeriDEM in-built examples + optional high-load)
# ---------------------------------------------------------------------------

set(EXECUTABLE_OUTPUT_PATH "${Test_Exec_Path}")

if(${Enable_CMAKE_Debug_Build})
  message(STATUS "Test binary directory = ${EXECUTABLE_OUTPUT_PATH}")
endif()

add_subdirectory(test_exec)

# PeriDEM example executables (in-process decks); ctest entries are below
add_subdirectory(test_data/peridem/twop_circ)
add_subdirectory(test_data/peridem/twop_circ_inbuilt)
add_subdirectory(test_data/peridem/twop_general_inbuilt)
add_subdirectory(test_data/peridem/compression_large_set_inbuilt)

# ---------------------------------------------------------------------------
# CI / environment
# ---------------------------------------------------------------------------
set(IS_CIRCLECI FALSE CACHE BOOL "Circle CI run")
if($ENV{CIRCLECI})
  set(IS_CIRCLECI TRUE CACHE BOOL "Variable indicating we are running in circle-ci")
endif()
message(STATUS "IS_CIRCLECI = '${IS_CIRCLECI}'")

execute_process(
  COMMAND bash -c "if [ -f /.dockerenv ]; then echo 'TRUE'; else echo 'FALSE'; fi"
  OUTPUT_VARIABLE INSIDE_CONTAINER
)
string(STRIP "${INSIDE_CONTAINER}" INSIDE_CONTAINER)
message(STATUS "INSIDE_CONTAINER = '${INSIDE_CONTAINER}'")

# ---------------------------------------------------------------------------
# Library tests
# ---------------------------------------------------------------------------
add_test(NAME test_util
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestUtil
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

add_test(NAME test_fe
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestFE -i ${Test_Data_Path}/fe
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

add_test(NAME test_nsearch
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestNSearch -i 10 -o 0
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

add_test(NAME test_nsearch_closest_point
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestNSearch -i 10 -o 2
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

add_test(NAME test_nsearch_profilenanoflann
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestNSearch -i 10 -o 1
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

add_test(NAME test_meshpartitioning_simple
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 0
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

add_test(NAME test_meshpartitioning_builtinmesh
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 1 -p 4 -m 4 -n 50
  WORKING_DIRECTORY ${Test_Data_Path}/meshpartitioning
)

add_test(NAME test_meshpartitioning_usermesh
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 2 -p 4 -m 4 -f ../common_data/mesh.msh
  WORKING_DIRECTORY ${Test_Data_Path}/meshpartitioning
)

add_test(NAME test_parallelcomp_taskflow
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 0 -i 100000 -nThreads 4
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

if(${INSIDE_CONTAINER} AND ${Disable_Docker_MPI_Tests})
  message(STATUS "Skipping MPI tests test_parallelcomp_builtinmesh_mpi and test_parallelcomp_usermesh_mpi in container")
else()
  message(STATUS "Registering MPI tests test_parallelcomp_builtinmesh_mpi and test_parallelcomp_usermesh_mpi")
  add_test(NAME test_parallelcomp_builtinmesh_mpi
    COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 1 -m 4 -n 50
    WORKING_DIRECTORY ${Test_Data_Path}/parallelcomp
  )
  add_test(NAME test_parallelcomp_usermesh_mpi
    COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 2 -m 4 -f ../common_data/mesh.msh
    WORKING_DIRECTORY ${Test_Data_Path}/parallelcomp
  )
endif()

# ---------------------------------------------------------------------------
# PeriDEM: run simulations with in-built input / mesh (see test_data/peridem/*/main.cpp)
# ---------------------------------------------------------------------------
add_test(NAME Test_PeriDEM_twop_circ
  COMMAND Test_PeriDEM_twop_circ
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_twop_circ>
)
set_tests_properties(Test_PeriDEM_twop_circ PROPERTIES LABELS "PeriDEM;Examples")

add_test(NAME Test_PeriDEM_twop_circ_inbuilt
  COMMAND Test_PeriDEM_twop_circ_inbuilt
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_twop_circ_inbuilt>
)
set_tests_properties(Test_PeriDEM_twop_circ_inbuilt PROPERTIES LABELS "PeriDEM;Examples")

add_test(NAME Test_PeriDEM_twop_general_inbuilt
  COMMAND Test_PeriDEM_twop_general_inbuilt
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_twop_general_inbuilt>
)
set_tests_properties(Test_PeriDEM_twop_general_inbuilt PROPERTIES LABELS "PeriDEM;Examples")

add_test(NAME Test_PeriDEM_compression_large_set_inbuilt
  COMMAND Test_PeriDEM_compression_large_set_inbuilt -numSteps 50
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_compression_large_set_inbuilt>
)
set_tests_properties(Test_PeriDEM_compression_large_set_inbuilt PROPERTIES LABELS "PeriDEM;Examples")

# Main PeriDEM app: on-disk input.json + meshes (built as ${CMAKE_BINARY_DIR}/bin/PeriDEM)
add_test(NAME test_peridem_compression_large_set
  COMMAND ${CMAKE_BINARY_DIR}/bin/PeriDEM -i input.json
  WORKING_DIRECTORY ${Test_Data_Path}/peridem/compression_large_set
)
set_tests_properties(test_peridem_compression_large_set PROPERTIES LABELS "PeriDEM;Examples")

# ---------------------------------------------------------------------------
# PeriDEM: optional high-load integration tests (shell-driven)
# ---------------------------------------------------------------------------
# if(${Enable_High_Load_Tests})
# endif()

# ---------------------------------------------------------------------------
# Apps (optional)
# ---------------------------------------------------------------------------
if(${Enable_Apps})
  message(STATUS "Building tests for apps")

  add_test(NAME test_apps_twoparticle_demo
    COMMAND ${Apps_Path}/twoparticle_demo/TwoParticle_Demo -i ./example/input_0.yaml -nThreads 2
    WORKING_DIRECTORY ${Apps_Path}/twoparticle_demo
  )

  add_test(NAME test_apps_peridynamics
    COMMAND ${Apps_Path}/peridynamics/Peridynamics -i ./example/input_0.yaml -nThreads 2
    WORKING_DIRECTORY ${Apps_Path}/peridynamics
  )
endif()
