/*
 * ----------------------------------
 * Copyright (c) 2021 Prashant K. Jha
 * ----------------------------------
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef TESTFELIB_H
#define TESTFELIB_H

#include <string>
#include <vector>

/*! @brief Namespace to group the methods used in testing of the library */
namespace test {

/**@{*/

/*!
 * @brief Perform test on quadrature points on line elements (NOT IMPLEMENTED)
 *
 * This function performs accuracy test of the quadrature points for
 * integration over reference line with vertices at {-1, 1}. List of tests
 * are as follows:
 *
 * 1. Computes quadrature points of the given order, writes them to the file,
 * and checks if the sum of quadrature weights is equal to 0.5 (area of
 * reference triangle).
 *
 * Also tests the exactness of the integration of the polynomial upto given
 * order.
 * Suppose \f$ n\f$ is the order of quadrature point, then we test if the
 * integration of the function \f$ f(s,t) = s^\alpha\, t^\beta \f$ is exact for
 * \f$ \alpha \f$ and \f$ \beta \f$ such that \f$ \alpha+\beta \leq n \f$. The
 * exact integration of function \f$ f\f$ over reference triangle is
 * \f[ I_{exact} = \int_0^1 \int_0^{1-s} s^\alpha\, t^\beta \, dt\, ds =
 * \sum_{i=0}^{\beta+1} (-1)^i \frac{{{\beta + 1} \choose i}}{(\alpha + i +1)
 * (\beta + 1)},
 * \f]
 * where \f[ {a \choose b} = \frac{a (a-1) (a-2) ... (a-b+1)}{1*2*3 ... *b}. \f]
 * We have \f$ {a \choose 0} = 1 \f$ so that
 * term for \f$ i=0\f$ is not zero. Above formula gives the exact value of
 * integral of \f$ f(s,t) = s^\alpha\, t^\beta \f$ over reference triangle.
 * Approximation by quadrature point is as follows
 * \f[ I_{approx} = \sum_{q=1}^{Q} w_q f(s_q, t_q) \f]
 * where \f$Q\f$ is the total number of quad points, \f$ w_q\f$ and
 * \f$(s_q, t_q)\f$ are the \f$ q^{th} \f$ quad weight and point. In this
 * test, we compare \f$ I_{exact} \f$ and \f$ I_{approx} \f$ and report
 * problem if both do not match.
 *
 * 3. Test the accuracy for simple quadrangle mesh on square domain [0,1]^2.
 * Exact integration of polynomial \f$ f(x,y) = x^\alpha \, y^\beta \f$ on
 * \f$[0,1]^2\f$ is given by
 * \f[ I_{exact} = \frac{1}{(\alpha+1) (\beta+1)}. \f]
 * We compare above with the approximation computed from the quadrature
 * points. We consider \f$\alpha + \beta \leq n \f$ where \f$ n\f$ is the
 * order of approximation we are testing.
 *
 * @param n Order of quadrature point approximation
 * @param filepath Path where mesh data for test can be found (expects files 'triMesh_nodes.csv' and 'triMesh_elements.csv' inside the filepath)
 */
void testLineElem(size_t n, std::string filepath);

/*!
 * @brief Perform test on quadrature points on triangle elements
 *
 * This function performs accuracy test of the quadrature points for
 * integration over reference triangle with vertices at {(0,0), (1,0), (0,1)}.
 * List of tests are as follows:
 *
 * 1. Computes quadrature points of the given order, writes them to the file,
 * and checks if the sum of quadrature weights is equal to 0.5, i.e. area of
 * reference triangle.
 *
 * Also tests the exactness of the integration of the polynomial upto given
 * order.
 * Suppose \f$ n\f$ is the order of quadrature point, then we test if the
 * integration of the function \f$ f(s,t) = s^\alpha\, t^\beta \f$ is exact for
 * \f$ \alpha \f$ and \f$ \beta \f$ such that \f$ \alpha+\beta \leq n \f$. The
 * exact integration of function \f$ f\f$ over reference triangle is
 * \f[ I_{exact} = \int_0^1 \int_0^{1-s} s^\alpha\, t^\beta \, dt\, ds =
 * \sum_{i=0}^{\beta+1} (-1)^i \frac{{{\beta + 1} \choose i}}{(\alpha + i +1)
 * (\beta + 1)},
 * \f]
 * where \f[ {a \choose b} = \frac{a (a-1) (a-2) ... (a-b+1)}{1*2*3 ... *b}. \f]
 * We have \f$ {a \choose 0} = 1 \f$ so that
 * term for \f$ i=0\f$ is not zero. Above formula gives the exact value of
 * integral of \f$ f(s,t) = s^\alpha\, t^\beta \f$ over reference triangle.
 * Approximation by quadrature point is as follows
 * \f[ I_{approx} = \sum_{q=1}^{Q} w_q f(s_q, t_q) \f]
 * where \f$Q\f$ is the total number of quad points, \f$ w_q\f$ and
 * \f$(s_q, t_q)\f$ are the \f$ q^{th} \f$ quad weight and point. In this
 * test, we compare \f$ I_{exact} \f$ and \f$ I_{approx} \f$ and report
 * problem if both do not match.
 *
 * 3. Test the accuracy for simple triangular mesh on square domain [0,1]^2.
 * Exact integration of polynomial \f$ f(x,y) = x^\alpha \, y^\beta \f$ on
 * \f$[0,1]^2\f$ is given by
 * \f[ I_{exact} = \frac{1}{(\alpha+1) (\beta+1)}. \f]
 * We compare above with the approximation computed from the quadrature
 * points. We consider \f$\alpha + \beta \leq n \f$ where \f$ n\f$ is the
 * order of approximation we are testing.
 *
 * @param n Order of quadrature point approximation
 * @param filepath Path where mesh data for test can be found (expects files 'triMesh_nodes.csv' and 'triMesh_elements.csv' inside the filepath)
 */
void testTriElem(size_t n, std::string filepath);

/*!
 * @brief Perform test on quadrature points on quadrangle elements
 *
 * This function performs accuracy test of the quadrature points for
 * integration over reference triangle with vertices at {(0,0), (1,0), (0,1)}.
 * List of tests are as follows:
 *
 * 1. Computes quadrature points of the given order, writes them to the file,
 * and checks if the sum of quadrature weights is equal to 2, i.e. area of
 * reference quadrangle element.
 *
 * Also tests the exactness of the integration of the polynomial upto given
 * order.
 * Suppose \f$ n\f$ is the order of quadrature point, then we test if the
 * integration of the function \f$ f(s,t) = s^\alpha\, t^\beta \f$ is exact for
 * \f$ \alpha \f$ and \f$ \beta \f$ such that \f$ \alpha+\beta \leq n \f$. The
 * exact integration of function \f$ f\f$ over reference triangle is
 * \f[ I_{exact} = \int_0^1 \int_0^{1-s} s^\alpha\, t^\beta \, dt\, ds =
 * \sum_{i=0}^{\beta+1} (-1)^i \frac{{{\beta + 1} \choose i}}{(\alpha + i +1)
 * (\beta + 1)},
 * \f]
 * where \f[ {a \choose b} = \frac{a (a-1) (a-2) ... (a-b+1)}{1*2*3 ... *b}. \f]
 * We have \f$ {a \choose 0} = 1 \f$ so that
 * term for \f$ i=0\f$ is not zero. Above formula gives the exact value of
 * integral of \f$ f(s,t) = s^\alpha\, t^\beta \f$ over reference triangle.
 * Approximation by quadrature point is as follows
 * \f[ I_{approx} = \sum_{q=1}^{Q} w_q f(s_q, t_q) \f]
 * where \f$Q\f$ is the total number of quad points, \f$ w_q\f$ and
 * \f$(s_q, t_q)\f$ are the \f$ q^{th} \f$ quad weight and point. In this
 * test, we compare \f$ I_{exact} \f$ and \f$ I_{approx} \f$ and report
 * problem if both do not match.
 *
 * 3. Test the accuracy for simple quadrangle mesh on square domain [0,1]^2.
 * Exact integration of polynomial \f$ f(x,y) = x^\alpha \, y^\beta \f$ on
 * \f$[0,1]^2\f$ is given by
 * \f[ I_{exact} = \frac{1}{(\alpha+1) (\beta+1)}. \f]
 * We compare above with the approximation computed from the quadrature
 * points. We consider \f$\alpha + \beta \leq n \f$ where \f$ n\f$ is the
 * order of approximation we are testing.
 *
 * @param n Order of quadrature point approximation
 * @param filepath Path where mesh data for test can be found (expects files 'quadMesh_nodes.csv' and 'quadMesh_elements.csv' inside the filepath)
 */
void testQuadElem(size_t n, std::string filepath);

/*!
 * @brief Perform test on quadrature points on tetrahedral elements
 *
 * @param n Order of quadrature point approximation
 * @param filepath Path where mesh data for test can be found (expects files 'tetMesh_nodes.csv' and 'tetMesh_elements.csv' inside the filepath)
 */
void testTetElem(size_t n, std::string filepath);

/*!
 * @brief Computes the time needed when quad data for elements are stored and
 * when they are computed as and when needed
 *
 * This function allocates dummy elements and test how much time it is
 * required to do computation when the quad data are stored for each element
 * and when the quad data are computed.
 *
 * @param n Order of quadrature point approximation
 * @param N Number of elements on which this test is performed
 */
void testTriElemTime(size_t n, size_t N);

/** @}*/

/*!
 * @brief Computes integration of polynomial exactly over reference triangle
 *
 * Given \f$ f(s,t) = s^\alpha\, t^\beta \f$, the exact integration is given by
 * \f[ I_{exact} = \int_0^1 \int_0^{1-s} s^\alpha\, t^\beta \, dt\, ds =
 * \sum_{i=0}^{\beta+1} (-1)^i \frac{{{\beta + 1} \choose i}}{(\alpha + i +1)
 * (\beta + 1)}, \f]
 * where \f[ {a \choose b} = \frac{a (a-1) (a-2) ... (a-b+1)}{1*2*3 ... *b}. \f]
 * We have \f$ {a \choose 0} = 1 \f$ so that
 * term for \f$ i=0\f$ is not zero. Above formula gives the exact value of
 * integral of \f$ f(s,t) = s^\alpha\, t^\beta \f$ over reference triangle.
 *
 * @param alpha Polynomial order in variable s
 * @param beta Polynomial order in variable t
 * @return I Exact integration of \f$ f(s,t) = s^\alpha\, t^\beta \f$
 */
double getExactIntegrationRefTri(size_t alpha, size_t beta);

/*!
 * @brief Computes integration of polynomial exactly over reference quadrangle
 *
 * Given \f$ f(s,t) = s^\alpha\, t^\beta \f$, the exact integration is given by
 * \f[ I_{exact} = \int_0^1 \int_0^{1-s} s^\alpha\, t^\beta \, dt\, ds. \f]
 *
 * If either \f$ \alpha\f$ or \f$ \beta\f$ are odd number then \f$ I_{exact}
 * = 0\f$. Otherwise, \f$ I_{exact} = \frac{4}{(\alpha +1) (\beta+1)} \f$.
 *
 * @param alpha Polynomial order in variable s
 * @param beta Polynomial order in variable t
 * @return I Exact integration of \f$ f(s,t) = s^\alpha\, t^\beta \f$
 */
double getExactIntegrationRefQuad(size_t alpha, size_t beta);

/*!
 * @brief Computes integration of polynomial exactly over reference tetrahedral
 *
 * @param alpha Polynomial order in variable s
 * @param beta Polynomial order in variable t
 * @param theta Polynomial order in variable r
 * @return I Exact integration of \f$ f(s,t) = s^\alpha\, t^\beta \, r^\theta
 * \f$
 */
double getExactIntegrationRefTet(size_t alpha, size_t beta, size_t theta);

/*!
 * @brief Computes \f$ {n\choose r}\f$ "n choose r"
 *
 * Computes formula \f[ {a \choose b} = \frac{a (a-1) (a-2) ... (a-b+1)}{1*2*3
 * ... *b}. \f]
 *
 * @param n Number
 * @param r Number which is smaller or equal to n
 * @return Value Value of "n choose r"
 */
double getNChooseR(size_t n, size_t r);

} // namespace test

#endif // TESTFELIB_H
