# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

AUX_SOURCE_DIRECTORY(./ SOURCES)
AUX_SOURCE_DIRECTORY(./mparticle SOURCES)

add_hpx_library(Material SOURCES ${SOURCES})
target_include_directories(Material PUBLIC ${PCL_INCLUDE_DIRS})
