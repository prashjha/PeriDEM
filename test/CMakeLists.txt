# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

#----------------------------------#
#<<<<<<<<<< set exec path >>>>>>>>>>#
#----------------------------------#
set(EXECUTABLE_OUTPUT_PATH "${Test_Exec_Path}")

if (${Enable_CMAKE_Debug_Build})
    message(STATUS "Test binary directory = ${EXECUTABLE_OUTPUT_PATH}")
endif()

#----------------------------------#
#<<<<<<<<<< test exec >>>>>>>>>>#
#----------------------------------#
add_subdirectory(test_exec)

#----------------------------------#
#<<<<<<<<<< Lib tests >>>>>>>>>>#
#----------------------------------#

# find if we are building in circle-ci
set(IS_CIRCLECI FALSE CACHE BOOL "Circle-ci variable")
if ($ENV{CIRCLECI})
    set(IS_CIRCLECI TRUE CACHE BOOL "Variable indicating we are running in circle-ci")
endif ()
message(STATUS "IS_CIRCLECI = '${IS_CIRCLECI}'")

# find if we are inside docker container
execute_process (
        COMMAND bash -c
        "if [ -f /.dockerenv ]; then
            echo 'TRUE';
        else
            echo 'FALSE';
        fi"
        OUTPUT_VARIABLE INSIDE_CONTAINER
)
string(STRIP ${INSIDE_CONTAINER} INSIDE_CONTAINER)
message(STATUS "INSIDE_CONTAINER = '${INSIDE_CONTAINER}'")

#message("INSIDE_CONTAINER_BASH = ${INSIDE_CONTAINER_BASH}")
#set(INSIDE_CONTAINER ${INSIDE_CONTAINER_BASH} CACHE BOOL "Variable indicating we are inside container or not")
#if (INSIDE_CONTAINER)
#    message("YES")
#else ()
#    message("NO")
#endif ()

# util lib
add_test(NAME test_util
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestUtil
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        )

# fe lib
add_test(NAME test_fe
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestFE -i ${Test_Data_Path}/fe
    WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

# nsearch lib
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

# mesh partitioning test
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

# parallel computation test
add_test(NAME test_parallelcomp_taskflow
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 0 -i 100000 -nThreads 4
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

if (${INSIDE_CONTAINER} AND ${Disable_Docker_MPI_Tests})
    message(STATUS "Not building MPI tests testparallelcomp_builtinmesh_mpi and testparallelcomp_usermesh_mpi inside containers")
else ()
    message(STATUS "Building MPI tests testparallelcomp_builtinmesh_mpi and testparallelcomp_usermesh_mpi")
    add_test(NAME test_parallelcomp_builtinmesh_mpi
            COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 1 -m 4 -n 50
            WORKING_DIRECTORY ${Test_Data_Path}/parallelcomp
    )
    add_test(NAME test_parallelcomp_usermesh_mpi
            COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 2 -m 4 -f ../common_data/mesh.msh
            WORKING_DIRECTORY ${Test_Data_Path}/parallelcomp
    )
endif ()

#----------------------------------#
#<<<<<<<<<< PeriDEM tests >>>>>>>>>>#
#----------------------------------#
#if (BASH_PROGRAM)
add_test(NAME test_peridem_twop_circ
        COMMAND ${BASH_PROGRAM} ./run.sh
        WORKING_DIRECTORY ${Test_Data_Path}/peridem/twop_circ
        )

add_test(NAME test_peridem_attrition_mix_particles_small_set
        COMMAND ${BASH_PROGRAM} ./run.sh
        WORKING_DIRECTORY ${Test_Data_Path}/peridem/attrition_mix_particles_small_set
)

add_test(NAME test_peridem_compression_small_set
        COMMAND ${BASH_PROGRAM} ./run.sh
        WORKING_DIRECTORY ${Test_Data_Path}/peridem/compression_small_set
)

add_test(NAME test_peridem_single_particle_circle
        COMMAND ${BASH_PROGRAM} ./run.sh
        WORKING_DIRECTORY ${Test_Data_Path}/peridem/single_particle_circle
)

add_test(NAME test_peridem_single_particle_rectangle_inbuilt_mesh
        COMMAND ${BASH_PROGRAM} ./run.sh
        WORKING_DIRECTORY ${Test_Data_Path}/peridem/single_particle_rectangle_inbuilt_mesh
)

if (${Enable_High_Load_Tests})

    add_test(NAME test_peridem_twop_concave_and_hex
            COMMAND ${BASH_PROGRAM} ./run.sh
            WORKING_DIRECTORY ${Test_Data_Path}/peridem/twop_concave_and_hex
            )

    add_test(NAME test_peridem_twop_wall_concave_diff_material_diff_size
            COMMAND ${BASH_PROGRAM} ./run.sh
            WORKING_DIRECTORY ${Test_Data_Path}/peridem/twop_wall_concave_diff_material_diff_size
    )

    add_test(NAME test_peridem_attrition_mix_particles_large_set
            COMMAND ${BASH_PROGRAM} ./run.sh
            WORKING_DIRECTORY ${Test_Data_Path}/peridem/attrition_mix_particles_large_set
    )

    add_test(NAME test_peridem_compression_large_set
            COMMAND ${BASH_PROGRAM} ./run.sh
            WORKING_DIRECTORY ${Test_Data_Path}/peridem/compression_large_set
    )

    #add_test(NAME test_peridem_twop_concave
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_concave
    #        )

    #add_test(NAME test_peridem_twop_circ_diff_radius
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_circ_diff_radius
    #        )

    #add_test(NAME test_peridem_twop_circ_diff_material
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_circ_diff_material
    #        )

    #add_test(NAME test_peridem_twop_circ_diff_radius_diff_material
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_circ_diff_radius_diff_material
    #        )

    #add_test(NAME test_peridem_twop_sphere
    #        COMMAND ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_sphere
    #        )

    #    add_test(NAME test_peridem_compressive_n25
    #            COMMAND ${BASH_PROGRAM} ./run.sh
    #            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/compressive_n25
    #            )

    #add_test(NAME test_peridem_compressive_n51
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/compressive_n51
    #        )

    #    add_test(NAME test_peridem_compressive_n51_restart
    #            COMMAND ./run.sh
    #            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/compressive_n51_restart
    #            )
endif() # high load tests

#endif ()


#----------------------------------#
#<<<<<<<<<< apps tests >>>>>>>>>>#
#----------------------------------#

if (${Enable_Apps})
    message(STATUS "Building tests for apps")

    add_test(NAME test_apps_twoparticle_demo
            COMMAND ${Apps_Path}/twoparticle_demo/TwoParticle_Demo -i ./example/input_0.yaml -nThreads 2
            WORKING_DIRECTORY ${Apps_Path}/twoparticle_demo
    )

    add_test(NAME test_apps_peridynamics
            COMMAND ${Apps_Path}/peridynamics/Peridynamics -i ./example/input_0.yaml -nThreads 2
            WORKING_DIRECTORY ${Apps_Path}/peridynamics
    )
endif ()