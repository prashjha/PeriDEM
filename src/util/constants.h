/*
* -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTILS_CONSTANTS_H
#define UTILS_CONSTANTS_H

namespace util {
    /*! @brief Provides geometrical methods such as point inside rectangle */
    namespace io {
        /*! @brief Default value of tab used in outputting formatted information */
        const int print_default_tab = 0;

        /*! @brief Default mpi rank in printing information */
        const int print_default_mpi_rank = 0;

        /*! @brief Default debug level for logger */
        const int logger_default_debug_lvl = 5;
    } // namespace io
} // namespace util

#endif //UTILS_CONSTANTS_H
