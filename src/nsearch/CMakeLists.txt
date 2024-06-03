# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

#include_directories(${PCL_INCLUDE_DIRS})
#link_directories(${PCL_LIBRARY_DIRS})

AUX_SOURCE_DIRECTORY(./ SOURCES)

add_library(NSearch ${SOURCES})

# target_link_libraries(NSearch PUBLIC nanoflann::nanoflann)
