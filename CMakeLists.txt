# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
cmake_minimum_required(VERSION 3.0)

#----------------------------------#
#<<<<<<<<<< Project name >>>>>>>>>>#
#----------------------------------#
project(PeriDEM LANGUAGES CXX)

set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/prashjha/PeriDEM")
set(CMAKE_PROJECT_DESCRIPTION "PeriDEM -- Peridynamically enhanced DEM model for granular media")

# version
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_UPDATE 0)

#----------------------------------#
#<<<<<<<<<< Project setting >>>>>>>>>>#
#----------------------------------#

# config files
configure_file("${PROJECT_SOURCE_DIR}/PeriDEMConfig.h.in" "${PROJECT_SOURCE_DIR}/src/PeriDEMConfig.h")

# executible directory
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

# set the common library directory
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()

set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib${LIBSUFFIX}")

# postfix for debug
set(CMAKE_DEBUG_POSTFIX d)

# cmake scripts to help search dependencies
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

# set default build type "Release"
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# cmake policy <-- check if it is still needed
#cmake_policy(SET CMP0023 OLD)

set(Enable_CMAKE_Debug_Build TRUE CACHE BOOL "Output important information during cmake for isolating build issues")

#----------------------------------#
#<<<<<<<<<< Rpath setting >>>>>>>>>>#
#----------------------------------#
# source: https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#----------------------------------#
#<<<<<<<<<< Testing >>>>>>>>>>#
#----------------------------------#
set(Enable_Tests FALSE CACHE BOOL "Build test executibles and perform tests")
set(Enable_High_Load_Tests FALSE CACHE BOOL "Perform high load tests")
enable_testing()

#----------------------------------#
#<<<<<<<<<< Dependencies >>>>>>>>>>#
#----------------------------------#
find_program (BASH_PROGRAM bash REQUIRED)

if (${Enable_CMAKE_Debug_Build})
    message("BASH_PROGRAM = ${BASH_PROGRAM}")
endif()


#<<<<<<<<<< HPX >>>>>>>>>>#
find_package(HPX REQUIRED)

if (NOT (HPX_BUILD_TYPE STREQUAL CMAKE_BUILD_TYPE))
    message("HPX_BUILD_TYPE: " ${HPX_BUILD_TYPE} " CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
    message(FATAL_ERROR "ERROR: Build type of HPX and PeriDEM library do not match")
endif ()

# include hpx cmake
list(APPEND CMAKE_MODULE_PATH "${HPX_PREFIX}/../cmake/")

if (${Enable_CMAKE_Debug_Build})
    message("HPX_INCLUDE_DIRS = ${HPX_INCLUDE_DIRS}")
endif()

#<<<<<<<<<< YAML >>>>>>>>>>#
find_package(YamlCpp REQUIRED)

#<<<<<<<<<< VTK >>>>>>>>>>#
find_package(VTK REQUIRED)

if (VTK_VERSION VERSION_LESS "6")
    message(FATAL_ERROR "ERROR: Requires VTK above version 6")
endif ()

if (${Enable_CMAKE_Debug_Build})
    message("VTK_DIR = ${VTK_DIR}")
    message("VTK_LIBRARIES = ${VTK_LIBRARIES}")
    message("VTK_INCLUDE_DIRS = ${VTK_INCLUDE_DIRS}")
endif()

#<<<<<<<<<< BLAS+LAPACK >>>>>>>>>>#
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

#<<<<<<<<<< PCL >>>>>>>>>>#
find_package(PCL REQUIRED)

if (${Enable_CMAKE_Debug_Build})
    message("PCL_DIR = ${PCL_DIR}")
    message("PCL_INCLUDE_DIRS = ${PCL_INCLUDE_DIRS}")
    message("PCL_LIBRARIES = ${PCL_LIBRARIES}")
endif()

#----------------------------------#
#<<<<<<<<<< Compiler flags >>>>>>>>>>#
#----------------------------------#
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -fPIC")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()

# set gnu standard to 14
if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "--std=gnu++14 ${CMAKE_CXX_FLAGS}")
    endif ()
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()

if (${Enable_CMAKE_Debug_Build})
    message("CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
    message("CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
    message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
    message("CMAKE_CXX_STANDARD = ${CMAKE_CXX_STANDARD}")
endif()

#----------------------------------#
#<<<<<<<<<< Include directories >>>>>>>>>>#
#----------------------------------#
include_directories(${HPX_INCLUDE_DIRS})
include_directories(SYSTEM external)
include_directories(src)

#----------------------------------#
#<<<<<<<<<< Subdirectories >>>>>>>>>>#
#----------------------------------#
# external libraries
add_subdirectory(external/)

# PeriDEM sub-directories
add_subdirectory(src/util)
add_subdirectory(src/rw)
add_subdirectory(src/inp)
add_subdirectory(src/material)
add_subdirectory(src/loading)
add_subdirectory(src/geometry)
add_subdirectory(src/fe)
add_subdirectory(src/particle)
add_subdirectory(src/nsearch)
add_subdirectory(src/model)

# test directory
if(${Enable_Tests})
    add_subdirectory(test/test_exec)
    add_subdirectory(test)
endif()

# documentation
set(Enable_Documentation FALSE CACHE BOOL "Generate documentation")
if(${Enable_Documentation})
    add_subdirectory(docs)
endif()

#----------------------------------#
#<<<<<<<<<< Applications >>>>>>>>>>#
#----------------------------------#
set(Enable_Apps FALSE CACHE BOOL "Build applications")
if(${Enable_Apps})
    add_subdirectory(apps/twop)
endif()

#----------------------------------#
#<<<<<<<<<< PeriDEM executible >>>>>>>>>>#
#----------------------------------#
add_hpx_executable(PeriDEM SOURCES src/main.cpp DEPENDENCIES Inp Model)


#----------------------------------#
#<<<<<<<<<< Post build operations >>>>>>>>>>#
#----------------------------------#
# test directory
if(${Enable_Tests})
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/test/test_data/
            ${PROJECT_BINARY_DIR}/test/test_data/)
endif()
