# -------------------------------------------
# Copyright (c) 2021 - 2024 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

# Extra apps: -DEnable_Apps=ON. Default PeriDEM executable is PeriDEM/ at repo root.
if(${Enable_Apps})
  add_subdirectory(twoparticle_demo)
  add_subdirectory(peridynamics)
  add_subdirectory(example-modular)
endif()

add_subdirectory(twoparticle_custom)
