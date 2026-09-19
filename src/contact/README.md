# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)
#
# Contact / wall / self-contact policy selection (input deck).
#
# Defaults (Jha-compatible):
#   Contact.Damping_Law  = com_and_node
#   Contact.Friction_Law = coulomb_simple
#   Contact.Correct_Volume = true   # partial Vj near Rc (modular default)
#   Model.Self_Contact   = broken_bond_kn
#   Model.Wall_Contact   = meshed
#   Model.Bond_Break     = tension
#
# Alternates:
#   Damping_Law:  com | node | off
#   Friction_Law: stick_slip
#   Correct_Volume: false  # full Vj — matches main-branch DEM contact spring
#   Self_Contact: reference_gap | none
#   Wall_Contact: analytical_plane
#   Bond_Break:   absolute_stretch
#
# Normal contact spring is force density ∝ Vj (corrected when Correct_Volume
# is true). The velocity update is v += (dt/rho)*f.
#
# Set policies in JSON under Contact / Model, or when building a deck in C++
# (see test/test_data/peridem/*_inbuilt drivers). Factories live in
# contact/policy.cpp, pd/selfContact.cpp, contact/wallContact.cpp.
