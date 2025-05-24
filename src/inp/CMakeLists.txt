# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

AUX_SOURCE_DIRECTORY(./ SOURCES)

#include_directories(${YAML_CPP_INCLUDE})
#add_hpx_library(Inp SOURCES ${SOURCES} DEPENDENCIES ${YAML_CPP_LIB} Util)
#target_include_directories(RW PUBLIC ${YAML_CPP_INCLUDE})
#target_link_libraries(Inp PUBLIC Util ${YAML_CPP_LIB})

add_library(Inp ${SOURCES})

target_link_libraries(Inp PUBLIC Util Geom) # PRIVATE nlohmann_json::nlohmann_json)