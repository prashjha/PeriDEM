# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

add_hpx_executable(TestPeriDEM SOURCES testPeriDEM.cpp testPeriDEMLib.cpp DEPENDENCIES Inp Model)
