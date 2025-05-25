# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

# Find Gmsh
#
# This sets the following variables:
# GMSH_FOUND - True if Gmsh was found.
# GMSH_INCLUDE_DIRS - Directories containing the Gmsh include files.
# GMSH_LIBRARIES - Libraries needed to use Gmsh.
# GMSH_VERSION - The version of Gmsh found.

find_path(GMSH_INCLUDE_DIR
    NAMES gmsh.h
    PATHS
        ${GMSH_DIR}/include
        $ENV{GMSH_DIR}/include
        /usr/include
        /usr/local/include
        /opt/local/include
)

find_library(GMSH_LIBRARY
    NAMES gmsh libgmsh
    PATHS
        ${GMSH_DIR}/lib
        ${GMSH_DIR}/lib64
        $ENV{GMSH_DIR}/lib
        $ENV{GMSH_DIR}/lib64
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        /opt/local/lib
)

# Try to find Gmsh version
if(GMSH_INCLUDE_DIR AND EXISTS "${GMSH_INCLUDE_DIR}/gmsh.h")
    file(STRINGS "${GMSH_INCLUDE_DIR}/gmsh.h" gmsh_version_str
         REGEX "^#define[\t ]+GMSH_VERSION[\t ]+\".*\"")

    string(REGEX REPLACE "^#define[\t ]+GMSH_VERSION[\t ]+\"([^\"]*)\".*" "\\1"
           GMSH_VERSION "${gmsh_version_str}")
    unset(gmsh_version_str)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh
    REQUIRED_VARS
        GMSH_LIBRARY
        GMSH_INCLUDE_DIR
    VERSION_VAR
        GMSH_VERSION
)

if(GMSH_FOUND)
    set(GMSH_LIBRARIES ${GMSH_LIBRARY})
    set(GMSH_INCLUDE_DIRS ${GMSH_INCLUDE_DIR})
endif()

mark_as_advanced(
    GMSH_INCLUDE_DIR
    GMSH_LIBRARY
) 