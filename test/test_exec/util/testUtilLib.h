/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2026 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef TESTFELIB_H
#define TESTFELIB_H

#include <string>
#include <vector>

namespace test {

/*! @brief Test methods  */
void testUtilMethods();

/*!
 * @brief Checks the contact-stiffness helpers are bit-for-bit what the
 * expressions they replaced produced
 *
 * util::normalContactStiffness and util::selfContactStiffness replaced the
 * same formula at nine call sites. The two differ only in the order of
 * operations, which changes the result in the last bit, so each has to
 * reproduce the grouping of the expression it replaced. Otherwise collecting
 * them changes the contact force in every example.
 */
void testContactStiffness();

} // namespace test

#endif // TESTFELIB_H
