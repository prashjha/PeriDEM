# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# source - https://vicrucann.github.io/tutorials/quick-cmake-doxygen/

# set input and output files
set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/input-conf.doxy.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/doxy/conf.doxy.in)

# request to configure the file
configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
if (${Enable_CMAKE_Debug_Build})
    message(STATUS "Doxygen build started")
    message(STATUS "DOXYGEN_IN = ${DOXYGEN_IN}")
    message(STATUS "DOXYGEN_OUT = ${DOXYGEN_OUT}")
endif()

# note the option ALL which allows to build the docs together with the application
add_custom_target(doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
