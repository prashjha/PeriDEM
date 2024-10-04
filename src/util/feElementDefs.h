/*
 * -------------------------------------------
 * Copyright (c) 2021 - 2024 Prashant K. Jha
 * -------------------------------------------
 * PeriDEM https://github.com/prashjha/PeriDEM
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE)
 */

#ifndef UTIL_GLOBALDEFS_H
#define UTIL_GLOBALDEFS_H

namespace util {

/**
 * \defgroup GlobalVars GlobalVars
 */
/**@{*/

/**
 * @name VTK Element types
 */
/**@{*/

//*! @brief Element type (for vtk)  */
//#define VTK_VERTEX 1                    // 1-node vertex
//#define VTK_POLY_VERTEX 2               // n-node polygonal vertex no edges
//#define VTK_LINE 3                      // 2-node line
//#define VTK_POLY_LINE 4                 // n-node line
//#define VTK_TRIANGLE 5                  // 3-node triangle
//#define VTK_TRIANGLE_STRIP 6            // n-node sequence of triangles
//#define VTK_POLYGON 7                   // n-node polygon
//#define VTK_PIXEL 8                     // 4-node pixel
//#define VTK_QUAD 9                      // 4-node quadrangle
//#define VTK_TETRA 10                    // 4-node tetrahedron
//#define VTK_VOXEL 11                    // 8-node voxel
//#define VTK_HEXAHEDRON 12               // 8-node hexahedron
//#define VTK_WEDGE 13                    // 6-node wedge (prism)
//#define VTK_PYRAMID 14                  // 5-node pyramid

/*! @brief Integer flag for vertex (point) element */
static const int vtk_type_vertex = 1;

/*! @brief Integer flag for poly vertex element */
static const int vtk_type_poly_vertex = 2;

/*! @brief Integer flag for line element */
static const int vtk_type_line = 3;

/*! @brief Integer flag for poly line element */
static const int vtk_type_poly_line = 4;

/*! @brief Integer flag for triangle element */
static const int vtk_type_triangle = 5;

/*! @brief Integer flag for triangle strip element */
static const int vtk_type_triangle_strip = 6;

/*! @brief Integer flag for polygon element */
static const int vtk_type_polygon = 7;

/*! @brief Integer flag for pixel element */
static const int vtk_type_pixel = 8;

/*! @brief Integer flag for quad element */
static const int vtk_type_quad = 9;

/*! @brief Integer flag for tetrahedron element */
static const int vtk_type_tetra = 10;

/*! @brief Integer flag for voxel element */
static const int vtk_type_voxel = 11;

/*! @brief Integer flag for hexahedron element */
static const int vtk_type_hexahedron = 12;

/*! @brief Integer flag for wedge element */
static const int vtk_type_wedge = 13;

/*! @brief Integer flag for pyramid element */
static const int vtk_type_pyramid = 14;

/*! @brief Map from element type to number of nodes (for vtk) */
static int vtk_map_element_to_num_nodes[16] = {0, 1, -1, 2, -1, 3, -1, -1,
                                               4, 4, 4,  8, 8,  6, 5,  -1};

/*! @brief Map from vtk element type to msh element type */
static int vtk_to_msh_element_type_map[16] = {-1, 15, -1, 1, -1, 2, -1, -1,
                                               -1, 3, 4, -1, 5, 6, 7, -1};

/** @}*/

/**
 * @name Gmsh Element types
 */
/**@{*/

//*! @brief Element type (for gmsh)  */
//#define MSH_LINE 1                      // 2-node line
//#define MSH_TRIANGLE 2                  // 3-node triangle
//#define MSH_QUAD 3                      // 4-node quadrangle
//#define MSH_TETRA 4                     // 4-node tetrahedron
//#define MSH_HEXA 5                      // 8-node hexahedron
//#define MSH_PRISM 6                     // 6-node wedge (prism)
//#define MSH_PYRAMID 7                   // 5-node pyramid
//#define MSH_LINE_ORDER_TWO 8            // 3-node second order line
//#define MSH_TRIANGLE_ORDER_TWO 9        // 6-node second order triangle
//#define MSH_QUAD_ORDER_TWO 10           // 10-node second order tetrahedron
//#define MSH_VERTEX 15                   // 1-node vertex

/*! @brief Integer flag for line element */
static const int msh_type_line = 1;

/*! @brief Integer flag for triangle element */
static const int msh_type_triangle = 2;

/*! @brief Integer flag for quadrangle element */
static const int msh_type_quadrangle = 3;

/*! @brief Integer flag for tetrahedron element */
static const int msh_type_tetrahedron = 4;

/*! @brief Integer flag for hexahedron element */
static const int msh_type_hexahedron = 5;

/*! @brief Integer flag for prism element */
static const int msh_type_prism = 6;

/*! @brief Integer flag for pyramid element */
static const int msh_type_pyramid = 7;

/*! @brief Integer flag for line (second order) element */
static const int msh_type_line_second_order = 8;

/*! @brief Integer flag for traingle (second order) element */
static const int msh_type_traingle_second_order = 9;

/*! @brief Integer flag for quadrangle (second order) element */
static const int msh_type_quadrangle_second_order = 10;

/*! @brief Integer flag for vertex (point) element */
static const int msh_type_vertex = 15;

/*! @brief Map from element type to number of nodes (for msh) */
static int msh_map_element_to_num_nodes[16] = {0, 2, 3,  4, 4, 8, 6, 5,
                                               3, 6, 10, 0, 0, 0, 0, 1};

/** @}*/

/**
 * @name Fixity mask
 */
/**@{*/

/*! @brief Free mask (none of the coordinates are fixed)  */
#define FREE_MASK 0x000

/*! @brief X-coordinate is fixed  */
#define FIX_X_MASK 0x001

/*! @brief Y-coordinate is fixed  */
#define FIX_Y_MASK 0x002

/*! @brief Z-coordinate is fixed  */
#define FIX_Z_MASK 0x004

/** @}*/

/** @}*/

} // namespace util

#endif // UTIL_GLOBALDEFS_H
