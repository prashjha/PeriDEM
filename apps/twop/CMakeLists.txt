# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(twop main.cpp)

target_link_libraries(twop PUBLIC Inp Model)

if (${Enable_CMAKE_Debug_Build})
    message(STATUS "Copying apps/twop example directory to build directory")
endif()
add_custom_command(
        TARGET twop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/example/
        ${EXECUTABLE_OUTPUT_PATH}/example/
)