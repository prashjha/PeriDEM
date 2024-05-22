/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <string>
#include <vector>

namespace test {

/*!
 * @brief Perform test on taskflow
 * @param N size of vector to profile taskflow
 * @param seed Seed
 * @return str String containing various information
 */
std::string testTaskflow(size_t N, int seed);

} // namespace test