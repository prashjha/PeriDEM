# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include_directories(${PCL_INCLUDE_DIRS})
#link_directories(${PCL_LIBRARY_DIRS})

AUX_SOURCE_DIRECTORY(./ SOURCES)

add_library(NSearch SHARED ${SOURCES})

target_link_libraries(NSearch PUBLIC nanoflann::nanoflann)
