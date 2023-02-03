# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

add_executable(TestParticle testParticle.cpp testParticleLib.cpp)

target_link_libraries(TestParticle PUBLIC Util RW FE Geometry Particle)
