# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
find_package(PkgConfig)

find_library(METIS_LIB
        NAMES libmetis.a libmetis.so libmetis.lib libmetis.dylib
        HINTS /usr/lib64 /usr/local/lib64 /usr/lib/ /usr/local/lib "${METIS_DIR}/lib/")

find_path(METIS_INCLUDE metis.h
        HINTS /usr/include /usr/local/include "${METIS_DIR}/include/")

mark_as_advanced(METIS_LIB)
mark_as_advanced(METIS_INCLUDE)

if (NOT METIS_LIB)
    message(FATAL_ERROR "Metis Library not found. Specify the Metis library location using METIS_DIR.")
else ()
    include_directories(${METIS_INCLUDE})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Found Metis library")
        message(STATUS "METIS_INCLUDE = ${METIS_INCLUDE}")
        message(STATUS "METIS_LIB = ${METIS_LIB}")
    endif ()
endif ()
