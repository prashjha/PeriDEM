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
  # Stage the whole package on every build. A configure-time GLOB would miss
  # newly added modules, and a POST_BUILD hook would not run when only the
  # .py files changed.
  add_custom_target(peridem_python_package ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/peridem" "${_py_out}"
    COMMENT "Staging peridem Python package")
  add_dependencies(peridem_python_package peridem_core)
endif()

# ---------------------------------------------------------------------------
# Tests
#
# run_tests.py uses pytest when it is installed and falls back to a built-in
# runner otherwise, so pytest is not a build dependency.
#
# Test_Python_API covers the binding surface (geometry, deck building, particle
# views, bonds). Test_Python_Example_Parity builds each example's deck in
# Python, compares it to the deck the matching C++ driver writes, then runs the
# same deck through bin/PeriDEM and through the binding and compares the nodes.
# ---------------------------------------------------------------------------
if(Enable_Tests AND NOT SKBUILD)
  set(_py_env
    "PYTHONPATH=${PROJECT_BINARY_DIR}/python:$ENV{PYTHONPATH}"
    "PERIDEM_BIN=$<TARGET_FILE:PeriDEM>")

  add_test(NAME Test_Python_API
    COMMAND ${Python_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py" test_api.py)
  set_tests_properties(Test_Python_API PROPERTIES
    ENVIRONMENT "${_py_env}" LABELS "python")

  add_test(NAME Test_Python_VTK_IO
    COMMAND ${Python_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py" test_vtk_io.py)
  set_tests_properties(Test_Python_VTK_IO PROPERTIES
    ENVIRONMENT "${_py_env}" LABELS "python")

  # Needs bin/PeriDEM plus the example drivers; run it after they are built.
  add_test(NAME Test_Python_Example_Parity
    COMMAND ${Python_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_example_parity.py")
  set_tests_properties(Test_Python_Example_Parity PROPERTIES
    ENVIRONMENT "${_py_env}" LABELS "python;slow" TIMEOUT 2400)
endif()

install(TARGETS peridem_core LIBRARY DESTINATION peridem)
install(DIRECTORY peridem/ DESTINATION peridem FILES_MATCHING PATTERN "*.py")
