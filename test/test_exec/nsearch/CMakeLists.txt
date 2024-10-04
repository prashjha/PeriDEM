# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

AUX_SOURCE_DIRECTORY(./ SOURCES)

add_executable(TestNSearch ${SOURCES})

target_link_libraries(TestNSearch PUBLIC Util FE Geometry Particle NSearch fmt::fmt Threads::Threads)
