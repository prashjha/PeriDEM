# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

# PeriDEM (apps/peridem) is always built. Extra apps: -DEnable_Apps=ON
add_subdirectory(peridem)

if(${Enable_Apps})
  add_subdirectory(twoparticle_demo)
  add_subdirectory(peridynamics)
  add_subdirectory(example-modular)
endif()
