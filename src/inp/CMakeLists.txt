# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

AUX_SOURCE_DIRECTORY(./ SOURCES)
AUX_SOURCE_DIRECTORY(./decks SOURCES)

#include_directories(${YAML_CPP_INCLUDE})
#add_hpx_library(Inp SOURCES ${SOURCES} DEPENDENCIES ${YAML_CPP_LIB} Util)
#target_include_directories(RW PUBLIC ${YAML_CPP_INCLUDE})
#target_link_libraries(Inp PUBLIC Util ${YAML_CPP_LIB})

add_library(Inp ${SOURCES})
target_link_libraries(Inp PUBLIC Util ${YAML_CPP_LIB})