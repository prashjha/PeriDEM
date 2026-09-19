# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module)

find_package(nanobind CONFIG QUIET)
if(NOT nanobind_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind.git
    GIT_TAG v2.7.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(nanobind)
endif()

nanobind_add_module(peridem_core NB_STATIC bindings.cpp)
set_target_properties(peridem_core PROPERTIES OUTPUT_NAME _core)
target_link_libraries(peridem_core PRIVATE PeriDEMModel)

if(NOT SKBUILD)
  set(_py_out "${PROJECT_BINARY_DIR}/python/peridem")
  set_target_properties(peridem_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${_py_out}"
  )
  file(GLOB _py_pkg "${CMAKE_CURRENT_SOURCE_DIR}/peridem/*.py")
  foreach(_f ${_py_pkg})
    configure_file(${_f} ${_py_out}/ COPYONLY)
  endforeach()
endif()

install(TARGETS peridem_core LIBRARY DESTINATION peridem)
install(DIRECTORY peridem/ DESTINATION peridem FILES_MATCHING PATTERN "*.py")
