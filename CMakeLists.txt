# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

cmake_minimum_required(VERSION 3.5)

#----------------------------------#
#<<<<<<<<<< Project name >>>>>>>>>>#
#----------------------------------#
project(PeriDEM LANGUAGES CXX C)

set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/prashjha/PeriDEM")
set(CMAKE_PROJECT_DESCRIPTION "PeriDEM -- Peridynamically enhanced DEM model for granular media")

# version
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
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

#----------------------------------#
#<<<<<<<<<< New CMAKE variables >>>>>>>>>>#
#----------------------------------#
set(Enable_CMAKE_Debug_Build TRUE CACHE BOOL "Output important information during cmake")
set(Enable_Documentation FALSE CACHE BOOL "Generate documentation if ON")
set(Enable_Tests FALSE CACHE BOOL "Build test executables and perform tests if ON")
set(Enable_High_Load_Tests FALSE CACHE BOOL "Perform high load tests if ON")
set(Disable_Docker_MPI_Tests TRUE CACHE BOOL "Disable (ON) or enable (OFF) MPI tests inside docker")

#----------------------------------#
#<<<<<<<<<< Rpath setting >>>>>>>>>>#
#----------------------------------#
# source: https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#----------------------------------#
#<<<<<<<<<< Testing >>>>>>>>>>#
#----------------------------------#
if (${Enable_CMAKE_Debug_Build})
    message(STATUS "Enable_Tests = ${Enable_Tests}")
    message(STATUS "Enable_High_Load_Tests = ${Enable_High_Load_Tests}")
    message(STATUS "Disable_Docker_MPI_Tests = ${Disable_Docker_MPI_Tests}")
endif()
enable_testing()

#----------------------------------#
#<<<<<<<<<< Dependencies >>>>>>>>>>#
#----------------------------------#
find_program (BASH_PROGRAM bash REQUIRED)
if (${Enable_CMAKE_Debug_Build})
    message(STATUS "BASH_PROGRAM = ${BASH_PROGRAM}")
endif()

# #<<<<<<<<<< MPI >>>>>>>>>>#
find_package(MPI REQUIRED)
if (${Enable_CMAKE_Debug_Build})
    message(STATUS "MPI_INCLUDE_PATH = ${MPI_INCLUDE_PATH}")
endif()

# #<<<<<<<<<< Threads >>>>>>>>>>#
find_package(Threads REQUIRED)

# #<<<<<<<<<< YAML >>>>>>>>>>#
find_package(YamlCpp REQUIRED)

# #<<<<<<<<<< VTK >>>>>>>>>>#
#find_package(VTK REQUIRED)
find_package(VTK COMPONENTS CommonCore CommonDataModel IOXML)

if (VTK_VERSION VERSION_LESS "6")
    message(FATAL_ERROR "ERROR: Requires VTK above version 6")
endif ()

if (${Enable_CMAKE_Debug_Build})
    message(STATUS "VTK_MAJOR_VERSION = ${VTK_MAJOR_VERSION}")
    message(STATUS "VTK_DIR = ${VTK_DIR}")
    message(STATUS "VTK_LIBRARIES = ${VTK_LIBRARIES}")
endif()

# #<<<<<<<<<< BLAS+LAPACK >>>>>>>>>>#
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

# #<<<<<<<<<< Metis >>>>>>>>>>#
find_package(Metis REQUIRED)

# #<<<<<<<<<< Doxygen >>>>>>>>>>#
if(${Enable_Documentation})
    find_package(Doxygen)
endif()

#----------------------------------#
#<<<<<<<<<< Compiler flags >>>>>>>>>>#
#----------------------------------#
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -fPIC")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()

# set gnu standard to 17
if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "--std=gnu++17 ${CMAKE_CXX_FLAGS}")
    endif ()
else ()
    set(CMAKE_CXX_STANDARD 17)
endif ()

if (${Enable_CMAKE_Debug_Build})
    message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
    message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
    message(STATUS "CMAKE_CXX_STANDARD = ${CMAKE_CXX_STANDARD}")
endif()

#----------------------------------#
#<<<<<<<<<< Include directories >>>>>>>>>>#
#----------------------------------#
if (${Enable_CMAKE_Debug_Build})
    message(STATUS "Including key directories")
endif()
include_directories(SYSTEM external)
include_directories(${MPI_INCLUDE_PATH})
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
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Building test")
    endif()
    add_subdirectory(test/test_exec)
    add_subdirectory(test)
endif()

# documentation
if(${Enable_Documentation})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Building documentation")
    endif()
    add_subdirectory(docs)
endif()

#----------------------------------#
#<<<<<<<<<< Applications >>>>>>>>>>#
#----------------------------------#
set(Enable_Apps FALSE CACHE BOOL "Build applications if ON")
if(${Enable_Apps})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Building apps")
    endif()
    add_subdirectory(apps/twop)
endif()

#----------------------------------#
#<<<<<<<<<< PeriDEM executible >>>>>>>>>>#
#----------------------------------#
add_executable(PeriDEM src/main.cpp)

target_link_libraries(PeriDEM PUBLIC Model)

#----------------------------------#
#<<<<<<<<<< Post build operations >>>>>>>>>>#
#----------------------------------#
# test directory
if(${Enable_Tests})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Copying test directory to build directory")
    endif()
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/test/test_data/
            ${PROJECT_BINARY_DIR}/test/test_data/)
endif()
