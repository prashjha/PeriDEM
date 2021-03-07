# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
find_package(PkgConfig)

find_library(YAML_CPP_LIB
        NAMES libyaml-cpp.a libyaml-cpp.so libyaml-cpp.lib libyaml-cpp.dylib
        HINTS /usr/lib64 /usr/local/lib64 /usr/lib/ /usr/local/lib "${YAML_CPP_DIR}/lib/")
find_path(YAML_CPP_INCLUDE yaml-cpp/yaml.h HINTS /usr/include /usr/local/include "${YAML_CPP_DIR}/include/")

mark_as_advanced(YAML_CPP_LIBRARY_DIR)
mark_as_advanced(YAML_CPP_LIB)
mark_as_advanced(YAML_CPP_INCLUDE)

if (NOT YAML_CPP_LIB)
    message(FATAL_ERROR "YAML CPP Library not found. Specify the YAML library location using YAML_CPP_DIR.")
else ()
    include_directories(${YAML_CPP_INCLUDE})
    #message("YAML Include Dir = ${YAML_CPP_INCLUDE}")
    #message("YAML Lib Dir = ${YAML_CPP_LIB}")
    #message("YAML Library Dir = ${YAML_CPP_LIBRARY_DIR}")
endif ()
