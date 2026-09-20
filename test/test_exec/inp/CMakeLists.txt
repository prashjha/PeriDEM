# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

add_executable(TestDeckField testDeckField.cpp)

target_link_libraries(TestDeckField PUBLIC
        Inp Geom Util)

add_test(NAME test_deck_field COMMAND TestDeckField)
set_tests_properties(test_deck_field PROPERTIES LABELS "Inp")

add_executable(TestDeckRoundTrip testDeckRoundTrip.cpp)

target_link_libraries(TestDeckRoundTrip PUBLIC
        Inp Geom Util)

add_test(NAME test_deck_roundtrip COMMAND TestDeckRoundTrip)
set_tests_properties(test_deck_roundtrip PROPERTIES LABELS "Inp")
