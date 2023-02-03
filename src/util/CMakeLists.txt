# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

AUX_SOURCE_DIRECTORY(./ SOURCES)

add_library(Util ${SOURCES})

target_link_libraries(Util PUBLIC ${LAPACK_LIBRARIES} ${BLAS_LIBRARIES} fmt::fmt Threads::Threads)