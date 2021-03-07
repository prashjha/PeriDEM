# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
find_package(Doxygen)

configure_file(conf.doxy.in ${CMAKE_CURRENT_SOURCE_DIR}/conf.doxy @ONLY)

add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile} conf.doxy
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
        COMMENT "Doxygen documentation"
        VERBATIM)

