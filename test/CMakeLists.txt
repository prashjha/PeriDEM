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
add_subdirectory(test_data/peridem/jha2021_comp_n50)

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
  COMMAND Test_PeriDEM_twop_circ_inbuilt -nThreads 4
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_twop_circ_inbuilt>
)
set_tests_properties(Test_PeriDEM_twop_circ_inbuilt PROPERTIES LABELS "PeriDEM;Examples")

# Same binary; long enough that the falling circle contacts (Damage_Z > 0).
# ~60 s at 4 threads on this machine (36000 steps, t=0.012).
add_test(NAME Test_PeriDEM_twop_circ_inbuilt_contact
  COMMAND Test_PeriDEM_twop_circ_inbuilt
    -nThreads 4 -finalTime 0.012 -numSteps 36000
    -requireContact -outputDir contact_out
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_twop_circ_inbuilt>
)
set_tests_properties(Test_PeriDEM_twop_circ_inbuilt_contact PROPERTIES
  LABELS "PeriDEM;Examples"
  TIMEOUT 180
)

# Jha et al. JMPS 2021 Table 2. Frozen v0.1.0 circ_damp .msh (hmin=0.1423 mm),
# Δt=0.2 μs, T=0.04 s. Unique inp/out dirs so ctest -j does not clobber decks.
# ~7 min each at 4 threads (200000 steps).
foreach(jha_n RANGE 1 5)
  add_test(NAME Test_PeriDEM_twop_jha2021_table2_${jha_n}
    COMMAND Test_PeriDEM_twop_circ_inbuilt
      -nThreads 4 -jha2021Table2Test ${jha_n}
      -outputDir jha2021_t${jha_n}_out -inputDir jha2021_t${jha_n}_inp
    WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_twop_circ_inbuilt>
  )
  set_tests_properties(Test_PeriDEM_twop_jha2021_table2_${jha_n} PROPERTIES
    LABELS "PeriDEM;Literature"
    TIMEOUT 720
  )
endforeach()

# Same Table 2 energy (0.6 mm gap + leftover v_y) on in-process Gmsh (lc = R/5).
foreach(jha_n RANGE 1 2)
  add_test(NAME Test_PeriDEM_twop_jha2021_inbuilt_${jha_n}
    COMMAND Test_PeriDEM_twop_circ_inbuilt
      -nThreads 4 -jha2021Table2Test ${jha_n} -inbuiltMesh
      -outputDir jha2021_inbuilt_t${jha_n}_out -inputDir jha2021_inbuilt_t${jha_n}_inp
    WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_twop_circ_inbuilt>
  )
  set_tests_properties(Test_PeriDEM_twop_jha2021_inbuilt_${jha_n} PROPERTIES
    LABELS "PeriDEM;Literature"
    TIMEOUT 720
  )
endforeach()

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

# Short contacting pack (12 circles). Default Δt=0.2 μs, T=0.004 s, require grain–grain contact.
add_test(NAME Test_PeriDEM_jha2021_comp_n50_smoke
  COMMAND Test_PeriDEM_jha2021_comp_n50 -nThreads 4
    -outputDir jha2021_comp_n50_smoke_out -inputDir jha2021_comp_n50_smoke_inp
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_jha2021_comp_n50>
)
set_tests_properties(Test_PeriDEM_jha2021_comp_n50_smoke PROPERTIES
  LABELS "PeriDEM;Literature"
  TIMEOUT 300
)

# T09: 1-rank vs 2-rank particle owners on a short 12-grain run (metrics match).
configure_file(
  ${CMAKE_SOURCE_DIR}/test/test_data/peridem/jha2021_comp_n50/run_mpi_particle_compare.sh
  ${CMAKE_BINARY_DIR}/test/test_data/peridem/jha2021_comp_n50/run_mpi_particle_compare.sh
  COPYONLY
)
add_test(NAME Test_PeriDEM_jha2021_comp_n50_mpi_particle
  COMMAND ${BASH_PROGRAM} ./run_mpi_particle_compare.sh
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_jha2021_comp_n50>
)
set_tests_properties(Test_PeriDEM_jha2021_comp_n50_mpi_particle PROPERTIES
  LABELS "PeriDEM;MPI"
  TIMEOUT 180
)

# T11a: larger pack (N=96) under particle-MPI — stepping-stone toward §4.4 (~502).
# Short horizon exercises MPI at paper-adjacent N; contact assert stays on n50_smoke
# (plate needs ~0.003 s to close the 1.15 Rc gap; full contact at N=96 is offline).
add_test(NAME Test_PeriDEM_jha2021_comp_n96_mpi_scale
  COMMAND mpirun -n 2 --quiet
    $<TARGET_FILE:Test_PeriDEM_jha2021_comp_n50>
    -nThreads 1 -nCols 12 -nRows 8
    -numSteps 800 -finalTime 1.6e-4 -noRequireContact
    -outputDir jha2021_comp_n96_mpi_scale_out
    -inputDir jha2021_comp_n96_mpi_scale_inp
  WORKING_DIRECTORY $<TARGET_FILE_DIR:Test_PeriDEM_jha2021_comp_n50>
)
set_tests_properties(Test_PeriDEM_jha2021_comp_n96_mpi_scale PROPERTIES
  LABELS "PeriDEM;MPI;Literature"
  TIMEOUT 300
)

# Main PeriDEM app: on-disk input.json + meshes (built as ${CMAKE_BINARY_DIR}/bin/PeriDEM)
add_test(NAME test_peridem_compression_large_set
  COMMAND ${CMAKE_BINARY_DIR}/bin/PeriDEM -i input.json
  WORKING_DIRECTORY ${Test_Data_Path}/peridem/compression_large_set
)
set_tests_properties(test_peridem_compression_large_set PROPERTIES LABELS "PeriDEM;Examples")

add_test(NAME test_twoparticle_custom
  COMMAND ${CMAKE_BINARY_DIR}/bin/TwoParticle_Custom
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/apps/twoparticle_custom
)
set_tests_properties(test_twoparticle_custom PROPERTIES LABELS "PeriDEM;Apps")

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
    COMMAND ${Apps_Path}/peridynamics/Peridynamics -i ./example/input_0.json -nThreads 2
    WORKING_DIRECTORY ${Apps_Path}/peridynamics
  )

  # T10: serial vs 2-rank nodal DOF-MPI on Single_Particle PD (input_0, short).
  add_test(NAME test_apps_peridynamics_dof_mpi
    COMMAND ${BASH_PROGRAM} ./run_mpi_dof_compare.sh
    WORKING_DIRECTORY ${Apps_Path}/peridynamics
  )
  set_tests_properties(test_apps_peridynamics_dof_mpi PROPERTIES
    LABELS "PeriDEM;MPI;Apps"
    TIMEOUT 180
  )
endif()
