# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

cmake_minimum_required(VERSION 3.10...4.3)

# Homebrew LLVM on macOS: CMake may use /usr/bin/ar with llvm-ranlib for Clang, which corrupts
# static libraries (linker: "archive member '/' not a mach-o file"). AR/RANLIB must be set
# *before* project() so generated archive rules pick up llvm-ar; updating the cache afterward
# does not rewrite those rules.
if(APPLE)
  set(_pdem_cxx_for_ar "$ENV{CXX}")
  if(NOT _pdem_cxx_for_ar AND DEFINED CMAKE_CXX_COMPILER)
    set(_pdem_cxx_for_ar "${CMAKE_CXX_COMPILER}")
  endif()
  if(_pdem_cxx_for_ar)
    get_filename_component(_PDEM_LLVM_BIN "${_pdem_cxx_for_ar}" DIRECTORY)
    if(EXISTS "${_PDEM_LLVM_BIN}/llvm-ar" AND EXISTS "${_PDEM_LLVM_BIN}/llvm-ranlib")
      set(CMAKE_AR "${_PDEM_LLVM_BIN}/llvm-ar" CACHE FILEPATH "Archiver for static libraries" FORCE)
      set(CMAKE_RANLIB "${_PDEM_LLVM_BIN}/llvm-ranlib" CACHE FILEPATH "Ranlib for static libraries" FORCE)
    endif()
    unset(_PDEM_LLVM_BIN)
  endif()
  unset(_pdem_cxx_for_ar)
endif()

