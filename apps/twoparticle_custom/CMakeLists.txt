# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

add_executable(TwoParticle_Custom
  main.cpp
  appDamping.cpp
  appPostprocess.cpp
)

target_include_directories(TwoParticle_Custom
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(TwoParticle_Custom PUBLIC PeriDEMModel)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
