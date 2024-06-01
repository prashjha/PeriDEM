# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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
add_test(NAME testutil
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestUtil
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        )

# fe lib
add_test(NAME testfe
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestFE -i ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/fe
    WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

# nsearch lib
add_test(NAME testnsearch
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestNSearch -i 20
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

# mesh partitioning test
add_test(NAME testmeshpartitioning_simple
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 0
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

add_test(NAME testmeshpartitioning_builtinmesh
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 1 -p 4 -m 4 -n 50
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/meshpartitioning
)

add_test(NAME testmeshpartitioning_usermesh
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 2 -p 4 -m 4 -f mesh.msh
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/meshpartitioning
)

# parallel computation test
add_test(NAME testparallelcomp_taskflow
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 0 -i 100000
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

if (${INSIDE_CONTAINER} AND ${Disable_Docker_MPI_Tests})
    message(STATUS "Not building MPI tests testparallelcomp_builtinmesh_mpi and testparallelcomp_usermesh_mpi inside containers")
else ()
    message(STATUS "Building MPI tests testparallelcomp_builtinmesh_mpi and testparallelcomp_usermesh_mpi")
    add_test(NAME testparallelcomp_builtinmesh_mpi
            COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 1 -m 4 -n 50
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/testparallelcomp
    )
    add_test(NAME testparallelcomp_usermesh_mpi
            COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 2 -m 4 -f mesh.msh
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/testparallelcomp
    )
endif ()

#----------------------------------#
#<<<<<<<<<< PeriDEM tests >>>>>>>>>>#
#----------------------------------#
#if (BASH_PROGRAM)
add_test(NAME twop_circ
        COMMAND ${BASH_PROGRAM} ./run.sh
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_circ
        )

# add_test(NAME twop_wall_concave_diff_material_diff_size
#         COMMAND ${BASH_PROGRAM} ./run.sh
#         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_wall_concave_diff_material_diff_size
#         )

if (${Enable_High_Load_Tests})

    add_test(NAME twop_concave_and_hex
            COMMAND ${BASH_PROGRAM} ./run.sh
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_concave_and_hex
            )

    #add_test(NAME twop_concave
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_concave
    #        )

    #add_test(NAME twop_circ_diff_radius
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_circ_diff_radius
    #        )

    #add_test(NAME twop_circ_diff_material
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_circ_diff_material
    #        )

    #add_test(NAME twop_circ_diff_radius_diff_material
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_circ_diff_radius_diff_material
    #        )

    #add_test(NAME twop_sphere
    #        COMMAND ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/twop_sphere
    #        )

    add_test(NAME compressive_n25
            COMMAND ${BASH_PROGRAM} ./run.sh
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/compressive_n25
            )

    #add_test(NAME compressive_n51
    #        COMMAND ${BASH_PROGRAM} ./run.sh
    #        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/compressive_n51
    #        )

    add_test(NAME compressive_n51_restart
            COMMAND ./run.sh
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/peridem/compressive_n51_restart
            )
endif() # high load tests

#endif ()
