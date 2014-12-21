//
// VRPN math functions
//
// vrpn_Math.h
//
// Author:
//   Kevin M. Godby <kevin@godby.org>
//
// Copyright (c) 2014
//

#ifndef VRPN_MATH_H_
#define VRPN_MATH_H_

//#include <boost/utility/enable_if.hpp>       // for enable_if
//#include <boost/type_traits/is_integral.hpp> // for is_integral
#include <cmath>                             // for floor, ceil

/**
 * Computes the nearest integer not greater in magnitude than x.
 *
 * Equivalent to C++11's std::trunc() function.
 */
template <typename T>
inline T trunc(T x)
{
    return (x > 0) ? std::floor(x) : std::ceil(x);
}

/**
 * Computes the nearest integer not greater in magnitude than x.
 *
 * Equivalent to C++11's std::trunc() function.
 */
/*
template <typename T>
inline typename boost::enable_if<typename boost::is_integral<T>::value, double>::type
trunc(T x)
{
    return trunc(static_cast<double>(x));
}
*/

#endif // VRPN_MATH_H_

