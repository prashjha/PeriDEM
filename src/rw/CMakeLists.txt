# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

AUX_SOURCE_DIRECTORY(./ SOURCES)

add_library(RW ${SOURCES})

target_link_libraries(RW PUBLIC ${VTK_LIBRARIES})

target_include_directories(RW
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external>
    $<INSTALL_INTERFACE:include>
)
