# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)
find_package(PkgConfig)

find_library(YAML_CPP_LIB
        NAMES libyaml-cpp.a libyaml-cpp.so libyaml-cpp.lib libyaml-cpp.dylib
        HINTS /usr/lib64 /usr/local/lib64 /usr/lib/ /usr/local/lib "${YAML_CPP_DIR}/lib/")

find_path(YAML_CPP_INCLUDE yaml-cpp/yaml.h
        HINTS /usr/include /usr/local/include "${YAML_CPP_DIR}/include/")

mark_as_advanced(YAML_CPP_LIB)
mark_as_advanced(YAML_CPP_INCLUDE)

if (NOT YAML_CPP_LIB)
    message(FATAL_ERROR "YAML CPP Library not found. Specify the YAML library location using YAML_CPP_DIR.")
else ()
    include_directories(${YAML_CPP_INCLUDE})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Found Yaml-cpp library")
        message(STATUS "YAML_CPP_INCLUDE = ${YAML_CPP_INCLUDE}")
        message(STATUS "YAML_CPP_LIB = ${YAML_CPP_LIB}")
    endif ()
endif ()
