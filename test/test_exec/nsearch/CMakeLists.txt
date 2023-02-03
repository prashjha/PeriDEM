# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

add_executable(TestNSearch testNSearch.cpp testNSearchLib.cpp)

target_link_libraries(TestNSearch PUBLIC Util FE Geometry Particle NSearch fmt::fmt)