# Homebrew LLVM on macOS: unlike AppleClang, the driver often omits an explicit SDK root in
# flags that clang-scan-deps (C++20/23 dependency scanning with Ninja) reproduces, so libc++
# cannot find system C headers (e.g. time.h). Pin the SDK before compiler tests / project().
if(APPLE AND NOT CMAKE_OSX_SYSROOT)
  execute_process(
    COMMAND xcrun --sdk macosx --show-sdk-path
    OUTPUT_VARIABLE _pdem_osx_sysroot
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(_pdem_osx_sysroot)
    set(CMAKE_OSX_SYSROOT "${_pdem_osx_sysroot}" CACHE PATH "macOS SDK root" FORCE)
  endif()
  unset(_pdem_osx_sysroot)
endif()

#----------------------------------#
#<<<<<<<<<< Project name >>>>>>>>>>#
#----------------------------------#
project(PeriDEM VERSION 0.2.1 LANGUAGES CXX C)

set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/prashjha/PeriDEM")
set(CMAKE_PROJECT_DESCRIPTION "PeriDEM -- Peridynamics-based high-fidelity model for granular media")

#----------------------------------#
#<<<<<<<<<< Project setting >>>>>>>>>>#
#----------------------------------#

# Configure header with version info
configure_file(
  "${PROJECT_SOURCE_DIR}/PeriDEMConfig.h.in"
  "${PROJECT_BINARY_DIR}/PeriDEMConfig.h"
  @ONLY
)

# Use version from project() for CMake package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/PeriDEMConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# executable directory
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
#<<<<<<<<<< Compiler flags >>>>>>>>>>#
#----------------------------------#
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -fPIC")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()

# Homebrew Clang + explicit macOS SDK: link may otherwise use the SDK libc++ while objects were
# built against Homebrew libc++ headers, causing missing symbols (e.g. std::__1::__hash_memory).
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  get_filename_component(_PDEM_LLVM_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(_PDEM_LLVM_ROOT "${_PDEM_LLVM_BIN}" DIRECTORY)
  set(_PDEM_LLVM_LIBCXX "${_PDEM_LLVM_ROOT}/lib/c++")
  if(EXISTS "${_PDEM_LLVM_LIBCXX}/libc++.dylib")
    set(_PDEM_LLVM_LIBCXX_LINK "-L${_PDEM_LLVM_LIBCXX} -Wl,-rpath,${_PDEM_LLVM_LIBCXX}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_PDEM_LLVM_LIBCXX_LINK}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${_PDEM_LLVM_LIBCXX_LINK}")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${_PDEM_LLVM_LIBCXX_LINK}")
  endif()
  unset(_PDEM_LLVM_LIBCXX_LINK)
  unset(_PDEM_LLVM_LIBCXX)
  unset(_PDEM_LLVM_ROOT)
  unset(_PDEM_LLVM_BIN)
endif()

# C++20: std::format and related library facilities; no C++23-only features required in-tree.
set(CMAKE_CXX_STANDARD 20)

if (${Enable_CMAKE_Debug_Build})
    message(STATUS "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
    message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
    message(STATUS "CMAKE_CXX_STANDARD = ${CMAKE_CXX_STANDARD}")
endif()

#----------------------------------#
#<<<<<<<<<< New CMAKE variables >>>>>>>>>>#
#----------------------------------#
set(Enable_CMAKE_Debug_Build TRUE CACHE BOOL "Output important information during cmake")
set(Enable_Documentation FALSE CACHE BOOL "Generate documentation if ON")
set(Enable_Tests FALSE CACHE BOOL "Build test executables and perform tests if ON")
set(Enable_High_Load_Tests FALSE CACHE BOOL "Perform high load tests if ON")
set(Disable_Docker_MPI_Tests TRUE CACHE BOOL "Disable (ON) or enable (OFF) MPI tests inside docker")
set(Enable_Apps FALSE CACHE BOOL "Build applications if ON")
set(Test_Exec_Path "${CMAKE_CURRENT_BINARY_DIR}/test/bin")
set(Test_Data_Path "${CMAKE_CURRENT_BINARY_DIR}/test/test_data/")
set(Apps_Path "${CMAKE_CURRENT_BINARY_DIR}/apps/")

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
    message(STATUS "Test_Exec_Path = ${Test_Exec_Path}")
    message(STATUS "Test_Data_Path = ${Test_Data_Path}")
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

# Add Gmsh
find_package(Gmsh REQUIRED)
include_directories(${GMSH_INCLUDE_DIRS})

#----------------------------------#
#<<<<<<<<<< Include directories >>>>>>>>>>#
#----------------------------------#
if (${Enable_CMAKE_Debug_Build})
    message(STATUS "Including key directories")
endif()
# include_directories(SYSTEM external)
# include_directories(${MPI_INCLUDE_PATH})
# include_directories(src)

#----------------------------------#
#<<<<<<<<<< Subdirectories >>>>>>>>>>#
#----------------------------------#
# external libraries
add_subdirectory(external/)

# PeriDEM sub-directories
add_subdirectory(src/fe)
add_subdirectory(src/fracture)
add_subdirectory(src/geom)
add_subdirectory(src/inp)
add_subdirectory(src/loading)
add_subdirectory(src/material)
add_subdirectory(src/mesh)
add_subdirectory(src/mesh_gen)
add_subdirectory(src/model)
add_subdirectory(src/nsearch)
add_subdirectory(src/particle)
add_subdirectory(src/rw)
add_subdirectory(src/util)

# test directory
if(${Enable_Tests})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Building test")
    endif()
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
if(${Enable_Apps})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Building apps")
    endif()
    add_subdirectory(apps)
endif()

#----------------------------------#
#<<<<<<<<<< PeriDEM executable >>>>>>>>>>#
#----------------------------------#
add_executable(PeriDEM src/main.cpp)

target_include_directories(PeriDEM
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(PeriDEM PUBLIC Model)

install(TARGETS FE Fracture Geom Inp Loading Material Mesh MeshGen Model PeriDEM NSearch Particle RW Util 
  EXPORT PeriDEMTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(EXPORT PeriDEMTargets
  FILE PeriDEMTargets.cmake
  NAMESPACE PeriDEM::
  DESTINATION lib/cmake/PeriDEM
)


#----------------------------------#
#<<<<<<<<<< PeriDEM install >>>>>>>>>>#
#----------------------------------#
install(FILES ${PROJECT_BINARY_DIR}/PeriDEMConfig.h DESTINATION include)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES external/csv/csv.h DESTINATION include/csv)
install(DIRECTORY external/nanoflann/ DESTINATION include/nanoflann FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY external/taskflow/taskflow/ DESTINATION include/taskflow)

configure_package_config_file(
  cmake/PeriDEMConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/PeriDEMConfig.cmake
  INSTALL_DESTINATION lib/cmake/PeriDEM
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/PeriDEMConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/PeriDEMConfigVersion.cmake
  ${PROJECT_SOURCE_DIR}/cmake/FindMetis.cmake
  DESTINATION lib/cmake/PeriDEM
)



#----------------------------------#
#<<<<<<<<<< Post build operations >>>>>>>>>>#
#----------------------------------#
# test directory
if(${Enable_Tests})
    if (${Enable_CMAKE_Debug_Build})
        message(STATUS "Copying test directory to build directory")
    endif()
    file(COPY
            ${CMAKE_SOURCE_DIR}/test/test_data/
            DESTINATION
            ${PROJECT_BINARY_DIR}/test/test_data/)
endif()
