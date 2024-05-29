# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#----------------------------------#
#<<<<<<<<<< Lib tests >>>>>>>>>>#
#----------------------------------#

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

# parallel computation test
add_test(NAME testparallelcomp1
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 0
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)
add_test(NAME testparallelcomp2
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 1 -i 1000000
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)
# MPI tests are disabled in circleci at the moment
set(IS_CIRCLECI FALSE CACHE BOOL "Circle-ci variable")
if ($ENV{CIRCLECI})
    set(IS_CIRCLECI TRUE)
endif ()

message("ENV{CIRCLECI} = $ENV{CIRCLECI}")
message("IS_CIRCLECI = ${IS_CIRCLECI}")
#if (NOT ${IS_CIRCLECI})
#    message("Building testparallelcomp3 that used MPI")
#    add_test(NAME testparallelcomp3
#            COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 2 -n 50 -m 3
#            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
#    )
#endif ()

add_test(NAME testparallelcomp3
        COMMAND mpirun -n 2 ${EXECUTABLE_OUTPUT_PATH}/TestParallelComp -o 2 -n 50 -m 3
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
)

# mesh partitioning test
add_test(NAME testmeshpartitioning1
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 0 -n 50 -m 4
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/meshpartitioning
)

add_test(NAME testmeshpartitioning2
        COMMAND ${EXECUTABLE_OUTPUT_PATH}/TestMeshPartitioning -o 1 -p 4 -m 4 -f mesh.msh
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/../test/test_data/meshpartitioning
)

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
