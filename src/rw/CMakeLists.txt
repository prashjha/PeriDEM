# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

AUX_SOURCE_DIRECTORY(./ SOURCES)

add_hpx_library(RW SOURCES ${SOURCES})

target_include_directories(RW PUBLIC ${VTK_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src/external/ ${PCL_INCLUDE_DIRS})
target_link_libraries(RW PUBLIC ${VTK_LIBRARIES})
